// SPDX-License-Identifier: MIT
#include "client.hpp"
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <stdexcept>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
extern char** environ;
namespace uke::client {
namespace {
constexpr size_t stdout_limit=1024*1024,stderr_limit=256*1024;
struct Fd {int n=-1;~Fd(){if(n>=0)close(n);}void reset(){if(n>=0)close(n);n=-1;}};
struct Spawn {
    posix_spawn_file_actions_t files{};
    posix_spawnattr_t attributes{};
    Spawn() {
        const int f=posix_spawn_file_actions_init(&files);if(f)throw std::runtime_error(strerror(f));
        const int a=posix_spawnattr_init(&attributes);if(a){posix_spawn_file_actions_destroy(&files);throw std::runtime_error(strerror(a));}
    }
    ~Spawn(){posix_spawnattr_destroy(&attributes);posix_spawn_file_actions_destroy(&files);}
    static void check(int value){if(value)throw std::runtime_error(strerror(value));}
    void signals(bool group) {
        sigset_t empty,defaults;sigemptyset(&empty);sigemptyset(&defaults);
        for(const int signal:{SIGINT,SIGTERM,SIGHUP,SIGPIPE,SIGQUIT})sigaddset(&defaults,signal);
        check(posix_spawnattr_setsigmask(&attributes,&empty));check(posix_spawnattr_setsigdefault(&attributes,&defaults));
        check(posix_spawnattr_setflags(&attributes,POSIX_SPAWN_SETSIGMASK|POSIX_SPAWN_SETSIGDEF|(group?POSIX_SPAWN_SETPGROUP:0)));
        if(group)check(posix_spawnattr_setpgroup(&attributes,0));
    }
};
pid_t launch(Spawn& spawn,std::vector<std::string> arguments) {
    std::vector<char*> raw;for(auto& argument:arguments)raw.push_back(argument.data());raw.push_back(nullptr);
    pid_t pid=-1;const int code=posix_spawn(&pid,arguments[0].c_str(),&spawn.files,&spawn.attributes,raw.data(),environ);
    if(code)throw std::runtime_error("Cannot start command: "+std::string(strerror(code)));
    return pid;
}
int exit_code(int status){return WIFEXITED(status)?WEXITSTATUS(status):WIFSIGNALED(status)?128+WTERMSIG(status):125;}
}
struct Query::Impl {
    Plan request;
    Result result;
    pid_t pid=-1;
    Fd output,error;
    bool exited=false,termination_sent=false;
    std::chrono::steady_clock::time_point began=std::chrono::steady_clock::now(),terminate_at{};
    explicit Impl(std::string_view id,const Values& values):request(plan(id,values)) {
        if(request.effect!=Effect::read||request.presentation!=Presentation::capture)throw std::invalid_argument("Only captured read-only queries may run in the background");
        if(request.access==Access::root&&geteuid()!=0)throw std::invalid_argument("A privileged query requires explicit terminal authorization");
        result.action_id=request.action_id;
        int out[2],err[2];
        if(pipe2(out,O_CLOEXEC))throw std::runtime_error("Cannot open query output pipe");
        output.n=out[0];Fd output_write{out[1]};
        if(pipe2(err,O_CLOEXEC))throw std::runtime_error("Cannot open query error pipe");
        error.n=err[0];Fd error_write{err[1]};
        if(fcntl(output.n,F_SETFL,O_NONBLOCK)||fcntl(error.n,F_SETFL,O_NONBLOCK))throw std::runtime_error("Cannot make query pipes nonblocking");
        try {
            Spawn spawn;spawn.signals(true);
            Spawn::check(posix_spawn_file_actions_adddup2(&spawn.files,output_write.n,STDOUT_FILENO));
            Spawn::check(posix_spawn_file_actions_adddup2(&spawn.files,error_write.n,STDERR_FILENO));
            Spawn::check(posix_spawn_file_actions_addopen(&spawn.files,STDIN_FILENO,"/dev/null",O_RDONLY,0));
            Spawn::check(posix_spawn_file_actions_addclosefrom_np(&spawn.files,3));
            pid=launch(spawn,request.argv);result.running=true;
        } catch(const std::exception& failure){result.error=failure.what();result.exit_status=127;output.reset();error.reset();}
    }
    void terminate(bool cancelled) {
        if(!result.running||termination_sent)return;
        result.cancelled=cancelled;termination_sent=true;terminate_at=std::chrono::steady_clock::now();
        // The leader remains unreaped until all pipes close, preventing PGID reuse.
        if(pid>0)kill(-pid,SIGTERM);
    }
    void drain(Fd& fd,std::string& text,size_t maximum) {
        if(fd.n<0)return;
        std::array<char,8192> bytes{};
        for(unsigned iteration=0;iteration<32;++iteration) {
            const auto n=read(fd.n,bytes.data(),bytes.size());
            if(n==0){fd.reset();return;}
            if(n<0){if(errno==EINTR)continue;if(errno!=EAGAIN&&errno!=EWOULDBLOCK){result.error+="\nOutput read failed";terminate(false);fd.reset();}return;}
            const auto retained=std::min(size_t(n),maximum>text.size()?maximum-text.size():0);text.append(bytes.data(),retained);
            if(retained<size_t(n)){result.truncated=true;terminate(false);return;}
        }
    }
    void poll() {
        if(!result.running)return;
        const auto now=std::chrono::steady_clock::now();result.elapsed_ms=std::chrono::duration_cast<std::chrono::milliseconds>(now-began).count();
        drain(output,result.output,stdout_limit);drain(error,result.error,stderr_limit);
        siginfo_t information{};
        if(waitid(P_PID,id_t(pid),&information,WEXITED|WNOHANG|WNOWAIT)==0)exited=information.si_pid==pid;
        else if(errno!=EINTR) {
            // Another SIGCHLD owner must not make us signal a recycled process group.
            result.error+="\nQuery child ownership was lost";result.exit_status=125;result.running=false;pid=-1;output.reset();error.reset();return;
        }
        if(!termination_sent&&now-began>=std::chrono::seconds(request.timeout_seconds)){result.timed_out=true;terminate(false);}
        if(termination_sent&&now-terminate_at>=std::chrono::milliseconds(200)) {
            kill(-pid,SIGKILL);output.reset();error.reset();
        }
        if(exited&&((output.n<0&&error.n<0)||(termination_sent&&now-terminate_at>=std::chrono::milliseconds(250)))) {
            int status=0;pid_t reaped;do{reaped=waitpid(pid,&status,0);}while(reaped<0&&errno==EINTR);
            result.exit_status=reaped==pid?exit_code(status):125;
            if(result.timed_out||result.truncated)result.exit_status=124;
            if(result.cancelled)result.exit_status=130;
            result.running=false;pid=-1;output.reset();error.reset();
        }
    }
    ~Impl() {
        if(result.running){terminate(true);for(unsigned i=0;i<40&&result.running;++i){poll();if(result.running)std::this_thread::sleep_for(std::chrono::milliseconds(10));}}
        if(result.running&&pid>0) {
            kill(-pid,SIGKILL);
            // An uninterruptible kernel read must not block the GUI/destructor.
            const auto child=pid;
            try {std::thread([child]{int status;while(waitpid(child,&status,0)<0&&errno==EINTR){}}).detach();}
            catch(...) {/* Process shutdown will adopt the killed child if no reaper thread can be created. */}
        }
    }
};
Query::Query(std::string_view action,const Values& values):impl_(std::make_unique<Impl>(action,values)){}
Query::~Query()=default;
void Query::poll(){impl_->poll();}
void Query::cancel(){impl_->terminate(true);}
const Result& Query::result()const{return impl_->result;}
Result run_terminal(std::string_view id,const Values& values,bool approved) {
    const auto request=plan(id,values);
    if(request.confirmation_required&&!approved)throw std::invalid_argument("Explicit review approval is required");
    if(!isatty(STDIN_FILENO)||!isatty(STDOUT_FILENO))throw std::runtime_error("This action requires an interactive terminal");
    auto arguments=request.argv;
    if(request.access==Access::root&&geteuid()!=0)arguments.insert(arguments.begin(),{"/usr/bin/sudo","--"});
    Spawn spawn;spawn.signals(false);Spawn::check(posix_spawn_file_actions_addclosefrom_np(&spawn.files,3));
    const auto began=std::chrono::steady_clock::now();const auto pid=launch(spawn,arguments);
    int status=0;pid_t reaped;do{reaped=waitpid(pid,&status,0);}while(reaped<0&&errno==EINTR);
    Result result;result.action_id=request.action_id;result.exit_status=reaped==pid?exit_code(status):125;
    result.elapsed_ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();
    return result;
}
}
