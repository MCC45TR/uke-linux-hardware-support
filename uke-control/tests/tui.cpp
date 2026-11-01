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
#include <sstream>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <vterm.h>
#include "tui-menu.hpp"
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
    std::pair<int,int> locate(const std::string& label,int minimum_row=0)const {
        std::istringstream rows(screen());std::string row;int y=0;
        while(std::getline(rows,row)){
            const auto at=row.find(label);
            if(y>=minimum_row&&at!=std::string::npos){mbstate_t state{};int x=0;size_t offset=0;while(offset<at){wchar_t c{};const auto n=mbrtowc(&c,row.data()+offset,at-offset,&state);assert(n&&n!=size_t(-1)&&n!=size_t(-2));offset+=n;x+=std::max(0,wcwidth(c));}return {x,y};}
            ++y;
        }
        throw std::runtime_error("Missing clickable label: "+label+"\n"+screen());
    }
    void tap_at(int x,int y,bool settle=true){if(settle)drain(260);const auto position=std::to_string(x+1)+";"+std::to_string(y+1);send("\033[<0;"+position+"M");send("\033[<0;"+position+"m");}
    void tap(const std::string& label,int minimum_row=0){expect(label);const auto [x,y]=locate(label,minimum_row);tap_at(x,y);}
    void resize(unsigned columns,unsigned rows){vterm_set_size(terminal_,int(rows),int(columns));winsize size{};size.ws_col=columns;size.ws_row=rows;assert(!ioctl(master_,TIOCSWINSZ,&size));kill(pid_,SIGWINCH);output.clear();drain(120);}
    void signal(int signal){assert(!kill(pid_,signal));}
    int finish() {
        const auto until=std::chrono::steady_clock::now()+3s;
        while(std::chrono::steady_clock::now()<until){const auto got=waitpid(pid_,&status_,WNOHANG);if(got==pid_){pid_=-1;termios restored{};assert(!tcgetattr(slave_,&restored));assert(restored.c_lflag==original_.c_lflag);assert(restored.c_iflag==original_.c_iflag);assert(restored.c_oflag==original_.c_oflag);return WIFEXITED(status_)?WEXITSTATUS(status_):128+WTERMSIG(status_);}drain(20);}
        throw std::runtime_error("TUI did not exit / restore its terminal");
    }
};
std::vector<std::string> fixture(std::string start="home"){return {"--language","en","--fixture","tests/fixtures/tui.json","--start",start};}
void menu_coverage() {
    namespace menu=uke::tui_menu;std::map<std::string,unsigned> count;
    for(const auto& m:menu::menus())for(const auto& item:menu::items(m.id))if(item.action)++count[item.id];
    assert(count.size()==uke::client::actions().size());
    for(const auto& a:uke::client::actions()){assert(count[a.id]==1);const auto& leaf=menu::menu(menu::owner(a));std::string id=leaf.id;unsigned depth=0;while(id!="home"){id=menu::menu(id).parent;assert(++depth<=3);}}
    assert(menu::owner(uke::client::action("nfc.status"))=="nfc");
    assert(menu::owner(uke::client::action("charge.protocol-inspect"))=="charge-expert");
    assert(menu::owner(uke::client::action("resize.prepare"))=="recovery");
}
void navigation() {
    Pty p(fixture());p.expect("Categories");p.expect("Power & battery");
    p.send("j\n");p.expect("Charging: advanced");p.send("j\n");p.expect("Charging snapshot");
    p.send("\033");p.expect("Charging: advanced");p.send("\n");p.expect("Charging snapshot");
    p.tap("< Back [Esc]");p.expect("Charging: advanced");p.tap("< Back [Esc]");p.expect("Audio & camera");
    const auto [x,y]=p.locate("Power & battery",5);p.tap_at(x,y);p.tap_at(x,y,false);p.expect("Charging: advanced");
    p.send("\033");p.resize(80,24);p.expect("Power & battery");
    p.send("\033OP");p.expect("NAVIGATION");p.send("\033");p.expect("Power & battery");
    p.resize(50,12);p.expect("Terminal too small");p.resize(64,18);p.expect("Categories");p.resize(120,36);
    p.send("q");assert(p.finish()==0);
}
void observations() {
    Pty p(fixture("status"));p.expect("General device status");p.send("\n");p.expect("SYNTHETIC");p.expect("cycles: 0");p.expect("-42");p.expect("Not reported");
    p.tap("JSON [J]");p.expect("Raw JSON");p.tap("Save [S]");p.expect("Saving is disabled");
    p.tap("History [H]");p.expect("Session history");p.send("\n");p.expect("SYNTHETIC");p.send("\033\033");p.expect("Raw JSON");
    p.send("q");assert(p.finish()==0);
}
void forms() {
    Pty p(fixture("charge.mode"),80,24);p.expect("Request charging policy");p.send("\n");p.expect("Charging policy");
    p.send("\n");p.expect("Daily 70");p.send("j\n");p.expect("Value:");p.send("\n");p.expect("Parameters");
    p.send("\033[19~");p.expect("Review the request");p.tap("Run [Enter]");p.expect("Changes are disabled");
    p.tap("Preview [F7]");p.expect("Preview charging policy");p.send("\n");p.expect("Parameters");p.send("\033[19~\n");p.expect("requested mode");
    p.send("\033\033\033\033");p.expect("Review the request");p.send("q");assert(p.finish()==0);
}
void background_reads() {
    Pty p(fixture("charge.status"));p.send("\n");p.expect("Reading…");p.send("\033");p.expect("Charging status");
    p.tap("Reading… [F9]");p.expect("Stop");p.tap("Stop");p.expect("Cancelled");
    p.tap("History [H]");p.expect("Session history");p.send("\n");p.expect("Cancelled");
    p.send("q");assert(p.finish()==0);
}
void touch_editor() {
    Pty p(fixture("charge.limit"),80,24);p.tap("Continue [Enter]");p.tap("End threshold (%)",5);p.expect("Clear");
    p.tap("+10");p.tap("+10");p.tap("+10");p.expect("> 100");
    p.send("0\n");p.expect("Request refused");p.send("\02590\n");p.expect("Parameters");p.expect("90");
    p.tap("End threshold (%)",5);p.tap("Clear");p.tap_at(38,12);p.tap_at(38,14);p.tap("Accept [Enter]");p.expect("Parameters");p.expect("80");
    p.tap("Review [F8]");p.expect("Review the request");p.tap("Run [Enter]");p.expect("Changes are disabled");
    p.send("q");assert(p.finish()==0);
}
void touch_search() {
    Pty p(fixture(),80,24);p.tap("Search [/]");p.expect("Search all actions");
    p.tap_at(49,10);p.tap_at(13,12);p.tap_at(50,14);p.expect("> usb");p.tap("Show results [Enter]");p.expect("Search results: usb");p.expect("USB");
    p.send("\033");p.expect("Power & battery");p.send("/zznomatching\n");p.expect("No matching action");p.send("\033");p.expect("Power & battery");
    p.send("q");assert(p.finish()==0);
}
void confirmations_and_paths() {
    Pty p(fixture("update.apply"),80,24);p.tap("Continue [Enter]");p.expect("Review the request");p.tap("Confirmation:");p.expect("Type exactly: APPLY");
    p.send("APPLY\n");p.expect("Review the request");p.tap("Run [Enter]");p.expect("Changes are disabled");p.send("q");assert(p.finish()==0);
    Pty file(fixture("snapshot.save"));file.send("\n\n\025session Şarj file.json\n");file.expect("Parameters");file.expect("session Şarj file.json");file.send("q");assert(file.finish()==0);
}
void wireless_shortcuts() {
    Pty p(fixture("connections"),80,24);p.expect("Bluetooth diagnostic report");p.tap("Wi-Fi diagnostic report",5);p.tap("Continue [Enter]");p.expect("Wi-Fi");p.send("\033[19~");p.expect("Review the request");p.expect("Wi-Fi");p.tap("Run [Enter]");p.expect("Changes are disabled");p.send("q");assert(p.finish()==0);
}
void drag_and_resize() {
    {Pty burst(fixture(),80,24);burst.expect("Power & battery");burst.send("\033[<0;8;8M\033[<0;8;8m");burst.expect("Charging: advanced");burst.send("q");assert(burst.finish()==0);}
    Pty p(fixture(),80,24);p.drain(260);
    p.send("\033[<0;8;7M\033[<32;8;5M\033[<0;8;5m");p.expect("Categories");assert(p.screen().find("Charging: advanced")==std::string::npos);assert(p.screen().find("Device overview")==std::string::npos);
    p.send("\033[<0;8;7M");p.resize(120,36);p.send("\033[<0;8;7m");p.expect("Categories");assert(p.screen().find("Charging: advanced")==std::string::npos);
    p.send("q");assert(p.finish()==0);
}
void readonly_and_signals() {
    Pty p({"--read-only","--language","en","--start","charge.mode"});p.expect("READ ONLY");p.send("\n\033[19~\n");p.expect("Changes are disabled");p.signal(SIGTERM);assert(p.finish()==143);
    Pty tr({"--fixture","tests/fixtures/tui.json"});tr.expect("SİMÜLASYON");tr.send("/ŞARJ\n");tr.expect("Arama sonuçları: ŞARJ");tr.send("q");assert(tr.finish()==0);
    const std::string path="/tmp/uke-tui-invalid-"+std::to_string(getpid())+".json";
    {std::ofstream file(path);file<<"{\"schema\":\"org.uke.control.tui-fixture.v1\",\"results\":{},\"command\":\"/bin/true\"}";}
    Pty invalid({"--fixture",path});invalid.expect("Unknown fixture field");assert(invalid.finish()==1);unlink(path.c_str());
}
int main(){assert(setlocale(LC_ALL,"C.UTF-8"));menu_coverage();navigation();observations();forms();background_reads();touch_editor();touch_search();confirmations_and_paths();wireless_shortcuts();drag_and_resize();readonly_and_signals();std::cout<<"124-action coverage; PTY keyboard, SGR touch, drag, forms, confirmation, search, ownership, resize and terminal restoration passed\n";}
