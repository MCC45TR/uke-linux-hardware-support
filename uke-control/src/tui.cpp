// SPDX-License-Identifier: MIT
#include "client.hpp"
#include "tui-text.hpp"
#include "tui-menu.hpp"
#include "version.hpp"
#include <json/json.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <clocale>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#define NCURSES_NOMACROS 1
#include <ncursesw/curses.h>

namespace {
namespace client=uke::client;
namespace text=uke::tui_text;
using Clock=std::chrono::steady_clock;
using Value=Json::Value;
volatile sig_atomic_t interrupted=0;
void stop(int signal){interrupted=signal;}
class SignalHandler {
    int signal_;
    struct sigaction original_{};
public:
    SignalHandler(int signal,void(*handler)(int)):signal_(signal){struct sigaction next{};next.sa_handler=handler;sigemptyset(&next.sa_mask);sigaction(signal_,&next,&original_);}
    ~SignalHandler(){sigaction(signal_,&original_,nullptr);}
};
struct Options {client::Language language=client::Language::turkish;bool readonly=false,color=true;std::string fixture,start="home";};
struct FixtureValue {std::string output,error;int status=0;unsigned delay=0;};
std::map<std::string,FixtureValue> load_fixture(const std::string& path) {
    const int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
    if(fd<0)throw std::runtime_error("Cannot read fixture");
    struct Close{int fd;~Close(){close(fd);}} close_file{fd};struct stat st{};
    if(fstat(fd,&st)||!S_ISREG(st.st_mode))throw std::runtime_error("Fixture must be a regular file");
    std::string bytes;std::array<char,8192> buffer{};
    for(;;){const auto n=read(fd,buffer.data(),buffer.size());if(n<0&&errno==EINTR)continue;if(n<0)throw std::runtime_error("Cannot read fixture");if(!n)break;bytes.append(buffer.data(),size_t(n));if(bytes.size()>1024*1024)throw std::runtime_error("Fixture exceeds 1 MiB");}
    Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["allowComments"]=false;builder["failIfExtra"]=true;builder["stackLimit"]=40;
    const std::unique_ptr<Json::CharReader> parser(builder.newCharReader());Value root;std::string error;
    if(!parser->parse(bytes.data(),bytes.data()+bytes.size(),&root,&error)||!root.isObject()||root["schema"]!="org.uke.control.tui-fixture.v1"||!root["results"].isObject())throw std::runtime_error("Invalid TUI fixture schema");
    for(const auto& key:root.getMemberNames())if(key!="schema"&&key!="results")throw std::runtime_error("Unknown fixture field");
    std::map<std::string,FixtureValue> result;
    for(const auto& id:root["results"].getMemberNames()) {
        (void)client::action(id);const auto& item=root["results"][id];if(!item.isObject())throw std::runtime_error("Invalid fixture result");
        for(const auto& key:item.getMemberNames())if(key!="data"&&key!="stdout"&&key!="stderr"&&key!="status"&&key!="delay_ms")throw std::runtime_error("Unknown fixture result field");
        if(item.isMember("data")&&item.isMember("stdout"))throw std::runtime_error("Fixture data/stdout are mutually exclusive");
        if(item.isMember("stdout")&&!item["stdout"].isString())throw std::runtime_error("Fixture stdout must be text");
        if(item.isMember("stderr")&&!item["stderr"].isString())throw std::runtime_error("Fixture stderr must be text");
        const auto status=item.get("status",0),delay=item.get("delay_ms",0);
        if(!status.isInt()||status.asInt()<0||status.asInt()>255||!delay.isUInt()||delay.asUInt()>3000)throw std::runtime_error("Invalid fixture status/delay");
        Json::StreamWriterBuilder writer;writer["indentation"]="";writer["emitUTF8"]=true;
        const auto output=item.isMember("data")?Json::writeString(writer,item["data"]):item.get("stdout","").asString();
        result[id]={output,item.get("stderr","").asString(),status.asInt(),delay.asUInt()};
    }
    return result;
}
class Terminal {
    termios original_{};SCREEN* screen_=nullptr;
    std::vector<std::unique_ptr<SignalHandler>> signals_;
public:
    bool color=false;
    explicit Terminal(bool colors) {
        if(!isatty(0)||!isatty(1))throw std::runtime_error("TUI requires an interactive terminal; use 'uke-control actions --json' for metadata");
        const char* term=getenv("TERM");if(!term||!*term||std::string_view(term)=="dumb")throw std::runtime_error("Set TERM to a supported interactive terminal");
        if(tcgetattr(0,&original_))throw std::runtime_error("Cannot save terminal mode");
        screen_=newterm(nullptr,stdout,stdin);if(!screen_)throw std::runtime_error("Cannot initialize terminal");
        cbreak();noecho();keypad(stdscr,true);curs_set(0);set_escdelay(30);
        if(colors&&has_colors()){start_color();use_default_colors();init_pair(1,COLOR_CYAN,-1);init_pair(2,COLOR_YELLOW,-1);init_pair(3,COLOR_RED,-1);init_pair(4,COLOR_WHITE,COLORS>=256?24:COLOR_BLUE);color=true;}
        mousemask(BUTTON1_PRESSED|BUTTON1_RELEASED|BUTTON4_PRESSED|BUTTON5_PRESSED|REPORT_MOUSE_POSITION,nullptr);mouseinterval(0);
        pointer(true);
        for(int signal:{SIGINT,SIGTERM,SIGHUP})signals_.push_back(std::make_unique<SignalHandler>(signal,stop));
    }
    void pointer(bool enabled){if(has_mouse()){fputs(enabled?"\033[?1003l\033[?1002h\033[?1006h":"\033[?1002l\033[?1003l\033[?1006l",stdout);fflush(stdout);}}
    void pause(){pointer(false);def_prog_mode();endwin();tcsetattr(0,TCSANOW,&original_);}
    void resume(){reset_prog_mode();pointer(true);winsize size{};if(!ioctl(0,TIOCGWINSZ,&size)&&size.ws_row&&size.ws_col)resize_term(size.ws_row,size.ws_col);clearok(stdscr,true);curs_set(0);}
    ~Terminal(){if(screen_){pointer(false);endwin();delscreen(screen_);tcsetattr(0,TCSANOW,&original_);}}
};
struct Entry {client::Result result;client::Values values;unsigned sequence=0;};
enum class View {menu,guide,form,choices,choice_info,review,result,help,history,edit,search,find,save,confirm};
enum class Command {back,next,home,search,history,help,active,up,down,item,field,choice,keyboard,insert,shift,symbols,space,erase,clear,left,right,step,inspect,preview,details,favorite,raw,find,save,cancel};
struct Hit {
    int y,x,h,w;Command command;size_t index=0;std::string value={};
    bool contains(int py,int px)const{return py>=y&&py<y+h&&px>=x&&px<x+w;}
    bool same(const Hit& other)const{return command==other.command&&index==other.index&&value==other.value;}
};
struct Page {
    View view=View::menu;
    std::string menu="home",id,filter,phrase;
    size_t selected=0,top=0,field=0,choice=0,caret=0;
    int scroll=0;
    bool keyboard=false,shift=false,symbols=false,details=false,raw=false;
    std::wstring input;
    std::optional<client::Plan> plan;
    std::shared_ptr<Entry> entry;
};
class App {
    Options options_;
    Terminal& terminal_;
    std::map<std::string,FixtureValue> fixture_;
    bool simulation_=false,running_=true;
    Page page_;
    std::vector<Page> back_;
    std::map<std::string,client::Values> drafts_;
    std::set<std::string> favorites_;
    std::vector<std::shared_ptr<Entry>> history_;
    std::shared_ptr<Entry> active_;
    std::unique_ptr<client::Query> query_;
    std::optional<FixtureValue> fake_;
    Clock::time_point fake_end_{};
    unsigned sequence_=0,generation_=0;
    int rows_=24,cols_=80,body_y_=5,body_h_=13,row_h_=2,max_scroll_=0;
    std::string notice_,find_;
    std::vector<Hit> hits_;
    std::optional<Hit> pressed_;
    bool dragging_=false;
    int press_y_=0,press_x_=0,drag_y_=0;
    unsigned press_generation_=0;
    Clock::time_point touch_ready_{};
    std::string tr(const char* en,const char* turkish)const{return options_.language==client::Language::turkish?turkish:en;}
    const client::Action& current()const{return client::action(page_.id);}
    bool busy()const{return query_||fake_;}
    bool writable()const{return !options_.readonly&&!simulation_;}
    int accent()const{return A_BOLD|(terminal_.color?COLOR_PAIR(1):0);}
    int warning()const{return A_BOLD|(terminal_.color?COLOR_PAIR(2):0);}
    void line(int y,int x,int width,const std::string& value,int attributes=0)const {
        if(y<0||y>=rows_||x<0||x>=cols_||width<=0)return;
        const auto safe=text::clipped(text::wide(value),std::min(width,cols_-x));
        attron(attributes);mvaddnwstr(y,x,safe.data(),int(safe.size()));attroff(attributes);
    }
    void fill(int y,int x,int height,int width,int attributes)const {
        attron(attributes);for(int row=0;row<height&&y+row<rows_;++row)if(y+row>=0)mvhline(y+row,x,' ',std::min(width,cols_-x));attroff(attributes);
    }
    void button(int y,int x,int h,int w,const std::string& label,Command command,size_t index=0,const std::string& value={},bool primary=false) {
        if(h<=0||w<2)return;
        const int attr=primary?A_REVERSE|A_BOLD:terminal_.color?COLOR_PAIR(4):A_REVERSE;
        fill(y,x,h,w,attr);
        const auto clipped=text::clipped(text::wide(label),w-2);
        line(y+(h-1)/2,x+std::max(1,(w-text::cells(clipped))/2),w-2,text::utf8(clipped),attr);
        hits_.push_back({y,x,h,w,command,index,value});
    }
    void invalidate(){++generation_;pressed_.reset();dragging_=false;}
    void push(Page next) {
        if(back_.size()>=64)throw std::runtime_error("Navigation limit reached; go back first");
        back_.push_back(page_);page_=std::move(next);invalidate();notice_.clear();
    }
    void back() {
        if(back_.empty()){notice_=tr("At Categories. Use Quit or q to exit.","Kategorilerdesiniz. Çıkmak için Çıkış veya q kullanın.");return;}
        page_=std::move(back_.back());back_.pop_back();invalidate();notice_.clear();
    }
    client::Values& draft(const std::string& id){if(!drafts_.contains(id))drafts_[id]=client::defaults(client::action(id));return drafts_.at(id);}
    std::vector<uke::tui_menu::Item> items()const {
        if(page_.menu=="search"||page_.menu=="favorites") {
            std::vector<uke::tui_menu::Item> result;
            for(const auto& id:client::search(page_.filter,{},options_.language))if(page_.menu!="favorites"||favorites_.contains(id))result.push_back({id,true});
            return result;
        }
        return uke::tui_menu::items(page_.menu);
    }
    std::string menu_title()const {
        if(page_.menu=="search")return tr("Search results: ","Arama sonuçları: ")+page_.filter;
        if(page_.menu=="favorites")return tr("Session favorites","Oturum favorileri");
        return uke::tui_menu::menu(page_.menu).title.get(options_.language);
    }
    std::string breadcrumb()const {
        std::string result=tr("Categories","Kategoriler");
        if(page_.menu!="home"&&page_.menu!="search"&&page_.menu!="favorites") {
            std::vector<std::string> trail;auto id=page_.menu;
            while(id!="home"){const auto& node=uke::tui_menu::menu(id);trail.push_back(node.title.get(options_.language));id=node.parent;}
            for(auto it=trail.rbegin();it!=trail.rend();++it)result+=" / "+*it;
        } else if(page_.menu!="home")result+=" / "+menu_title();
        if(page_.view!=View::menu&&!page_.id.empty())result+=" / "+client::action(page_.id).title.get(options_.language);
        return result;
    }
    void panel(const std::string& title) {
        attron(accent());mvhline(4,0,ACS_HLINE,cols_);mvhline(rows_-6,0,ACS_HLINE,cols_);
        mvvline(4,0,ACS_VLINE,rows_-9);mvvline(4,cols_-1,ACS_VLINE,rows_-9);
        mvaddch(4,0,ACS_ULCORNER);mvaddch(4,cols_-1,ACS_URCORNER);mvaddch(rows_-6,0,ACS_LLCORNER);mvaddch(rows_-6,cols_-1,ACS_LRCORNER);attroff(accent());
        line(4,2,cols_-4," "+title+" ",accent());
    }
    void paragraph(int y,int h,const std::string& value) {
        const auto lines=text::wrap(value,cols_-6);max_scroll_=std::max(0,int(lines.size())-h);page_.scroll=std::clamp(page_.scroll,0,max_scroll_);
        for(int row=0;row<h&&row+page_.scroll<int(lines.size());++row)line(y+row,3,cols_-6,text::utf8(lines[size_t(row+page_.scroll)]));
        if(max_scroll_)line(rows_-6,cols_-19,16,std::to_string(page_.scroll+1)+" / "+std::to_string(lines.size()),accent());
    }
    std::string access(const client::Action& a)const {
        if(a.access==client::Access::root)return geteuid()==0?tr("Root (current process)","Root (mevcut süreç)"):tr("Root; sudo authorizes only this action","Root; sudo yalnız bu işlemi yetkilendirir");
        if(a.access==client::Access::active_session)return tr("Active local session","Etkin yerel oturum");
        return tr("Current user","Mevcut kullanıcı");
    }
    std::string details(const client::Action& a)const {
        std::string value=a.description.get(options_.language)+"\n\n"+tr("What it does\n","İşlemin etkisi\n")+client::effect_description(a.effect).get(options_.language);
        if(a.tests_package)value+="\n\n"+tr("Requires the optional uke-control-tests package.","İsteğe bağlı uke-control-tests paketi gerekir.");
        if(!a.parameters.empty())value+="\n\n"+tr("Continue to choose parameters. Form defaults are requests, not current device readings.","Parametreleri seçmek için devam edin. Form varsayılanları istektir; cihazın mevcut okuması değildir.");
        if(!writable()&&a.effect!=client::Effect::read)value+="\n\n"+tr("Changes are disabled. You can still inspect the form and request.","Değişiklikler kapalı. Formu ve isteği inceleyebilirsiniz.");
        if(page_.details)value+="\n\n"+tr("Access: ","Yetki: ")+access(a)+"\nID: "+a.id;
        return value;
    }
    std::string choice_label(const client::Parameter& p,const std::string& value)const {
        for(const auto& c:p.choices)if(c.value==value)return c.title.get(options_.language);
        return value.empty()?tr("Required — select a value","Gerekli — bir değer seçin"):value;
    }
    void header() {
        const auto mode=simulation_?tr("SIMULATION","SİMÜLASYON"):options_.readonly?tr("READ ONLY","SALT OKUNUR"):"";
        line(0,2,cols_-4,"UKE CONTROL "+std::string(uke::application_version)+(mode.empty()?"":"   ·   "+mode),accent());
        if(busy())button(0,cols_-25,1,23,tr("Reading… [F9]","Okunuyor… [F9]"),Command::active);
        const std::array<std::string,4> labels{cols_<100?tr("Categories","Kategoriler"):tr("Categories [Home]","Kategoriler [Home]"),tr("Search [/]","Ara [/]"),tr("History [H]","Geçmiş [H]"),tr("Help [F1]","Yardım [F1]")};
        const std::array<Command,4> commands{Command::home,Command::search,Command::history,Command::help};
        for(int i=0;i<4;++i){const int x=1+i*(cols_-2)/4,end=1+(i+1)*(cols_-2)/4;button(1,x,2,end-x-1,labels[size_t(i)],commands[size_t(i)]);}
        line(3,2,cols_-4,breadcrumb());
    }
    std::string next_label()const {
        switch(page_.view) {
        case View::menu:case View::history:return tr("Open [Enter]","Aç [Enter]");
        case View::guide:return current().parameters.empty()&&current().effect==client::Effect::read?tr("Read [Enter]","Oku [Enter]"):tr("Continue [Enter]","Devam [Enter]");
        case View::form:return tr("Review [F8]","İncele [F8]");
        case View::choices:return tr("Details [Enter]","Açıklama [Enter]");
        case View::choice_info:return tr("Choose [Enter]","Seç [Enter]");
        case View::review:return tr("Run [Enter]","Çalıştır [Enter]");
        case View::result:case View::help:return tr("Back [Enter]","Geri [Enter]");
        case View::search:return tr("Show results [Enter]","Sonuçlar [Enter]");
        default:return tr("Accept [Enter]","Kabul et [Enter]");
        }
    }
    void footer() {
        const std::string message=!notice_.empty()?notice_:busy()?tr("Reading in background — Stop is available on the result page.","Arka planda okunuyor — Durdur düğmesi sonuç ekranında."):"";
        line(rows_-5,2,cols_-4,message,warning());
        const int back_width=std::max(15,(cols_-4)/4),arrow_width=7;
        button(rows_-4,1,3,back_width,back_.empty()?tr("Quit [q]","Çıkış [q]"):tr("< Back [Esc]","< Geri [Esc]"),Command::back);
        button(rows_-4,back_width+2,3,arrow_width,"↑",Command::up);
        button(rows_-4,back_width+arrow_width+3,3,arrow_width,"↓",Command::down);
        const int next_x=back_width+2*arrow_width+4;
        button(rows_-4,next_x,3,cols_-next_x-1,next_label(),Command::next,0,{},true);
        line(rows_-1,2,cols_-4,tr("↑↓ select/scroll · Enter forward · Esc back · Tap buttons · F4 TR/EN","↑↓ seç/kaydır · Enter ileri · Esc geri · Düğmelere dokunun · F4 TR/EN"));
    }
    void list_position(size_t count,int capacity) {
        if(!count){page_.selected=page_.top=0;return;}
        page_.selected=std::min(page_.selected,count-1);page_.top=std::min(page_.top,count-1);
        if(page_.selected<page_.top)page_.top=page_.selected;
        if(page_.selected>=page_.top+size_t(capacity))page_.top=page_.selected-size_t(capacity)+1;
        page_.top=std::min(page_.top,count>size_t(capacity)?count-size_t(capacity):0);
        line(rows_-6,std::max(2,cols_-20),17,std::to_string(page_.selected+1)+" / "+std::to_string(count),accent());
    }
    void list_row(int y,const std::string& title,const std::string& subtitle,bool selected,Command command,size_t index) {
        const int attributes=selected?A_REVERSE|A_BOLD:0;
        fill(y,2,row_h_,cols_-4,attributes);
        line(y,3,cols_-7,(selected?"> ":"  ")+title,attributes);
        if(row_h_>1)line(y+1,5,cols_-9,subtitle,selected?A_REVERSE:A_DIM);
        hits_.push_back({y,2,row_h_,cols_-4,command,index,{}});
    }
    void draw_menu() {
        panel(menu_title());const auto list=items();const int capacity=std::max(1,body_h_/row_h_);list_position(list.size(),capacity);
        if(list.empty()){paragraph(body_y_,body_h_,tr("No matching action. Go back to change the search.","Eşleşen işlem yok. Aramayı değiştirmek için geri dönün."));return;}
        for(int row=0;row<capacity&&page_.top+size_t(row)<list.size();++row) {
            const auto index=page_.top+size_t(row);const auto& item=list[index];
            if(item.action) {
                const auto& a=client::action(item.id);const auto prefix=a.effect==client::Effect::read?tr("Read · ","Oku · "):tr("Request · ","İstek · ");
                list_row(body_y_+row*row_h_,a.title.get(options_.language)+(favorites_.contains(a.id)?" *":""),prefix+a.description.get(options_.language),index==page_.selected,Command::item,index);
            } else {const auto& m=uke::tui_menu::menu(item.id);list_row(body_y_+row*row_h_,m.title.get(options_.language)+"  ›",m.description.get(options_.language),index==page_.selected,Command::item,index);}
        }
    }
    void action_tools() {
        const int width=(cols_-6)/3;
        button(body_y_,2,2,width,tr("Details [D]","Ayrıntı [D]"),Command::details);
        button(body_y_,width+3,2,width,tr("Current [F5]","Mevcut [F5]"),Command::inspect);
        button(body_y_,2*width+4,2,cols_-2*width-6,tr("Preview [F7]","Önizle [F7]"),Command::preview);
    }
    void draw_guide() {
        panel(current().title.get(options_.language));action_tools();
        paragraph(body_y_+3,body_h_-3,details(current()));
    }
    void draw_form() {
        panel(tr("Parameters — ","Parametreler — ")+current().title.get(options_.language));
        const auto& a=current();const size_t count=a.parameters.size();page_.selected=page_.field;
        const int capacity=std::max(1,(body_h_-3)/row_h_);list_position(count+1,capacity);page_.field=page_.selected;
        for(int row=0;row<capacity&&page_.top+size_t(row)<=count;++row) {
            const auto at=page_.top+size_t(row);const bool review=at==count;
            const auto title=review?tr("Review this request →","Bu isteği incele →"):a.parameters[at].title.get(options_.language);
            const auto value=review?tr("Check values and effect before running","Çalıştırmadan önce değerleri ve etkiyi denetleyin"):choice_label(a.parameters[at],draft(a.id).at(a.parameters[at].id));
            list_row(body_y_+row*row_h_,title,value,at==page_.field,Command::field,at);
        }
        if(page_.field<count) {
            const auto& p=a.parameters[page_.field];line(rows_-9,3,cols_-6,p.description.get(options_.language));
            line(rows_-8,3,cols_-6,p.kind==client::Kind::integer?tr("Allowed range: ","Geçerli aralık: ")+std::to_string(p.minimum)+"–"+std::to_string(p.maximum):tr("Tap a field or press Enter to edit. Defaults are requests.","Düzenlemek için alana dokunun veya Enter'a basın. Varsayılanlar istektir."),A_DIM);
        }
    }
    void draw_choices() {
        const auto& p=current().parameters[page_.field];panel(p.title.get(options_.language));
        page_.selected=page_.choice;const int capacity=std::max(1,body_h_/row_h_);list_position(p.choices.size(),capacity);page_.choice=page_.selected;
        for(int row=0;row<capacity&&page_.top+size_t(row)<p.choices.size();++row) {
            const auto at=page_.top+size_t(row);const auto& c=p.choices[at];list_row(body_y_+row*row_h_,c.title.get(options_.language),c.description.get(options_.language),at==page_.choice,Command::choice,at);
        }
    }
    std::string confirmation()const {
        if(!page_.plan||!page_.plan->confirmation_required)return {};
        const auto& p=*page_.plan;
        if(p.effect==client::Effect::maintenance)return p.action_id;
        if(p.effect==client::Effect::publish)return p.values.at("repository");
        if(p.effect==client::Effect::packages)return tr("APPLY","UYGULA");
        return {};
    }
    void draw_review() {
        panel(tr("Review the request","İsteği incele"));action_tools();const auto& a=current();
        std::string value=a.title.get(options_.language)+"\n\n"+client::effect_description(a.effect).get(options_.language)+"\n\n";
        for(const auto& p:a.parameters)value+=p.title.get(options_.language)+": "+choice_label(p,page_.plan->values.at(p.id))+"\n";
        if(a.presentation==client::Presentation::terminal)value+="\n"+tr("The command continues in the normal terminal. Existing system authentication and backend questions still apply.","Komut normal terminalde sürer. Mevcut sistem yetkilendirmesi ve backend soruları geçerlidir.");
        if(page_.details)value+="\n\n"+tr("Access: ","Yetki: ")+access(a)+"\n\n"+tr("Exact arguments (no shell):\n","Tam argümanlar (kabuk yok):\n")+client::display_command(page_.plan->argv);
        if(!writable()&&a.effect!=client::Effect::read)value+="\n\n"+tr("Changes are disabled in this mode.","Bu kipte değişiklikler kapalı.");
        const auto phrase=confirmation();paragraph(body_y_+3,std::max(1,body_h_-3-(phrase.empty()?0:3)),value);
        if(!phrase.empty())button(rows_-9,2,2,cols_-4,tr("Confirmation: ","Onay: ")+(page_.phrase.empty()?tr("[C] type ","[C] yazın ")+phrase:page_.phrase),Command::item);
    }
    std::string result_text()const {
        if(!page_.entry)return tr("No result yet.","Henüz sonuç yok.");
        const auto& r=page_.entry->result;
        std::string value=client::action(r.action_id).title.get(options_.language)+"\n";
        value+=r.running?tr("Reading…","Okunuyor…"):r.cancelled?tr("Cancelled","İptal edildi"):r.timed_out?tr("Read timed out","Okuma süresi aşıldı"):r.exit_status==0?tr("Completed","Tamamlandı"):tr("Not completed","Tamamlanmadı");
        if(!r.running)value+=" · "+tr("Exit status: ","Çıkış kodu: ")+std::to_string(r.exit_status)+" · "+std::to_string(r.elapsed_ms)+" ms";
        value+="\n\n";
        if(r.truncated)value+=tr("Output is partial: the output limit was reached.\n\n","Çıktı kısmidir: çıktı sınırına ulaşıldı.\n\n");
        if(!r.output.empty())value+=client::format_output(r.output,options_.language,page_.raw);
        else if(r.running)value+=simulation_?tr("Synthetic read is pending; no command was started.","Örnek okuma bekleniyor; komut başlatılmadı."):tr("Waiting for the read query. You can go back while it runs.","Okuma sorgusu bekleniyor. Çalışırken geri dönebilirsiniz.");
        else if(client::action(r.action_id).presentation==client::Presentation::terminal)value+=tr("Output was displayed in the normal terminal. This receipt records the actual command exit.","Çıktı normal terminalde gösterildi. Bu makbuz gerçek komut çıkışını kaydeder.");
        if(!r.error.empty())value+="\n\n"+tr("Details:\n","Ayrıntılar:\n")+r.error;
        return value;
    }
    void draw_result() {
        panel(page_.raw?tr("Raw JSON / output","Ham JSON / çıktı"):tr("Result","Sonuç"));const int w=(cols_-7)/4;
        button(body_y_,2,2,w,page_.raw?tr("Readable [J]","Okunabilir [J]"):"JSON [J]",Command::raw);
        button(body_y_,w+3,2,w,tr("Find [/]","Bul [/]"),Command::find);
        button(body_y_,2*w+4,2,w,tr("Save [S]","Kaydet [S]"),Command::save);
        button(body_y_,3*w+5,2,cols_-3*w-7,page_.entry&&page_.entry->result.running?tr("Stop","Durdur"):tr("Current [F5]","Mevcut [F5]"),page_.entry&&page_.entry->result.running?Command::cancel:Command::inspect);
        paragraph(body_y_+3,body_h_-3,result_text());
    }
    std::string help()const {
        return tr("NAVIGATION\nChoose a category, an action, then Continue. Every action has a guide. The bottom Back and forward buttons always name the next step. Esc goes back without cancelling a background read. Home opens Categories; H opens session history; / searches all 124 actions. Up/down or j/k select; Page Up/Down scroll; Enter advances. F4 switches Turkish/English. Space marks a favorite on an action guide; F2 opens favorites. q quits outside text entry.\n\nTOUCH\nTap once to open a row or button. Drag vertically or use the large arrow buttons to scroll. A release after dragging does not open a row. Fields, options, previews, results and confirmation are tappable. Text fields have a built-in keyboard; numeric fields also have bounded step buttons. Physical keyboard editing remains available. Terminal mouse reporting must be enabled (Konsole/xterm compatible). Font size determines the physical target size. Use at least 80×24 for comfortable touch input. System passwords and existing backend terminal questions use the terminal/system input method.\n\nREQUESTS\nParameter defaults are requests, not device readings. Every option explains its effect. F5 opens related current state; F7 opens a separate preview when defined. F8 reviews a form. Details shows access and the exact argument vector; no shell is used. Changes require a separate Run action. Storage maintenance, packages and publishing also require typed confirmation. Existing backend checks and confirmations remain authoritative. Run the application as your normal session user.\n\nRESULTS & PRIVACY\nReads stay owned until actual process exit; Stop cancels only a read. Going back never cancels it. Writes restore the normal terminal and have no application timeout. J toggles JSON; / searches output; n finds the next match. S saves a new private 0600 receipt without overwriting. History is bounded and stays in memory; opening history never replays commands. Reports can contain private data: review before sharing. Unknown data is not zero or false. A command exit does not establish physical hardware support.\n\nSHARED CLIENT\nThe C++ client catalog, validation, plans, query ownership and receipts are shared with future GTK/QML frontends. Action IDs and API v1 remain stable. Simulation executes no commands and saves no files; read-only mode disables changes.",
        "GEZİNME\nKategori, işlem ve Devam sırasını izleyin. Her işlemin açıklaması vardır. Alttaki Geri ve ileri düğmeleri sonraki adımı yazar. Esc arka plan okumasını iptal etmeden geri döner. Home kategorileri, H oturum geçmişini açar; / tüm 124 işlemde arar. Yukarı/aşağı veya j/k seçer; Page Up/Down kaydırır; Enter ilerler. F4 Türkçe/İngilizce değiştirir. İşlem açıklamasında Boşluk favoriye ekler; F2 favorileri açar. Metin girişi dışında q çıkar.\n\nDOKUNMATİK\nSatıra veya düğmeye bir kez dokunun. Kaydırmak için dikey sürükleyin veya büyük ok düğmelerini kullanın. Sürükleme sonunda satır açılmaz. Alanlar, seçenekler, önizlemeler, sonuçlar ve onay dokunulabilirdir. Metin alanında yerleşik klavye; sayı alanında ayrıca sınırlı artır/azalt düğmeleri bulunur. Fiziksel klavye kullanılabilir. Terminalde fare raporlaması açık olmalıdır (Konsole/xterm uyumlu). Fiziksel hedef boyutunu yazı boyutu belirler. Rahat dokunmatik giriş için en az 80×24 kullanın. Sistem parolaları ve mevcut backend terminal soruları terminalin/sistemin giriş yöntemini kullanır.\n\nİSTEKLER\nParametre varsayılanları cihaz okuması değil, istektir. Her seçeneğin etkisi açıklanır. F5 ilgili mevcut durumu, F7 varsa ayrı önizlemeyi açar. F8 formu incelemeye götürür. Ayrıntı yetkiyi ve tam argümanları gösterir; kabuk kullanılmaz. Değişiklikler ayrı Çalıştır işlemi gerektirir. Depolama bakımı, paketler ve yayımlama ayrıca yazılı onay ister. Mevcut backend denetimleri ve onayları yetkilidir. Uygulamayı normal oturum kullanıcısı olarak çalıştırın.\n\nSONUÇLAR VE GİZLİLİK\nOkumalar gerçek süreç çıkışına kadar sahiplenilir; Durdur yalnız okumayı iptal eder. Geri dönmek iptal etmez. Yazmalar normal terminali geri yükler ve uygulama süre aşımı kullanmaz. J JSON'u açar/kapatır; / çıktıda arar; n sonraki eşleşmeyi bulur. S üzerine yazmadan yeni özel 0600 makbuz kaydeder. Geçmiş sınırlıdır ve bellekte kalır; geçmişi açmak komut tekrarlamaz. Raporlar özel veri içerebilir; paylaşmadan önce inceleyin. Bilinmeyen veri sıfır veya yanlış değildir. Komut çıkışı fiziksel donanım desteğini kanıtlamaz.\n\nORTAK İSTEMCİ\nC++ istemci kataloğu, doğrulama, planlar, sorgu sahipliği ve makbuzlar gelecekteki GTK/QML ön yüzleriyle ortaktır. İşlem kimlikleri ve API v1 sabittir. Simülasyon komut çalıştırmaz ve dosya kaydetmez; salt okunur kip değişiklikleri kapatır.");
    }
    void draw_history() {
        panel(tr("Session history — view only","Oturum geçmişi — yalnız görüntüle"));const int capacity=std::max(1,body_h_/row_h_);list_position(history_.size(),capacity);
        if(history_.empty()){paragraph(body_y_,body_h_,tr("No completed action yet. History stays in memory.","Henüz tamamlanmış işlem yok. Geçmiş bellekte kalır."));return;}
        for(int row=0;row<capacity&&page_.top+size_t(row)<history_.size();++row) {
            const auto index=page_.top+size_t(row);const auto& e=*history_[index];list_row(body_y_+row*row_h_,"#"+std::to_string(e.sequence)+" "+client::action(e.result.action_id).title.get(options_.language),tr("Exit status: ","Çıkış kodu: ")+std::to_string(e.result.exit_status)+" · "+std::to_string(e.result.elapsed_ms)+" ms",index==page_.selected,Command::item,index);
        }
    }
    bool input_view()const{return page_.view==View::edit||page_.view==View::search||page_.view==View::find||page_.view==View::save||page_.view==View::confirm;}
    bool numeric()const{return page_.view==View::edit&&current().parameters[page_.field].kind==client::Kind::integer;}
    void keyboard(int top,int h) {
        const int kh=std::clamp(h/5,1,3);
        if(numeric()) {
            for(int row=0;row<3;++row)for(int col=0;col<3;++col){const auto digit=std::to_string(row*3+col+1);const int x=2+col*(cols_-4)/3,end=2+(col+1)*(cols_-4)/3;button(top+row*kh,x,kh,end-x-1,digit,Command::insert,0,digit);}
            const std::array<std::string,5> labels{"−10","−1","0","+1","+10"};
            for(int col=0;col<5;++col){const int x=2+col*(cols_-4)/5,end=2+(col+1)*(cols_-4)/5;button(top+3*kh,x,kh,end-x-1,labels[size_t(col)],col==2?Command::insert:Command::step,0,col==2?"0":std::to_string(col==0?-10:col==1?-1:col==3?1:10));}
        } else {
            const std::array<std::wstring,4> letters=options_.language==client::Language::turkish?std::array<std::wstring,4>{L"1234567890",L"qwertyuıopğü",L"asdfghjklşi",L"zxcvbnmöç"}:std::array<std::wstring,4>{L"1234567890",L"qwertyuiop",L"asdfghjkl",L"zxcvbnm"};
            const std::array<std::wstring,4> symbols{L"1234567890",L"!@#$%&*()_+",L"-=/\\:;\"'[]",L".,?<>|{}~"};
            for(int row=0;row<4;++row){auto keys=page_.symbols?symbols[size_t(row)]:letters[size_t(row)];for(size_t col=0;col<keys.size();++col){auto key=keys[col];if(page_.shift&&!page_.symbols)key=options_.language==client::Language::turkish&&key==L'i'?L'İ':towupper(key);const auto label=text::utf8(std::wstring(1,key));const int x=2+int(col)*(cols_-4)/int(keys.size()),end=2+int(col+1)*(cols_-4)/int(keys.size());button(top+row*kh,x,kh,end-x-1,label,Command::insert,0,label);}}
        }
        const std::array<std::string,7> controls{numeric()?"":page_.shift?"abc":"ABC",numeric()?"":"#+=",tr("Space","Boşluk"),"←","→","⌫",tr("Clear","Temizle")};
        const std::array<Command,7> commands{Command::shift,Command::symbols,Command::space,Command::left,Command::right,Command::erase,Command::clear};
        for(int i=0;i<7;++i){if(numeric()&&i<3)continue;const int x=2+i*(cols_-4)/7,end=2+(i+1)*(cols_-4)/7;button(top+4*kh,x,kh,end-x-1,controls[size_t(i)],commands[size_t(i)]);}
        if(kh>1)for(int row=0;row<5;++row)fill(top+(row+1)*kh-1,2,1,cols_-4,0);
    }
    void draw_input() {
        const std::string title=page_.view==View::edit?current().parameters[page_.field].title.get(options_.language):page_.view==View::search?tr("Search all actions","Tüm işlemlerde ara"):page_.view==View::find?tr("Find in result","Sonuçta bul"):page_.view==View::confirm?tr("Type the confirmation","Onayı yazın"):tr("Save a private receipt","Özel makbuz kaydet");panel(title);
        std::string description=page_.view==View::edit?current().parameters[page_.field].description.get(options_.language):page_.view==View::confirm?tr("Type exactly: ","Aynen yazın: ")+confirmation():page_.view==View::save?tr("New file only; contains private observations.","Yalnız yeni dosya; özel gözlemler içerir."):tr("Type a word. Enter shows matches; Esc discards.","Bir sözcük yazın. Enter eşleşmeleri gösterir; Esc vazgeçer.");
        line(body_y_,3,cols_-21,description);button(body_y_,cols_-19,1,16,page_.keyboard?tr("Hide keyboard","Klavyeyi gizle"):tr("Keyboard","Klavye"),Command::keyboard);
        size_t left=0;const int width=cols_-8;
        while(left<page_.caret&&text::cells(std::wstring_view(page_.input).substr(left,page_.caret-left))>=width-1)++left;
        fill(body_y_+1,2,1,cols_-4,A_REVERSE);line(body_y_+1,3,cols_-6,"> "+text::utf8(page_.input.substr(left)),A_REVERSE);
        if(!page_.keyboard){curs_set(1);move(body_y_+1,5+text::cells(std::wstring_view(page_.input).substr(left,page_.caret-left)));}
        const int gap=body_h_<10?2:3;
        if(page_.keyboard)keyboard(body_y_+gap,body_h_-gap);
        else {
            if(page_.view==View::search){const auto found=client::search(text::utf8(page_.input),{},options_.language);line(body_y_+3,3,cols_-6,std::to_string(found.size())+tr(" matches — Enter to show"," eşleşme — göstermek için Enter"),accent());for(size_t i=0;i<found.size()&&int(i)<body_h_-5;++i)line(body_y_+5+int(i),3,cols_-6,client::action(found[i]).title.get(options_.language));}
            else {auto guide=description+"\n\n"+tr("Enter accepts · Esc discards · Ctrl-U clears","Enter kabul eder · Esc vazgeçer · Ctrl-U temizler");if(numeric()){const auto& p=current().parameters[page_.field];guide+="\n\n"+tr("Allowed range: ","Geçerli aralık: ")+std::to_string(p.minimum)+"–"+std::to_string(p.maximum);}paragraph(body_y_+3,body_h_-3,guide);}
        }
    }
    void render() {
        int old_rows=rows_,old_cols=cols_;getmaxyx(stdscr,rows_,cols_);if(old_rows!=rows_||old_cols!=cols_)invalidate();
        body_h_=rows_-11;row_h_=rows_>=28?3:2;max_scroll_=0;hits_.clear();erase();curs_set(0);
        if(rows_<18||cols_<64){line(1,1,cols_-2,tr("Terminal too small. Minimum: 64×18. q exits.","Terminal çok küçük. En az 64×18 gerekli. q çıkar."));refresh();return;}
        header();
        switch(page_.view) {
        case View::menu:draw_menu();break;case View::guide:draw_guide();break;case View::form:draw_form();break;case View::choices:draw_choices();break;case View::review:draw_review();break;case View::result:draw_result();break;case View::history:draw_history();break;
        case View::choice_info:{const auto& p=current().parameters[page_.field];const auto& c=p.choices[page_.choice];panel(c.title.get(options_.language));paragraph(body_y_,body_h_,c.description.get(options_.language)+"\n\n"+tr("Value: ","Değer: ")+c.value);break;}
        case View::help:panel(tr("Help","Yardım"));paragraph(body_y_,body_h_,help());break;
        default:draw_input();break;
        }
        footer();refresh();
    }
    void complete() {
        active_->result.running=false;history_.push_back(active_);
        const auto bytes=[&]{size_t n=0;for(const auto& e:history_)n+=e->result.output.size()+e->result.error.size();return n;};
        while(history_.size()>32||(history_.size()>1&&bytes()>8*1024*1024))history_.erase(history_.begin());
        notice_=tr("Finished: ","Tamamlandı: ")+client::action(active_->result.action_id).title.get(options_.language)+" · "+std::to_string(active_->result.exit_status);
    }
    void tick() {
        if(query_){query_->poll();active_->result=query_->result();if(!active_->result.running){query_.reset();complete();}}
        if(fake_&&Clock::now()>=fake_end_){active_->result.output=fake_->output;active_->result.error=fake_->error;active_->result.exit_status=fake_->status;active_->result.elapsed_ms=fake_->delay;fake_.reset();complete();}
    }
    void cancel_query() {
        if(query_){query_->cancel();notice_=tr("Stopping the read; waiting for actual process exit.","Okuma durduruluyor; gerçek süreç çıkışı bekleniyor.");}
        if(fake_){fake_.reset();active_->result.cancelled=true;active_->result.exit_status=130;complete();}
    }
    void start(client::Plan request) {
        if(busy())throw std::runtime_error(tr("A read is still running. Open its result and use Stop before starting another action.","Bir okuma hâlâ çalışıyor. Başka işlemden önce sonucunu açıp Durdur kullanın."));
        if(!writable()&&request.effect!=client::Effect::read)throw std::runtime_error(tr("Changes are disabled in this mode.","Bu kipte değişiklikler kapalı."));
        auto next=page_;next.view=View::result;next.scroll=0;next.raw=false;next.entry=std::make_shared<Entry>();next.entry->sequence=++sequence_;next.entry->values=request.values;next.entry->result.action_id=request.action_id;next.entry->result.running=true;push(next);active_=page_.entry;
        if(simulation_){const auto found=fixture_.find(request.action_id);fake_=found==fixture_.end()?FixtureValue{"{\"available\":false,\"reason\":\"no_fixture_result\"}",tr("No synthetic result for this action.","Bu işlem için örnek sonuç yok."),69,100}:found->second;fake_end_=Clock::now()+std::chrono::milliseconds(fake_->delay);return;}
        if(request.presentation==client::Presentation::capture){try{query_=std::make_unique<client::Query>(request.action_id,request.values);}catch(...){active_->result.running=false;back();throw;}return;}
        terminal_.pause();SignalHandler ignore_int(SIGINT,SIG_IGN),ignore_quit(SIGQUIT,SIG_IGN);
        std::cout<<"\n"<<client::action(request.action_id).title.get(options_.language)<<"\n"<<client::display_command(request.argv)<<"\n"<<tr("Returning after the command exits.\n","Komut çıktıktan sonra geri dönülecek.\n")<<std::flush;
        try{active_->result=client::run_terminal(request.action_id,request.values,request.confirmation_required);}catch(const std::exception& failure){active_->result.running=false;active_->result.exit_status=127;active_->result.error=failure.what();}
        terminal_.resume();complete();
    }
    void guide(const std::string& id) {
        auto next=page_;next.view=View::guide;next.id=id;next.scroll=0;next.details=false;next.plan.reset();next.phrase.clear();push(next);
    }
    void related(bool preview) {
        if(page_.id.empty())return;
        const auto& a=current();const auto id=preview?a.preview:a.inspect;
        if(id.empty()){notice_=tr("No separate observation/preview is defined. Read this action's explanation.","Ayrı gözlem/önizleme tanımlı değil. Bu işlemin açıklamasını okuyun.");return;}
        const auto values=draft(a.id);auto& next=draft(id);for(auto& [key,value]:next)if(values.contains(key))value=values.at(key);guide(id);
    }
    void review() {auto request=client::plan(page_.id,draft(page_.id));auto next=page_;next.view=View::review;next.plan=request;next.phrase.clear();next.scroll=0;next.details=false;push(next);}
    void begin_input(View view,const std::string& value,bool touch) {auto next=page_;next.view=view;next.input=text::wide(value);next.caret=next.input.size();next.keyboard=touch;next.shift=next.symbols=false;next.scroll=0;push(next);}
    void open_field(bool touch) {
        if(page_.field==current().parameters.size()){review();return;}
        const auto& p=current().parameters[page_.field];const auto value=draft(page_.id).at(p.id);
        if(p.kind==client::Kind::choice){auto next=page_;next.view=View::choices;next.top=0;next.choice=0;for(size_t i=0;i<p.choices.size();++i)if(p.choices[i].value==value)next.choice=i;push(next);}
        else begin_input(View::edit,value,touch);
    }
    void accept_input() {
        const auto value=text::utf8(page_.input);const auto view=page_.view;
        if(view==View::search){Page next;next.view=View::menu;next.menu="search";next.filter=value;page_=next;invalidate();return;}
        if(view==View::edit){client::validate_value(current().parameters[page_.field],value);draft(page_.id)[current().parameters[page_.field].id]=value;back();}
        else if(view==View::confirm){back();page_.phrase=value;}
        else if(view==View::save){if(!writable()||!page_.entry||page_.entry->result.running)throw std::runtime_error("Saving is disabled");client::save_receipt(value,page_.entry->result,page_.entry->values);back();notice_=tr("Private receipt saved: ","Özel makbuz kaydedildi: ")+value;}
        else if(view==View::find){back();find_=value;find_next(true);}
    }
    void next(bool touch=false) {
        if(input_view()){accept_input();return;}
        switch(page_.view) {
        case View::menu:{const auto list=items();if(list.empty())return;const auto item=list.at(page_.selected);if(item.action)guide(item.id);else {const auto& m=uke::tui_menu::menu(item.id);if(!m.action.empty()){draft(m.action)["topic"]=m.topic;guide(m.action);}else {Page next;next.menu=item.id;push(next);}}break;}
        case View::guide:if(!current().parameters.empty()){draft(page_.id);auto next=page_;next.view=View::form;next.top=next.field=next.selected=0;push(next);}else if(current().effect==client::Effect::read&&current().presentation==client::Presentation::capture)start(client::plan(page_.id,{}));else review();break;
        case View::form:review();break;
        case View::choices:{auto next=page_;next.view=View::choice_info;next.scroll=0;push(next);break;}
        case View::choice_info:{const auto& p=current().parameters[page_.field];draft(page_.id)[p.id]=p.choices.at(page_.choice).value;back();back();break;}
        case View::review:if(page_.phrase!=confirmation()){notice_=tr("Confirmation does not match. Nothing ran.","Onay eşleşmiyor. İşlem çalışmadı.");if(touch)begin_input(View::confirm,page_.phrase,true);}else start(*page_.plan);break;
        case View::history:if(!history_.empty()){auto entry=history_.at(page_.selected);auto next=page_;next.view=View::result;next.id=entry->result.action_id;next.menu=uke::tui_menu::owner(client::action(next.id));next.entry=entry;next.scroll=0;next.raw=false;push(next);}break;
        default:back();break;
        }
    }
    void select_delta(int delta) {
        size_t count=0;
        if(page_.view==View::menu)count=items().size();else if(page_.view==View::form)count=current().parameters.size()+1;else if(page_.view==View::choices)count=current().parameters[page_.field].choices.size();else if(page_.view==View::history)count=history_.size();
        if(count){auto& at=page_.view==View::form?page_.field:page_.view==View::choices?page_.choice:page_.selected;at=size_t(std::clamp(int(at)+delta,0,int(count)-1));}else page_.scroll=std::clamp(page_.scroll+delta,0,max_scroll_);
    }
    void drag_delta(int delta) {
        size_t count=0;int height=body_h_;
        if(page_.view==View::menu)count=items().size();else if(page_.view==View::form){count=current().parameters.size()+1;height-=3;}else if(page_.view==View::choices)count=current().parameters[page_.field].choices.size();else if(page_.view==View::history)count=history_.size();
        if(!count){select_delta(delta);drag_y_-=delta;return;}
        const int steps=delta/row_h_,capacity=std::max(1,height/row_h_);if(!steps)return;
        drag_y_-=steps*row_h_;
        page_.top=size_t(std::clamp(int(page_.top)+steps,0,std::max(0,int(count)-capacity)));
        auto& at=page_.view==View::form?page_.field:page_.view==View::choices?page_.choice:page_.selected;
        at=std::clamp(at,page_.top,std::min(count-1,page_.top+size_t(capacity)-1));
    }
    void find_next(bool first=false) {
        if(find_.empty())return;
        const auto lines=text::wrap(result_text(),cols_-6);if(lines.empty())return;
        for(size_t offset=first?0:1;offset<=lines.size();++offset){const auto at=(size_t(page_.scroll)+offset)%lines.size();if(lines[at].find(text::wide(find_))!=std::wstring::npos){page_.scroll=int(at);return;}}
        notice_=tr("No matching result line.","Eşleşen sonuç satırı yok.");
    }
    void edit_key(wint_t key,bool special) {
        if(key==27){back();return;}if(key==L'\n'||key==L'\r'||key==KEY_ENTER){accept_input();return;}
        if(key==21){page_.input.clear();page_.caret=0;}
        else if(key==KEY_BACKSPACE||key==127||key==8){if(page_.caret)page_.input.erase(--page_.caret,1);}
        else if(key==KEY_DC){if(page_.caret<page_.input.size())page_.input.erase(page_.caret,1);}
        else if(key==KEY_LEFT){if(page_.caret)--page_.caret;}
        else if(key==KEY_RIGHT){if(page_.caret<page_.input.size())++page_.caret;}
        else if(key==KEY_HOME)page_.caret=0;else if(key==KEY_END)page_.caret=page_.input.size();
        else if(!special&&iswprint(wchar_t(key))&&text::utf8(page_.input).size()<4092){page_.input.insert(page_.caret,1,wchar_t(key));++page_.caret;}
    }
    void quit(){if(busy()){cancel_query();notice_=tr("Read stopping. Press q again after it exits.","Okuma durduruluyor. Çıktıktan sonra yeniden q basın.");}else running_=false;}
    void command(const Hit& target,bool touch) {
        switch(target.command) {
        case Command::back:if(back_.empty())quit();else back();break;
        case Command::next:next(touch);break;
        case Command::home:page_=Page{};back_.clear();invalidate();break;
        case Command::search:begin_input(View::search,"",touch);break;
        case Command::history:{auto next=page_;next.view=View::history;next.selected=history_.empty()?0:history_.size()-1;next.top=0;push(next);break;}
        case Command::active:if(active_){auto next=page_;next.view=View::result;next.entry=active_;next.id=active_->result.action_id;next.menu=uke::tui_menu::owner(client::action(next.id));next.scroll=0;next.raw=false;push(next);}break;
        case Command::help:{auto next=page_;next.view=View::help;next.scroll=0;push(next);break;}
        case Command::up:select_delta(-1);break;case Command::down:select_delta(1);break;
        case Command::item:if(page_.view==View::review)begin_input(View::confirm,page_.phrase,touch);else {page_.selected=target.index;next(touch);}break;
        case Command::field:page_.field=target.index;open_field(touch);break;
        case Command::choice:page_.choice=target.index;next(touch);break;
        case Command::keyboard:page_.keyboard=!page_.keyboard;invalidate();break;
        case Command::insert:for(const auto c:text::wide(target.value))edit_key(wint_t(c),false);break;
        case Command::shift:page_.shift=!page_.shift;break;case Command::symbols:page_.symbols=!page_.symbols;break;
        case Command::space:edit_key(L' ',false);break;case Command::erase:edit_key(KEY_BACKSPACE,true);break;case Command::clear:edit_key(21,false);break;case Command::left:edit_key(KEY_LEFT,true);break;case Command::right:edit_key(KEY_RIGHT,true);break;
        case Command::step:{const auto& p=current().parameters[page_.field];const auto raw=text::utf8(page_.input);std::uint64_t value=p.minimum;if(!raw.empty()){client::validate_value(p,raw);value=std::stoull(raw);}const int amount=std::stoi(target.value);if(amount<0)value-=std::min(value-p.minimum,std::uint64_t(-amount));else value+=std::min(p.maximum-value,std::uint64_t(amount));page_.input=text::wide(std::to_string(value));page_.caret=page_.input.size();break;}
        case Command::inspect:related(false);break;case Command::preview:related(true);break;
        case Command::details:page_.details=!page_.details;page_.scroll=0;break;
        case Command::favorite:if(!favorites_.erase(page_.id))favorites_.insert(page_.id);break;
        case Command::raw:page_.raw=!page_.raw;page_.scroll=0;break;
        case Command::find:begin_input(View::find,find_,touch);break;
        case Command::save:if(!writable())notice_=tr("Saving is disabled in this mode.","Bu kipte kaydetme devre dışı.");else if(page_.entry&&!page_.entry->result.running)begin_input(View::save,"uke-tui-result-"+std::to_string(page_.entry->sequence)+".json",touch);break;
        case Command::cancel:cancel_query();break;
        }
    }
    std::optional<Hit> hit(int y,int x)const {for(auto it=hits_.rbegin();it!=hits_.rend();++it)if(it->contains(y,x))return *it;return {};}
    void mouse_event(const MEVENT& e) {
        if(e.bstate&(BUTTON4_PRESSED|BUTTON5_PRESSED)){pressed_.reset();select_delta(e.bstate&BUTTON4_PRESSED?-3:3);return;}
        if(e.bstate&BUTTON1_RELEASED) {
            const auto target=hit(e.y,e.x);const auto press=pressed_;pressed_.reset();
            if(!press||dragging_||press_generation_!=generation_||Clock::now()<touch_ready_||!target||!target->same(*press)||std::abs(e.y-press_y_)>1||std::abs(e.x-press_x_)>2)return;
            const auto before=generation_;command(*target,true);if(before!=generation_)touch_ready_=Clock::now()+std::chrono::milliseconds(250);return;
        }
        if(e.bstate&(BUTTON1_PRESSED|REPORT_MOUSE_POSITION)) {
            if(!pressed_){if(!(e.bstate&BUTTON1_PRESSED)||Clock::now()<touch_ready_)return;pressed_=hit(e.y,e.x);press_y_=drag_y_=e.y;press_x_=e.x;press_generation_=generation_;return;}
            if(std::abs(e.y-press_y_)>1||std::abs(e.x-press_x_)>2)dragging_=true;
            if(dragging_&&e.y!=drag_y_)drag_delta(drag_y_-e.y);
        }
    }
    void mouse() {
        // Consume the whole run: one KEY_MOUSE can contain both press and release.
        MEVENT event{};while(getmouse(&event)==OK)mouse_event(event);
    }
    void key(wint_t key,bool special) {
        if(key==KEY_RESIZE){invalidate();return;}
        if(key==KEY_MOUSE){mouse();return;}
        notice_.clear();
        if(input_view()){edit_key(key,special);return;}
        if(key==27||key==KEY_LEFT){back();return;}
        if(key==KEY_F(4)){options_.language=options_.language==client::Language::turkish?client::Language::english:client::Language::turkish;page_.phrase.clear();invalidate();return;}
        if(key==KEY_F(1)||key==L'?'){if(page_.view==View::help)back();else command({0,0,0,0,Command::help},false);return;}
        if(key==L'q'||key==L'Q'){quit();return;}
        if(key==KEY_HOME){command({0,0,0,0,Command::home},false);return;}
        if(key==L'h'||key==L'H'){command({0,0,0,0,Command::history},false);return;}
        if(key==KEY_F(3)||(key==L'/'&&page_.view!=View::review)){command({0,0,0,0,page_.view==View::result?Command::find:Command::search},false);return;}
        if(key==KEY_F(2)){Page next;next.menu="favorites";push(next);return;}
        if(key==KEY_F(5)||key==KEY_F(7)){related(key==KEY_F(7));return;}
        if(key==KEY_F(8)&&page_.view==View::form){review();return;}
        if(key==KEY_F(9)){command({0,0,0,0,Command::active},false);return;}
        if(key==L'\n'||key==L'\r'||key==KEY_ENTER||key==KEY_RIGHT){if(page_.view==View::form)open_field(false);else next();return;}
        const int delta=key==KEY_DOWN||key==L'j'||key==L'\t'?1:key==KEY_UP||key==L'k'||key==KEY_BTAB?-1:key==KEY_NPAGE?std::max(1,body_h_/row_h_-1):key==KEY_PPAGE?-std::max(1,body_h_/row_h_-1):0;
        if(delta){select_delta(delta);return;}
        if(page_.view==View::result){if(key==L'J')command({0,0,0,0,Command::raw},false);else if(key==L's'||key==L'S')command({0,0,0,0,Command::save},false);else if(key==L'n')find_next();else if(key==L'x')cancel_query();}
        if(page_.view==View::guide&&(key==L'd'||key==L'D'))command({0,0,0,0,Command::details},false);
        if(page_.view==View::guide&&key==L' ')command({0,0,0,0,Command::favorite},false);
        if(page_.view==View::review){if(key==L'd'||key==L'D'){page_.details=!page_.details;page_.scroll=0;}else if(key==L'c'||key==L'C')begin_input(View::confirm,page_.phrase,false);}
    }
public:
    App(Options options,Terminal& terminal):options_(std::move(options)),terminal_(terminal) {
        simulation_=!options_.fixture.empty();if(simulation_)fixture_=load_fixture(options_.fixture);
        const auto start=uke::tui_menu::legacy(options_.start);
        const auto menu=std::find_if(uke::tui_menu::menus().begin(),uke::tui_menu::menus().end(),[&](const auto& m){return m.id==start;});
        const auto target=menu!=uke::tui_menu::menus().end()?start:uke::tui_menu::owner(client::action(options_.start));
        std::vector<std::string> trail;for(auto id=target;id!="home";id=uke::tui_menu::menu(id).parent)trail.push_back(id);
        for(auto it=trail.rbegin();it!=trail.rend();++it){Page next;next.menu=*it;push(next);}
        if(menu==uke::tui_menu::menus().end())guide(options_.start);
        else if(!menu->action.empty()){draft(menu->action)["topic"]=menu->topic;guide(menu->action);}
    }
    int run() {
        while(running_&&!interrupted){tick();render();timeout(busy()?80:-1);wint_t input{};const int type=get_wch(&input);if(type==ERR){if(feof(stdin)||ferror(stdin))break;continue;}
            if(rows_<18||cols_<64){if(input==L'q')quit();continue;}
            try{key(input,type==KEY_CODE_YES);}catch(const std::exception& failure){notice_=tr("Request refused: ","İstek reddedildi: ")+std::string(failure.what());}
        }
        return interrupted?128+interrupted:0;
    }
};
Options arguments(int argc,char** argv) {
    Options result;std::set<std::string> seen;
    for(int i=1;i<argc;++i) {
        const std::string arg=argv[i];if(!seen.insert(arg).second)throw std::invalid_argument("Duplicate TUI option");
        if(arg=="--read-only")result.readonly=true;
        else if(arg=="--no-color")result.color=false;
        else if(arg=="--language"||arg=="--fixture"||arg=="--start") {
            if(++i>=argc)throw std::invalid_argument("Missing TUI option value");
            const std::string value=argv[i];
            if(arg=="--language"){if(value!="tr"&&value!="en")throw std::invalid_argument("Language must be tr or en");result.language=value=="tr"?client::Language::turkish:client::Language::english;}
            else if(arg=="--fixture")result.fixture=value;
            else result.start=value;
        } else throw std::invalid_argument("Unknown TUI option: "+arg);
    }
    return result;
}
}
int main(int argc,char** argv) {
    if(argc==2&&std::string_view(argv[1])=="--version"){std::cout<<"uke-control-tui "<<uke::application_version<<'\n';return 0;}
    if(argc==2&&(std::string_view(argv[1])=="--help"||std::string_view(argv[1])=="-h")) {
        std::cout<<"uke-control tui [--language tr|en] [--read-only] [--no-color] [--start CATEGORY|ACTION] [--fixture FILE]\n"
            "C++ terminal interface. F1: full help; Tab/arrows: navigate; Enter: read/form; F7: preview; F8: review.\n"
            "Run as a session user. Root actions hand over only their validated command to sudo.\n"
            "Fixture mode disables every command and mutation. No history is saved automatically.\n";return 0;
    }
    try {
        const auto options=arguments(argc,argv);std::setlocale(LC_ALL,"");if(MB_CUR_MAX<2)std::setlocale(LC_CTYPE,"C.UTF-8");
        Terminal terminal(options.color);App app(options,terminal);return app.run();
    } catch(const std::invalid_argument& error){std::cerr<<"uke-control tui: "<<error.what()<<'\n';return 64;}
    catch(const std::exception& error){std::cerr<<"uke-control tui: "<<error.what()<<'\n';return 1;}
}
