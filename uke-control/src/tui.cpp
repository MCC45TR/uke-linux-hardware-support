// SPDX-License-Identifier: MIT
#include "client.hpp"
#include "tui-text.hpp"
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
struct Options {client::Language language=client::Language::turkish;bool readonly=false,color=true;std::string fixture,start="overview";};
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
        if(colors&&has_colors()){start_color();use_default_colors();init_pair(1,COLOR_CYAN,-1);init_pair(2,COLOR_YELLOW,-1);init_pair(3,COLOR_RED,-1);color=true;}
        mousemask(BUTTON1_CLICKED|BUTTON1_DOUBLE_CLICKED|BUTTON4_PRESSED|BUTTON5_PRESSED,nullptr);mouseinterval(160);
        for(int signal:{SIGINT,SIGTERM,SIGHUP})signals_.push_back(std::make_unique<SignalHandler>(signal,stop));
    }
    void pause(){def_prog_mode();endwin();tcsetattr(0,TCSANOW,&original_);}
    void resume(){reset_prog_mode();winsize size{};if(!ioctl(0,TIOCGWINSZ,&size)&&size.ws_row&&size.ws_col)resize_term(size.ws_row,size.ws_col);clearok(stdscr,true);curs_set(0);}
    ~Terminal(){if(screen_){endwin();delscreen(screen_);tcsetattr(0,TCSANOW,&original_);}}
};
struct Entry {client::Result result;client::Values values;unsigned sequence=0;};
enum class View { browse,form,choices,review,result,help,history,edit,search,find,save };
class App {
    Options options_;
    Terminal& terminal_;
    std::map<std::string,FixtureValue> fixture_;
    bool simulation_=false,global_=false,favorites_only_=false,raw_=false;
    size_t category_=0,selected_=0,field_=0,choice_=0,history_selected_=0;
    int focus_=1,info_scroll_=0,output_scroll_=0,choice_scroll_=0;
    std::string filter_,notice_,current_id_,find_;
    std::vector<std::string> visible_;
    std::set<std::string> favorites_;
    std::map<std::string,client::Values> drafts_;
    View view_=View::browse,return_view_=View::browse;
    std::wstring input_;
    std::string prior_filter_;
    bool prior_global_=false,prior_favorites_=false;
    size_t prior_selected_=0;
    size_t caret_=0;
    std::optional<client::Plan> pending_;
    std::unique_ptr<client::Query> query_;
    std::optional<FixtureValue> fake_;
    Clock::time_point fake_end_{};
    std::string running_id_;
    client::Values running_values_;
    std::vector<std::shared_ptr<Entry>> history_;
    std::shared_ptr<Entry> result_;
    bool follow_query_=true;
    unsigned sequence_=0;
    int rows_=24,cols_=80,body_=18,nav_width_=24,action_x_=25,action_width_=35;
    int action_y_=3,action_height_=16,detail_x_=61,detail_y_=3,detail_width_=50,detail_height_=16;
    std::string tr(const char* english,const char* turkish)const{return options_.language==client::Language::turkish?turkish:english;}
    const client::Action& current()const{return client::action(current_id_);}
    bool busy()const{return query_||fake_;}
    bool writable()const{return !options_.readonly&&!simulation_;}
    void line(int y,int x,int width,const std::string& value,int attributes=0)const {
        if(y<0||y>=rows_||x<0||x>=cols_||width<=0)return;
        const auto text_value=text::clipped(text::wide(value),std::min(width,cols_-x));
        attron(attributes);mvaddnwstr(y,x,text_value.data(),int(text_value.size()));attroff(attributes);
    }
    void wide_line(int y,int x,int width,const std::wstring& value,int attributes=0)const{line(y,x,width,text::utf8(value),attributes);}
    int accent()const{return A_BOLD|(terminal_.color?COLOR_PAIR(1):0);}
    int warning()const{return A_BOLD|(terminal_.color?COLOR_PAIR(2):0);}
    void panel(int y,int x,int height,int width,const std::string& title,bool active=false)const {
        if(height<2||width<3)return;
        const int attributes=active?accent():A_NORMAL;
        attron(attributes);mvhline(y,x,ACS_HLINE,width);mvhline(y+height-1,x,ACS_HLINE,width);mvvline(y,x,ACS_VLINE,height);mvvline(y,x+width-1,ACS_VLINE,height);
        mvaddch(y,x,ACS_ULCORNER);mvaddch(y,x+width-1,ACS_URCORNER);mvaddch(y+height-1,x,ACS_LLCORNER);mvaddch(y+height-1,x+width-1,ACS_LRCORNER);attroff(attributes);
        line(y,x+2,width-4," "+title+" ",attributes);
    }
    int paragraph(int y,int x,int width,int height,const std::string& value,int offset=0)const {
        const auto lines=text::wrap(value,width);
        offset=std::clamp(offset,0,std::max(0,int(lines.size())-height));
        for(int i=0;i<height&&i+offset<int(lines.size());++i)wide_line(y+i,x,width,lines[size_t(i+offset)]);
        return offset;
    }
    void rebuild() {
        const auto selected_id=visible_.empty()?std::string{}:visible_[std::min(selected_,visible_.size()-1)];
        visible_=client::search(filter_,global_||favorites_only_?"":client::categories()[category_].id,options_.language);
        if(favorites_only_)std::erase_if(visible_,[&](const auto& id){return !favorites_.contains(id);});
        const auto found=std::find(visible_.begin(),visible_.end(),selected_id);
        selected_=found==visible_.end()?0:size_t(found-visible_.begin());
        sync_selected();
    }
    void sync_selected(){if(!visible_.empty())current_id_=visible_[std::min(selected_,visible_.size()-1)];else current_id_.clear();info_scroll_=0;}
    void browse(){view_=View::browse;sync_selected();}
    void select_delta(int amount) {
        if(visible_.empty())return;
        selected_=size_t(std::clamp(int(selected_)+amount,0,int(visible_.size())-1));sync_selected();
    }
    std::string access(const client::Action& a)const {
        if(a.access==client::Access::root)return geteuid()==0?tr("Root (current process)","Root (mevcut süreç)"):tr("Root; sudo only for this action","Root; yalnız bu eylem için sudo");
        if(a.access==client::Access::active_session)return tr("Active local session broker","Etkin yerel oturum broker'ı");
        return tr("Current user / session","Mevcut kullanıcı / oturum");
    }
    client::Values& draft(const std::string& id){if(!drafts_.contains(id))drafts_[id]=client::defaults(client::action(id));return drafts_[id];}
    std::string details(const client::Action& a)const {
        std::string value=a.title.get(options_.language)+"\n\n"+a.description.get(options_.language)+"\n\n"+
            tr("Effect: ","Etki: ")+client::effect_description(a.effect).get(options_.language)+"\n\n"+tr("Access: ","Yetki: ")+access(a)+"\n";
        if(a.tests_package)value+="\n"+tr("Requires the optional uke-control-tests package.","İsteğe bağlı uke-control-tests paketi gerekir.")+"\n";
        if(!a.preview.empty())value+="\nF7: "+tr("Open the safe preview first.","Önce güvenli önizlemeyi açın.")+"\n";
        if(!a.inspect.empty())value+="F5: "+tr("Inspect the related current state.","İlgili mevcut durumu okuyun.")+"\n";
        if(!writable()&&a.effect!=client::Effect::read)value+="\n"+tr("Changes are disabled by simulation/read-only mode.","Değişiklikler simülasyon/salt okunur kipte devre dışı.")+"\n";
        value+="\n"+tr("Form defaults are requests, not current device readings. Unknown data is not zero or false.","Form varsayılanları istektir; mevcut cihaz okuması değildir. Bilinmeyen veri sıfır veya yanlış değildir.")+"\n\nID: "+a.id;
        return value;
    }
    void retain(client::Result result,const client::Values& values) {
        auto entry=std::make_shared<Entry>(Entry{std::move(result),values,++sequence_});history_.push_back(entry);
        if(follow_query_){result_=entry;output_scroll_=0;}
        const auto bytes=[&]{size_t n=0;for(const auto& e:history_)n+=e->result.output.size()+e->result.error.size();return n;};
        while(history_.size()>32||(history_.size()>1&&bytes()>8*1024*1024))history_.erase(history_.begin());
        notice_=tr("Command finished; exit status ","Komut tamamlandı; çıkış kodu ")+std::to_string(entry->result.exit_status)+tr(". Physical acceptance is separate.",". Fiziksel kabul ayrıdır.");
    }
    void tick() {
        if(query_) {
            query_->poll();
            if(!query_->result().running){const auto result=query_->result();query_.reset();retain(result,running_values_);}
        }
        if(fake_&&Clock::now()>=fake_end_) {
            client::Result result;result.action_id=running_id_;result.output=fake_->output;result.error=fake_->error;result.exit_status=fake_->status;result.elapsed_ms=fake_->delay;
            fake_.reset();retain(result,running_values_);
        }
    }
    void cancel_query() {
        if(query_){query_->cancel();notice_=tr("Stopping the read-only query; waiting for actual child exit.","Salt okunur sorgu durduruluyor; gerçek alt süreç çıkışı bekleniyor.");}
        if(fake_){client::Result result;result.action_id=running_id_;result.cancelled=true;result.exit_status=130;fake_.reset();retain(result,running_values_);}
    }
    void open_action(const std::string& id,bool force_form=false) {
        if(id.empty())return;
        current_id_=id;notice_.clear();field_=0;pending_.reset();
        const auto& a=current();
        if(!writable()&&a.effect!=client::Effect::read)notice_=tr("Changes are disabled. You may inspect parameters and plans.","Değişiklikler kapalı. Parametreleri ve planları inceleyebilirsiniz.");
        if(force_form||!a.parameters.empty()){draft(a.id);view_=View::form;return;}
        if(a.effect==client::Effect::read&&a.presentation==client::Presentation::capture){start(a.id,{});return;}
        pending_=client::plan(a.id,{});input_.clear();view_=View::review;
    }
    void related(bool preview) {
        if(current_id_.empty())return;
        const auto id=preview?current().preview:current().inspect;
        if(id.empty()) {
            if(!preview&&current().effect==client::Effect::read)open_action(current_id_);
            else notice_=tr("No separate preview/current-state action is defined; inspect this guide.","Ayrı önizleme/mevcut durum eylemi tanımlanmamış; bu rehberi inceleyin.");
            return;
        }
        const auto prior=draft(current_id_);auto& next=draft(id);
        for(auto& [key,value]:next)if(prior.contains(key))value=prior.at(key);
        open_action(id);
    }
    void start(const std::string& id,const client::Values& values) {
        if(busy()){notice_=tr("A read query is still owned. Press Esc to stop it before another action.","Bir okuma sorgusu hâlâ sahiplenilmiş. Başka eylemden önce Esc ile durdurun.");return;}
        const auto request=client::plan(id,values);
        if(!writable()&&request.effect!=client::Effect::read)throw std::runtime_error("Changes are disabled");
        running_id_=id;running_values_=request.values;view_=View::result;raw_=false;output_scroll_=0;result_.reset();follow_query_=true;
        if(simulation_) {
            const auto it=fixture_.find(id);
            fake_=it==fixture_.end()?FixtureValue{"{\"available\":false,\"reason\":\"no_fixture_result\"}",tr("No synthetic result is provided for this action.","Bu eylem için örnek sonuç sağlanmamış."),69,100}:it->second;
            fake_end_=Clock::now()+std::chrono::milliseconds(fake_->delay);return;
        }
        if(request.presentation==client::Presentation::capture){query_=std::make_unique<client::Query>(id,values);return;}
        terminal_.pause();
        SignalHandler ignore_int(SIGINT,SIG_IGN),ignore_quit(SIGQUIT,SIG_IGN);
        std::cout<<"\n"<<client::action(id).title.get(options_.language)<<"\n"<<client::display_command(request.argv)<<"\n"<<tr("Returning to the TUI after the actual command exits.\n","Gerçek komut çıktıktan sonra TUI'ye dönülecek.\n")<<std::flush;
        client::Result result;
        try{result=client::run_terminal(id,values,request.confirmation_required);}
        catch(const std::exception& failure){result.action_id=id;result.exit_status=127;result.error=failure.what();}
        terminal_.resume();retain(result,request.values);
    }
    void review() {
        pending_=client::plan(current_id_,draft(current_id_));input_.clear();view_=View::review;
    }
    std::string confirmation()const {
        if(!pending_||!pending_->confirmation_required)return {};
        if(pending_->effect==client::Effect::maintenance)return pending_->action_id;
        if(pending_->effect==client::Effect::publish)return pending_->values.at("repository");
        if(pending_->effect==client::Effect::packages)return tr("APPLY","UYGULA");
        return {};
    }
    void metrics() {
        getmaxyx(stdscr,rows_,cols_);body_=rows_-7;nav_width_=24;action_x_=25;action_y_=3;
        if(cols_>=112){action_width_=36;action_height_=body_;detail_x_=62;detail_y_=3;detail_width_=cols_-63;detail_height_=body_;}
        else {action_width_=cols_-26;action_height_=std::max(5,body_/2);detail_x_=25;detail_y_=3+action_height_;detail_width_=cols_-26;detail_height_=body_-action_height_;}
    }
    void header()const {
        const auto mode=simulation_?tr("SIMULATION: no commands execute","SİMÜLASYON: komut çalıştırılmaz"):options_.readonly?tr("READ ONLY","SALT OKUNUR"):tr("REVIEW BEFORE CHANGES","DEĞİŞİKLİKTEN ÖNCE İNCELE");
        line(0,1,cols_-2,"Uke Control "+std::string(uke::application_version)+" | "+mode,accent());
        line(1,1,cols_-2,busy()?tr("Reading: ","Okunuyor: ")+running_id_+" | Esc: "+tr("stop this read","bu okumayı durdur"):tr("Local user interface. Hardware qualification and backend guards remain independent.","Yerel kullanıcı arayüzü. Donanım doğrulaması ve backend denetimleri ayrıdır."),busy()?warning():A_NORMAL);
    }
    void footer(const std::string& keys)const {
        line(rows_-4,1,cols_-2,notice_,warning());
        line(rows_-3,1,cols_-2,keys);
        line(rows_-2,1,cols_-2,tr("F1 help | F2 favorites | F3 search | F4 TR/EN | H history | q quit","F1 yardım | F2 favoriler | F3 ara | F4 TR/EN | H geçmiş | q çıkış"));
        line(rows_-1,1,cols_-2,tr("F5 current read | F6 parameters | F7 preview | F8 review | Space favorite","F5 mevcut okuma | F6 parametreler | F7 önizleme | F8 incele | Boşluk favori"));
    }
    void draw_browse() {
        panel(3,0,body_,nav_width_,tr("Categories","Kategoriler"),focus_==0);
        const int catrows=body_-2;const auto top=std::max(0,int(category_)-catrows+1);
        for(int row=0;row<catrows&&row+top<int(client::categories().size());++row)line(4+row,1,nav_width_-2,client::categories()[size_t(row+top)].title.get(options_.language),size_t(row+top)==category_?A_REVERSE:0);
        panel(action_y_,action_x_,action_height_,action_width_,favorites_only_?tr("Favorites","Favoriler"):global_?tr("Search: all","Arama: tümü"):tr("Actions","Eylemler"),focus_==1);
        const int height=action_height_-2;const auto first=std::max(0,int(selected_)-height+1);
        if(visible_.empty())line(action_y_+1,action_x_+1,action_width_-2,tr("No matching action","Eşleşen eylem yok"));
        for(int row=0;row<height&&row+first<int(visible_.size());++row) {
            const auto& a=client::action(visible_[size_t(row+first)]);
            const auto prefix=favorites_.contains(a.id)?"* ":a.effect==client::Effect::read?"  ":"! ";
            line(action_y_+1+row,action_x_+1,action_width_-2,prefix+a.title.get(options_.language),size_t(row+first)==selected_?A_REVERSE:0);
        }
        panel(detail_y_,detail_x_,detail_height_,detail_width_,tr("Guide / PgUp PgDn","Rehber / PgUp PgDn"),focus_==2);
        if(!current_id_.empty())info_scroll_=paragraph(detail_y_+1,detail_x_+1,detail_width_-2,detail_height_-2,details(current()),info_scroll_);
        footer(tr("Tab: pane | arrows: select | Enter: read / form | /: global search | Esc: stop / back","Tab: alan | oklar: seç | Enter: oku / form | /: genel arama | Esc: durdur / geri"));
        if(!filter_.empty())line(2,1,cols_-2,tr("Filter: ","Filtre: ")+filter_,warning());
        else if(!current_id_.empty())line(2,1,cols_-2,"ID: "+current_id_+tr(" | ! change  * favorite"," | ! değişiklik  * favori"));
    }
    void draw_form() {
        const auto& a=current();panel(3,0,body_,cols_,a.title.get(options_.language));
        const int split=std::clamp(cols_/2,30,58);const int height=body_-3;const int first=std::max(0,int(field_)-height+1);
        const auto& values=drafts_.at(a.id);
        for(int i=0;i<height&&i+first<=int(a.parameters.size());++i) {
            const int index=i+first;
            std::string label=index==int(a.parameters.size())?tr("[ Review this plan ]","[ Bu planı incele ]"):a.parameters[size_t(index)].title.get(options_.language)+": "+values.at(a.parameters[size_t(index)].id);
            line(4+i,2,split-4,label,size_t(index)==field_?A_REVERSE:0);
        }
        mvvline(4,split,ACS_VLINE,body_-2);
        std::string explanation;
        if(field_<a.parameters.size()) {
            const auto& p=a.parameters[field_];explanation=p.title.get(options_.language)+"\n\n"+p.description.get(options_.language)+"\n";
            if(p.kind==client::Kind::integer)explanation+="\n"+tr("Allowed range: ","Geçerli aralık: ")+std::to_string(p.minimum)+".."+std::to_string(p.maximum);
            explanation+="\n"+tr("Form default: ","Form varsayılanı: ")+(p.initial.empty()?tr("(required)","(gerekli)"):p.initial);
            if(p.kind==client::Kind::choice) {
                explanation+="\n\n"+tr("Left/right changes the choice; Enter opens every option and its explanation.","Sol/sağ seçeneği değiştirir; Enter tüm seçenekleri ve açıklamalarını açar.");
                const auto it=std::find_if(p.choices.begin(),p.choices.end(),[&](const auto& value){return value.value==values.at(p.id);});
                if(it!=p.choices.end())explanation+="\n\n"+it->title.get(options_.language)+"\n"+it->description.get(options_.language);
            } else explanation+="\n\n"+tr("Enter edits this value. Paths may contain spaces and UTF-8. Ctrl-U clears. Nothing runs while editing.","Enter bu değeri düzenler. Yollar boşluk ve UTF-8 içerebilir. Ctrl-U temizler. Düzenlerken işlem çalışmaz.");
        } else explanation=a.description.get(options_.language)+"\n\n"+client::effect_description(a.effect).get(options_.language);
        info_scroll_=paragraph(4,split+2,cols_-split-4,body_-2,explanation,info_scroll_);
        footer(tr("Up/down/Tab: field | Enter: edit/choose | PgUp/PgDn: guide | F8: review | Esc: back","Oklar/Tab: alan | Enter: düzenle/seç | PgUp/PgDn: rehber | F8: incele | Esc: geri"));
    }
    void draw_choices()const {
        const auto& p=current().parameters[field_];panel(3,0,body_,cols_,p.title.get(options_.language));
        const int left=std::clamp(cols_/2,25,48),height=body_-2;
        for(int row=0;row<height&&row+choice_scroll_<int(p.choices.size());++row){const auto index=size_t(row+choice_scroll_);line(4+row,2,left-3,p.choices[index].title.get(options_.language),index==choice_?A_REVERSE:0);}
        mvvline(4,left,ACS_VLINE,height);
        if(choice_<p.choices.size())paragraph(4,left+2,cols_-left-4,height,p.choices[choice_].title.get(options_.language)+"\n\n"+p.choices[choice_].description.get(options_.language)+"\n\n"+tr("Value: ","Değer: ")+p.choices[choice_].value);
        footer(tr("Up/down/PgUp/PgDn: option | Enter: choose | Esc: keep previous value","Yukarı/aşağı/PgUp/PgDn: seçenek | Enter: seç | Esc: önceki değeri koru"));
    }
    void draw_review() {
        panel(3,0,body_,cols_,tr("Review the exact request","Tam isteği incele"),true);
        const auto& a=client::action(pending_->action_id);
        std::string review=a.title.get(options_.language)+"\n\n"+a.description.get(options_.language)+"\n\n"+
            client::effect_description(a.effect).get(options_.language)+"\n\n"+tr("Access: ","Yetki: ")+access(a)+"\n\n";
        for(const auto& [key,value]:pending_->values)review+=key+": "+value+"\n";
        review+="\n"+tr("Exact argument vector (display only; no shell):\n","Tam argüman dizisi (gösterim; kabuk kullanılmaz):\n")+client::display_command(pending_->argv);
        if(a.presentation==client::Presentation::terminal)review+="\n\n"+tr("The TUI will restore the terminal. Existing interactive backend approvals remain required. Writes have no application timeout.","TUI terminali geri yükleyecek. Mevcut etkileşimli backend onayları gereklidir. Yazmalara uygulama süre aşımı uygulanmaz.");
        info_scroll_=paragraph(4,2,cols_-4,body_-5,review,info_scroll_);
        const auto phrase=confirmation();
        line(rows_-8,2,cols_-4,!writable()&&pending_->confirmation_required?tr("Changes disabled; this plan cannot execute.","Değişiklikler kapalı; bu plan çalıştırılamaz."):phrase.empty()?tr("Enter: run this request  |  Esc: cancel","Enter: bu isteği çalıştır  |  Esc: iptal"):tr("Type to confirm: ","Onay için yazın: ")+phrase,warning());
        if(!phrase.empty())line(rows_-7,2,cols_-4,"> "+text::utf8(input_),A_REVERSE);
        footer(tr("PgUp/PgDn: review details | F7: safe preview | Enter: explicit approval | Esc: no action","PgUp/PgDn: ayrıntılar | F7: güvenli önizleme | Enter: açık onay | Esc: işlem yapmadan dön"));
    }
    std::string result_text()const {
        const client::Result* r=query_&&follow_query_?&query_->result():result_?&result_->result:nullptr;
        if(!r&&fake_&&follow_query_)return tr("Synthetic read is pending. No command or hardware query was started.","Örnek okuma bekleniyor. Komut veya donanım sorgusu başlatılmadı.");
        if(!r)return tr("No result yet. Select an action to begin.","Henüz sonuç yok. Başlamak için bir eylem seçin.");
        std::string value=tr("Action: ","Eylem: ")+r->action_id+"\n"+tr("Exit status: ","Çıkış kodu: ")+(r->running?tr("running","çalışıyor"):std::to_string(r->exit_status))+" | "+std::to_string(r->elapsed_ms)+" ms\n";
        if(r->cancelled)value+=tr("Cancellation requested; this is not successful completion.\n","İptal istendi; bu başarılı tamamlama değildir.\n");
        if(r->timed_out)value+=tr("Read deadline exceeded.\n","Okuma süresi aşıldı.\n");
        if(r->truncated)value+=tr("Output bound exceeded; retained output is partial.\n","Çıktı sınırı aşıldı; tutulan çıktı kısmidir.\n");
        value+=tr("Physical acceptance: not established\n\n","Fiziksel kabul: bu sonuçla kurulmadı\n\n");
        if(!r->output.empty())value+=client::format_output(r->output,options_.language,raw_);
        else if(client::action(r->action_id).presentation==client::Presentation::terminal)value+=tr("Interactive output was shown in the normal terminal. This receipt records only the actual exit status. Use F5 for a fresh related observation.\n","Etkileşimli çıktı normal terminalde gösterildi. Bu makbuz yalnız gerçek çıkış kodunu kaydeder. Yeni ilgili okuma için F5 kullanın.\n");
        if(!r->error.empty())value+="\n"+tr("Error stream:\n","Hata çıktısı:\n")+r->error;
        return value;
    }
    void draw_result() {
        panel(3,0,body_,cols_,raw_?tr("Raw JSON / output","Ham JSON / çıktı"):tr("Readable result","Okunabilir sonuç"),true);
        output_scroll_=paragraph(4,2,cols_-4,body_-2,result_text(),output_scroll_);
        footer(tr("Arrows/PgUp/PgDn: scroll | J: JSON | /: find | S: save | Tab: browse | Esc: stop/back","Oklar/PgUp/PgDn: kaydır | J: JSON | /: bul | S: kaydet | Tab: gezin | Esc: durdur/geri"));
    }
    std::string help()const {
        return tr("NAVIGATION\nTab moves between categories, actions and the guide. Arrow keys or j/k select. Enter reads or opens a parameter form. Mouse clicks select; wheel scrolls the focused pane. F3 or / searches every category. F2 filters session favorites; Space marks an action.\n\nPARAMETERS\nEach field has its own purpose, accepted range and choice descriptions. Left/right cycles an enum; Enter opens its full option guide. Enter edits a text/number; Ctrl-U clears; Esc discards the edit. F8 validates the common plan. Editing never executes a command.\n\nSAFE EXECUTION\nF5 reads related current state; F7 opens a separate preview when defined. The review screen shows the exact validated argument vector, access and effect. Normal changes require an explicit Enter; package, publication and storage workflows have visible confirmation text and keep backend approvals. Root actions authorize only their command with sudo. Run the TUI as your regular session user.\n\nRESULTS\nRead queries run without blocking navigation. Esc cancels only a read; writes run in the restored terminal and remain owned until actual exit. J toggles JSON. / searches the result and n finds the next match. S saves a new private 0600 receipt without overwrite. H opens bounded in-memory session history; viewing history never replays a write. No history is saved automatically.\n\nLIMITS\n--fixture visibly disables commands and mutations. --read-only disables changes and result-file writes. Missing data is not zero/false; driver readback is not physical acceptance. No automatic package cleanup, firmware qualification, active chroot, audio recording or device testing is introduced.\n\nGTK / QML\nlibuke-control-client owns the same bilingual catalog, validation, plans, read-query ownership and receipts. 'uke-control actions --json' exposes that metadata without a GUI or device access. No frontend may replace the existing backend admission checks.",
        "GEZİNME\nTab kategoriler, eylemler ve rehber arasında geçer. Oklar veya j/k seçim yapar. Enter okur veya parametre formunu açar. Fare tıklaması seçer; tekerlek odaklanan alanı kaydırır. F3 veya / tüm kategorilerde arar. F2 oturum favorilerini süzer; Boşluk eylemi işaretler.\n\nPARAMETRELER\nHer alanın amacı, geçerli aralığı ve seçenek açıklamaları vardır. Sol/sağ enum değerini değiştirir; Enter tüm seçenek rehberini açar. Enter metin/sayıyı düzenler; Ctrl-U temizler; Esc düzenlemeyi atar. F8 ortak planı doğrular. Düzenlerken komut çalıştırılmaz.\n\nGÜVENLİ ÇALIŞTIRMA\nF5 ilgili mevcut durumu okur; F7 varsa ayrı önizlemeyi açar. İnceleme ekranı tam doğrulanmış argümanları, yetkiyi ve etkiyi gösterir. Normal değişiklikler açık Enter ister; paket, yayımlama ve depolama akışları görünür onay metni kullanır ve backend onaylarını korur. Root eylemleri yalnız kendi komutunu sudo ile yetkilendirir. TUI'yi normal oturum kullanıcısı olarak çalıştırın.\n\nSONUÇLAR\nOkuma sorguları gezinmeyi engellemeden çalışır. Esc yalnız okumayı iptal eder; yazmalar geri yüklenmiş terminalde çalışır ve gerçek çıkışa kadar sahiplenilir. J JSON'u açar/kapatır. / sonuçta arar; n sonraki eşleşmeye gider. S üzerine yazmadan yeni özel 0600 makbuz kaydeder. H sınırlı bellek içi oturum geçmişini açar; geçmişe bakmak yazmayı tekrarlamaz. Geçmiş otomatik kaydedilmez.\n\nSINIRLAR\n--fixture komutları ve değişiklikleri görünür biçimde devre dışı bırakır. --read-only değişiklikleri ve sonuç dosyası yazmasını kapatır. Eksik veri sıfır/yanlış değildir; sürücü geri okuması fiziksel kabul değildir. Otomatik paket temizliği, firmware doğrulaması, etkin chroot, ses kaydı veya cihaz testi eklenmez.\n\nGTK / QML\nlibuke-control-client aynı iki dilli kataloğa, doğrulamaya, planlara, okuma sorgusu sahipliğine ve makbuzlara sahiptir. 'uke-control actions --json' bu metaveriyi GUI veya cihaz erişimi olmadan sunar. Hiçbir ön yüz mevcut backend kabul denetimlerinin yerini alamaz.");
    }
    void draw_history()const {
        panel(3,0,body_,cols_,tr("Session history (never replays)","Oturum geçmişi (işlem tekrarlamaz)"),true);
        if(history_.empty())line(5,2,cols_-4,tr("No completed query/action yet.","Henüz tamamlanmış sorgu/eylem yok."));
        const int height=body_-2,first=std::max(0,int(history_selected_)-height+1);
        for(int row=0;row<height&&row+first<int(history_.size());++row) {
            const auto index=size_t(row+first);const auto& e=*history_[index];
            line(4+row,2,cols_-4,"#"+std::to_string(e.sequence)+" ["+std::to_string(e.result.exit_status)+"] "+client::action(e.result.action_id).title.get(options_.language)+" ("+std::to_string(e.result.elapsed_ms)+" ms)",index==history_selected_?A_REVERSE:0);
        }
        footer(tr("Up/down: result | Enter: view only | Esc: back | History stays in memory","Yukarı/aşağı: sonuç | Enter: yalnız görüntüle | Esc: geri | Geçmiş bellekte kalır"));
    }
    void draw_input() {
        if(view_==View::search){draw_browse();line(2,1,cols_-2,tr("Search all: ","Tümünde ara: ")+text::utf8(input_),A_REVERSE);}
        else {
            panel(3,0,body_,cols_,view_==View::edit?current().parameters[field_].title.get(options_.language):view_==View::save?tr("Save a new private receipt","Yeni özel makbuz kaydet"):tr("Find in output","Çıktıda bul"),true);
            const auto explanation=view_==View::edit?current().parameters[field_].description.get(options_.language):view_==View::save?tr("Existing files are preserved. Receipts contain private observations and paths; review them before sharing.","Mevcut dosyalar korunur. Makbuzlar özel okumalar ve yollar içerir; paylaşmadan önce inceleyin."):tr("Search the currently displayed result. Enter finds the next matching line.","Gösterilen sonuçta arar. Enter sonraki eşleşen satırı bulur.");
            paragraph(5,2,cols_-4,5,explanation);
            const auto columns=std::max(1,cols_-6);auto value=input_;size_t left=0;
            while(left<caret_&&text::cells(std::wstring_view(value).substr(left,caret_-left))>=columns-1)++left;
            wide_line(12,2,cols_-4,L"> "+value.substr(left),A_REVERSE);
            if(12<rows_-6){curs_set(1);move(12,4+text::cells(std::wstring_view(value).substr(left,caret_-left)));}
        }
        footer(tr("Enter: accept | Esc: discard | Ctrl-U: clear | Left/right/Home/End: cursor","Enter: kabul | Esc: vazgeç | Ctrl-U: temizle | Sol/sağ/Home/End: imleç"));
    }
    void render() {
        metrics();erase();curs_set(0);header();
        if(rows_<18||cols_<64) {
            paragraph(4,1,std::max(1,cols_-2),std::max(1,rows_-6),tr("Terminal too small. Resize to at least 64 columns and 18 rows. q exits safely.","Terminal çok küçük. En az 64 sütun ve 18 satıra büyütün. q güvenle çıkar."));
            refresh();return;
        }
        switch(view_) {
        case View::browse:draw_browse();break;case View::form:draw_form();break;case View::choices:draw_choices();break;case View::review:draw_review();break;
        case View::result:draw_result();break;case View::history:draw_history();break;
        case View::help:panel(3,0,body_,cols_,tr("Full help and boundaries","Tam yardım ve sınırlar"),true);paragraph(4,2,cols_-4,body_-2,help(),info_scroll_);footer(tr("Up/down/PgUp/PgDn: scroll | Esc: return","Yukarı/aşağı/PgUp/PgDn: kaydır | Esc: dön"));break;
        case View::edit:case View::search:case View::find:case View::save:draw_input();break;
        }
        refresh();
    }
    void begin_input(View mode,const std::string& value={}){
        return_view_=view_;view_=mode;input_=text::wide(value);caret_=input_.size();
        if(mode==View::search){prior_filter_=filter_;prior_global_=global_;prior_favorites_=favorites_only_;prior_selected_=selected_;}
    }
    void find_next(bool first=false) {
        const auto lines=text::wrap(result_text(),std::max(1,cols_-4));const auto needle=text::wide(find_);
        if(needle.empty())return;
        for(size_t offset=first?0:1;offset<=lines.size();++offset) {
            const auto at=(size_t(std::max(0,output_scroll_))+offset)%lines.size();
            if(lines[at].find(needle)!=std::wstring::npos){output_scroll_=int(at);return;}
        }
        notice_=tr("No matching result line.","Eşleşen sonuç satırı yok.");
    }
    void input_key(wint_t key,bool special) {
        if(key==27){
            if(view_==View::search){filter_=prior_filter_;global_=prior_global_;favorites_only_=prior_favorites_;rebuild();selected_=visible_.empty()?0:std::min(prior_selected_,visible_.size()-1);sync_selected();}
            view_=return_view_;return;
        }
        if(key==L'\n'||key==KEY_ENTER||key==L'\r') {
            const auto value=text::utf8(input_);
            if(view_==View::edit){client::validate_value(current().parameters[field_],value);draft(current_id_)[current().parameters[field_].id]=value;view_=View::form;}
            else if(view_==View::search){filter_=value;view_=View::browse;focus_=1;}
            else if(view_==View::find){find_=value;view_=View::result;find_next(true);}
            else if(view_==View::save){if(!result_||!writable())throw std::runtime_error("Result saving is disabled");client::save_receipt(value,result_->result,result_->values);view_=View::result;notice_=tr("Private receipt saved: ","Özel makbuz kaydedildi: ")+value;}
            return;
        }
        if(key==21){input_.clear();caret_=0;}
        else if(key==KEY_BACKSPACE||key==127||key==8){if(caret_){input_.erase(--caret_,1);}}
        else if(key==KEY_DC){if(caret_<input_.size())input_.erase(caret_,1);}
        else if(key==KEY_LEFT){if(caret_)--caret_;}
        else if(key==KEY_RIGHT){if(caret_<input_.size())++caret_;}
        else if(key==KEY_HOME)caret_=0;
        else if(key==KEY_END)caret_=input_.size();
        else if(!special&&iswprint(wchar_t(key))&&text::utf8(input_).size()<4092){input_.insert(caret_,1,wchar_t(key));++caret_;}
        if(view_==View::search){filter_=text::utf8(input_);global_=true;favorites_only_=false;selected_=0;rebuild();}
    }
    bool key(wint_t key,bool special) {
        notice_.clear();
        if(view_==View::edit||view_==View::search||view_==View::find||view_==View::save){input_key(key,special);return true;}
        if(key==KEY_RESIZE)return true;
        if(key==KEY_F(4)){options_.language=options_.language==client::Language::turkish?client::Language::english:client::Language::turkish;if(view_==View::review)input_.clear();return true;}
        if(key==KEY_F(1)||key==L'?'){if(view_!=View::help){return_view_=view_;view_=View::help;info_scroll_=0;}return true;}
        if(view_!=View::review&&(key==L'q'||key==L'Q')){if(busy()){cancel_query();return true;}return false;}
        if(view_!=View::review&&(key==L'h'||key==L'H')){history_selected_=history_.empty()?0:history_.size()-1;view_=View::history;return true;}
        if(key==27){if(busy()){cancel_query();return true;}if(view_==View::help)view_=return_view_;else if(view_==View::choices)view_=View::form;else if(view_!=View::browse)browse();else if(!filter_.empty()||favorites_only_){filter_.clear();global_=false;favorites_only_=false;rebuild();}else if(focus_!=0)focus_=0;else return false;return true;}
        const bool down=key==KEY_DOWN||key==L'j',up=key==KEY_UP||key==L'k',enter=key==L'\n'||key==L'\r'||key==KEY_ENTER;
        if(view_==View::help){info_scroll_=std::clamp(info_scroll_+(down?1:up?-1:key==KEY_NPAGE?body_-3:key==KEY_PPAGE?-(body_-3):0),0,std::max(0,int(text::wrap(help(),cols_-4).size())-body_+2));return true;}
        if(view_==View::history) {
            if(!history_.empty())history_selected_=size_t(std::clamp(int(history_selected_)+(down?1:up?-1:0),0,int(history_.size())-1));
            if(enter&&!history_.empty()){result_=history_[history_selected_];current_id_=result_->result.action_id;view_=View::result;output_scroll_=0;raw_=false;follow_query_=false;}
            return true;
        }
        if(view_==View::result) {
            if(key==L'\t'){browse();return true;}
            const int count=int(text::wrap(result_text(),std::max(1,cols_-4)).size()),maximum=std::max(0,count-body_+2);
            if(key==L'J'){raw_=!raw_;output_scroll_=0;}
            else if(key==L'/'){begin_input(View::find,find_);}
            else if(key==L'n')find_next();
            else if(key==L's'||key==L'S'){if(!writable())notice_=tr("Saving is disabled in this mode.","Bu kipte kaydetme devre dışı.");else if(result_)begin_input(View::save,"uke-tui-result-"+std::to_string(result_->sequence)+".json");}
            else if(key==KEY_F(5)){if(!current_id_.empty())related(false);}
            else output_scroll_=std::clamp(key==KEY_HOME?0:key==KEY_END?maximum:output_scroll_+(down?1:up?-1:key==KEY_NPAGE?body_-3:key==KEY_PPAGE?-(body_-3):0),0,maximum);
            return true;
        }
        if(view_==View::choices) {
            const auto& p=current().parameters[field_];const int delta=down?1:up?-1:key==KEY_NPAGE?body_-3:key==KEY_PPAGE?-(body_-3):0;
            choice_=size_t(std::clamp(int(choice_)+delta,0,int(p.choices.size())-1));choice_scroll_=std::max(0,int(choice_)-body_+3);
            if(enter){draft(current_id_)[p.id]=p.choices[choice_].value;view_=View::form;}return true;
        }
        if(view_==View::review) {
            if(key==KEY_F(7)){related(true);return true;}
            if(key==KEY_NPAGE)info_scroll_+=std::max(1,body_-6);else if(key==KEY_PPAGE)info_scroll_=std::max(0,info_scroll_-body_+6);
            else if(enter){if(text::utf8(input_)!=confirmation())notice_=tr("Confirmation does not match; nothing ran.","Onay eşleşmiyor; işlem çalışmadı.");else {const auto request=*pending_;start(request.action_id,request.values);}}
            else if(key==KEY_BACKSPACE||key==127||key==8){if(!input_.empty())input_.pop_back();}
            else if(!special&&iswprint(wchar_t(key))&&input_.size()<160)input_+=wchar_t(key);
            return true;
        }
        if(view_==View::form) {
            if(key==KEY_F(5)||key==KEY_F(7)){related(key==KEY_F(7));return true;}
            if(key==KEY_NPAGE||key==KEY_PPAGE){info_scroll_=std::max(0,info_scroll_+(key==KEY_NPAGE?1:-1)*(body_-3));return true;}
            const auto& a=current();if(down||up||key==L'\t'){field_=size_t(std::clamp(int(field_)+(up?-1:1),0,int(a.parameters.size())));info_scroll_=0;}
            if(key==KEY_F(8)||(enter&&field_==a.parameters.size())){info_scroll_=0;review();}
            else if(field_<a.parameters.size()) {
                const auto& p=a.parameters[field_];auto& value=draft(current_id_)[p.id];
                if(p.kind==client::Kind::choice) {
                    auto it=std::find_if(p.choices.begin(),p.choices.end(),[&](const auto& c){return c.value==value;});const int at=it==p.choices.end()?0:int(it-p.choices.begin());
                    if(key==KEY_LEFT||key==KEY_RIGHT){const int index=(at+(key==KEY_LEFT?-1:1)+int(p.choices.size()))%int(p.choices.size());value=p.choices[size_t(index)].value;}
                    if(enter||key==L' '){choice_=size_t(at);choice_scroll_=std::max(0,at-body_+3);view_=View::choices;}
                } else if(enter)begin_input(View::edit,value);
            }
            return true;
        }
        if(key==KEY_F(2)){favorites_only_=!favorites_only_;global_=favorites_only_;selected_=0;rebuild();return true;}
        if(key==KEY_F(3)||key==L'/'){begin_input(View::search,filter_);return true;}
        if(key==KEY_F(5)){related(false);return true;}
        if(key==KEY_F(7)){related(true);return true;}
        if(key==KEY_F(6)||key==KEY_F(8)){if(!current_id_.empty())open_action(current_id_,key==KEY_F(6));return true;}
        if(key==L'\t'){focus_=(focus_+1)%3;return true;}
        if(key==KEY_BTAB){focus_=(focus_+2)%3;return true;}
        if(key==KEY_LEFT){focus_=std::max(0,focus_-1);return true;}
        if(key==KEY_RIGHT){focus_=std::min(2,focus_+1);return true;}
        if(key==KEY_MOUSE) {
            MEVENT event{};if(getmouse(&event)==OK) {
                if(event.bstate&(BUTTON4_PRESSED|BUTTON5_PRESSED)){if(focus_==2)info_scroll_=std::max(0,info_scroll_+((event.bstate&BUTTON4_PRESSED)?-3:3));else select_delta((event.bstate&BUTTON4_PRESSED)?-1:1);}
                else if(event.x<nav_width_&&event.y>3&&event.y<3+body_-1){const int top=std::max(0,int(category_)-(body_-2)+1);category_=size_t(std::clamp(top+event.y-4,0,int(client::categories().size())-1));global_=false;selected_=0;focus_=0;rebuild();}
                else if(event.x>=action_x_&&event.x<action_x_+action_width_&&event.y>action_y_&&event.y<action_y_+action_height_-1&&!visible_.empty()){const int first=std::max(0,int(selected_)-action_height_+3);selected_=size_t(std::clamp(first+event.y-action_y_-1,0,int(visible_.size())-1));focus_=1;sync_selected();if(event.bstate&BUTTON1_DOUBLE_CLICKED)open_action(current_id_);}
                else focus_=2;
            }return true;
        }
        if(focus_==0&&(down||up)){category_=size_t(std::clamp(int(category_)+(down?1:-1),0,int(client::categories().size())-1));global_=false;selected_=0;rebuild();}
        else if(focus_==1&&(down||up||key==KEY_NPAGE||key==KEY_PPAGE))select_delta(down?1:up?-1:key==KEY_NPAGE?action_height_-3:-(action_height_-3));
        else if(focus_==2&&(down||up||key==KEY_NPAGE||key==KEY_PPAGE))info_scroll_=std::max(0,info_scroll_+(down?1:up?-1:key==KEY_NPAGE?detail_height_-3:-(detail_height_-3)));
        if(key==L' '&&!current_id_.empty()){if(!favorites_.erase(current_id_))favorites_.insert(current_id_);if(favorites_only_)rebuild();}
        if(enter){if(focus_==0)focus_=1;else if(!current_id_.empty())open_action(current_id_);}
        return true;
    }
public:
    App(Options options,Terminal& terminal):options_(std::move(options)),terminal_(terminal) {
        simulation_=!options_.fixture.empty();if(simulation_)fixture_=load_fixture(options_.fixture);
        const auto& categories=client::categories();const auto it=std::find_if(categories.begin(),categories.end(),[&](const auto& c){return c.id==options_.start;});
        if(it!=categories.end())category_=size_t(it-categories.begin());
        else {const auto& a=client::action(options_.start);const auto parent=std::find_if(categories.begin(),categories.end(),[&](const auto& c){return c.id==a.category;});category_=size_t(parent-categories.begin());}
        rebuild();if(it==categories.end()){const auto a=std::find(visible_.begin(),visible_.end(),options_.start);if(a!=visible_.end()){selected_=size_t(a-visible_.begin());sync_selected();}}
    }
    int run() {
        for(bool running=true;running&&!interrupted;) {
            tick();render();timeout(busy()?100:-1);
            wint_t input{};const int type=get_wch(&input);
            if(type==ERR){if(feof(stdin)||ferror(stdin))break;continue;}
            if(rows_<18||cols_<64){if(input==L'q'||input==27)break;continue;}
            try{running=key(input,type==KEY_CODE_YES);}catch(const std::exception& failure){notice_=tr("Request refused: ","İstek reddedildi: ")+std::string(failure.what());}
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
