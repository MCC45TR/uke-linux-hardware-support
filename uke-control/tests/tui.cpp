// SPDX-License-Identifier: MIT
#include <cassert>
#include <cerrno>
#include <chrono>
#include <clocale>
#include <climits>
#include <csignal>
#include <cwchar>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <pty.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <vterm.h>
using namespace std::chrono_literals;
class Pty {
    int master_=-1,slave_=-1;
    pid_t pid_=-1;
    termios original_{};
    int status_=0;
    VTerm* terminal_=nullptr;
    VTermScreen* screen_=nullptr;
public:
    std::string output;
    explicit Pty(std::vector<std::string> options,unsigned columns=120,unsigned rows=36) {
        terminal_=vterm_new(int(rows),int(columns));assert(terminal_);vterm_set_utf8(terminal_,1);
        screen_=vterm_obtain_screen(terminal_);vterm_screen_enable_altscreen(screen_,1);vterm_screen_reset(screen_,1);
        winsize size{};size.ws_col=columns;size.ws_row=rows;
        assert(!openpty(&master_,&slave_,nullptr,nullptr,&size));assert(!tcgetattr(slave_,&original_));
        pid_=fork();assert(pid_>=0);
        if(!pid_) {
            close(master_);assert(setsid()>=0);assert(!ioctl(slave_,TIOCSCTTY,0));
            for(int fd=0;fd<3;++fd)assert(dup2(slave_,fd)>=0);
            if(slave_>2)close(slave_);
            setenv("TERM","xterm-256color",1);setenv("LANG","C.UTF-8",1);setenv("LC_ALL","C.UTF-8",1);
            std::vector<std::string> args{"build/uke-control-tui"};args.insert(args.end(),options.begin(),options.end());
            std::vector<char*> raw;for(auto& arg:args)raw.push_back(arg.data());raw.push_back(nullptr);
            execv(raw[0],raw.data());_exit(127);
        }
        assert(fcntl(master_,F_SETFL,O_NONBLOCK)>=0);
    }
    ~Pty(){if(pid_>0){kill(pid_,SIGKILL);waitpid(pid_,nullptr,0);}close(master_);close(slave_);vterm_free(terminal_);}
    std::string screen()const {
        int rows,cols;vterm_get_size(terminal_,&rows,&cols);std::string value;
        for(int row=0;row<rows;++row){for(int col=0;col<cols;++col){VTermScreenCell cell{};if(!vterm_screen_get_cell(screen_,VTermPos{row,col},&cell))continue;
            if(!cell.chars[0]){value+=' ';continue;}
            for(auto cp:cell.chars){if(!cp)break;if(cp==UINT32_MAX)continue;char bytes[MB_LEN_MAX];mbstate_t state{};const auto n=wcrtomb(bytes,wchar_t(cp),&state);if(n!=size_t(-1))value.append(bytes,n);}
        }value+='\n';}
        return value;
    }
    void drain(unsigned milliseconds=100) {
        const auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(milliseconds);
        do {
            pollfd poller{master_,POLLIN,0};poll(&poller,1,10);
            char bytes[8192];ssize_t count;
            while((count=read(master_,bytes,sizeof(bytes)))>0){output.append(bytes,size_t(count));assert(output.size()<4*1024*1024);assert(vterm_input_write(terminal_,bytes,size_t(count))==size_t(count));vterm_screen_flush_damage(screen_);}
        }while(std::chrono::steady_clock::now()<until);
    }
    void expect(const std::string& value,unsigned milliseconds=2500) {
        const auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(milliseconds);
        while(screen().find(value)==std::string::npos&&std::chrono::steady_clock::now()<until)drain(20);
        if(screen().find(value)==std::string::npos)throw std::runtime_error("Missing terminal text: "+value+"\n"+screen());
    }
    void send(const std::string& value){output.clear();assert(write(master_,value.data(),value.size())==ssize_t(value.size()));drain(80);}
    void resize(unsigned columns,unsigned rows){vterm_set_size(terminal_,int(rows),int(columns));winsize size{};size.ws_col=columns;size.ws_row=rows;assert(!ioctl(master_,TIOCSWINSZ,&size));kill(pid_,SIGWINCH);output.clear();drain(120);}
    void signal(int signal){assert(!kill(pid_,signal));}
    int finish() {
        const auto until=std::chrono::steady_clock::now()+3s;
        while(std::chrono::steady_clock::now()<until){const auto got=waitpid(pid_,&status_,WNOHANG);if(got==pid_){pid_=-1;termios restored{};assert(!tcgetattr(slave_,&restored));assert(restored.c_lflag==original_.c_lflag);assert(restored.c_iflag==original_.c_iflag);assert(restored.c_oflag==original_.c_oflag);return WIFEXITED(status_)?WEXITSTATUS(status_):128+WTERMSIG(status_);}drain(20);}
        throw std::runtime_error("TUI did not exit / restore its terminal");
    }
};
std::vector<std::string> fixture(std::string start="overview"){return {"--language","en","--fixture","tests/fixtures/tui.json","--start",start};}
void observations() {
    Pty p(fixture());p.expect("SIMULATION");p.expect("General device status");
    p.send("\n");p.expect("SYNTHETIC");p.expect("cycles: 0");p.expect("-42");p.expect("Not reported");
    p.send("J");p.expect("Raw JSON");p.send("S");p.expect("Saving is disabled");
    p.send("h");p.expect("Session history");p.send("\n");p.expect("Readable result");
    p.send("\t");p.expect("Categories");p.send("/zznomatching");p.expect("No matching action");p.send("\033");p.expect("General device status");
    p.send("\033OP");p.expect("Full help");p.send("\033OP");p.send("\033");p.expect("Categories");
    p.resize(80,24);p.expect("Categories");p.resize(50,12);p.expect("Terminal too small");p.resize(120,36);p.expect("Categories");
    p.send("q");assert(p.finish()==0);
}
void forms() {
    Pty p(fixture("charge.mode"),80,24);p.expect("Request charging policy");
    p.send("\n");p.expect("Charging policy");p.send("\n");p.expect("Daily 70");p.send("j");p.expect("80");p.send("\n");
    p.send("\033[19~");p.expect("Review the exact request");p.send("\n");p.expect("Changes are disabled");
    p.send("\033[18~");p.expect("Charging policy");p.send("\033[19~");p.expect("Review the exact request");p.send("\n");p.expect("requested mode");
    p.send("q");assert(p.finish()==0);
}
void cancellation() {
    Pty p(fixture("charge.status"));p.expect("Charging status");p.send("\n");p.expect("Reading:");p.send("\t");p.expect("Categories");
    p.send("\033");p.send("h");p.expect("Session history");p.send("\n");p.expect("Cancellation requested");
    p.send("q");assert(p.finish()==0);
}
void readonly_and_signals() {
    Pty p({"--read-only","--language","en","--start","charge.mode"});p.expect("READ ONLY");p.send("\n");p.expect("Charging policy");p.send("\033[19~");p.expect("Review the exact request");p.send("\n");p.expect("Changes are disabled");p.signal(SIGTERM);assert(p.finish()==143);
    Pty tr({"--fixture","tests/fixtures/tui.json"});tr.expect("SİMÜLASYON");tr.send("/ŞARJ");tr.expect("ŞARJ");tr.send("\n");tr.send("q");assert(tr.finish()==0);
    const std::string path="/tmp/uke-tui-invalid-"+std::to_string(getpid())+".json";
    {std::ofstream file(path);file<<"{\"schema\":\"org.uke.control.tui-fixture.v1\",\"results\":{},\"command\":\"/bin/true\"}";}
    Pty invalid({"--fixture",path});invalid.expect("Unknown fixture field");assert(invalid.finish()==1);unlink(path.c_str());
}
int main(){assert(setlocale(LC_ALL,"C.UTF-8"));observations();forms();cancellation();readonly_and_signals();std::cout<<"PTY navigation, forms, previews, cancellation, language, resize and terminal restoration passed\n";}
