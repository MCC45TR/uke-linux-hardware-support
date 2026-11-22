// SPDX-License-Identifier: MIT
#include "client.hpp"
#include "sysfs.hpp"
#include "version.hpp"
#include <json/json.h>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <fcntl.h>
#include <functional>
#include <locale.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

#ifndef UKE_CLIENT_PROGRAM
#define UKE_CLIENT_PROGRAM "/usr/bin/uke-control"
#endif
namespace uke::client {
namespace {
using Value=Json::Value;
std::string encode(const Value& value){Json::StreamWriterBuilder writer;writer["indentation"]="";writer["emitUTF8"]=true;return Json::writeString(writer,value);}
bool decode(const std::string& text,Value& value) {
    if(text.size()>8*1024*1024)return false;
    Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["allowComments"]=false;builder["failIfExtra"]=true;builder["stackLimit"]=48;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());std::string error;
    return reader->parse(text.data(),text.data()+text.size(),&value,&error);
}
std::string utf8(const std::string& text){Value value;if(!decode(sysfs::quote(text),value))throw std::runtime_error("Text exceeds its display bound");return value.asString();}
Value bilingual(const Text& text){Value value;value["en"]=text.en;value["tr"]=text.tr;return value;}
std::string kind_name(Kind kind) {
    switch(kind) {
    case Kind::choice:return "choice";case Kind::integer:return "integer";case Kind::path:return "path";
    case Kind::absolute_path:return "absolute_path";case Kind::identifier:return "identifier";case Kind::interface:return "interface";
    case Kind::usb_device:return "usb_device";case Kind::input_device:return "input_device";case Kind::partition_uuid:return "partition_uuid";
    case Kind::color:return "rgb_hex";case Kind::swap_size:return "swap_size";case Kind::repository:return "repository";
    }throw std::logic_error("Unknown parameter kind");
}
void require(bool ok,const std::string& message){if(!ok)throw std::invalid_argument(message);}
std::uint64_t integer(const std::string& text) {
    std::uint64_t result{};const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    require(!text.empty()&&parsed.ec==std::errc{}&&parsed.ptr==text.data()+text.size(),"Expected a nonnegative decimal integer");return result;
}
void validate(const Parameter& p,const std::string& value) {
    require(value.size()<=4096,"Parameter exceeds 4096 bytes: "+p.id);
    if(value.empty()){require(p.optional,"Missing parameter: "+p.id);return;}
    require(utf8(value)==value,"Invalid UTF-8 parameter: "+p.id);
    require(std::none_of(value.begin(),value.end(),[](unsigned char c){return c<32||c==127;}),"Control characters are not allowed: "+p.id);
    for(size_t i=0;i+1<value.size();++i)require(!(static_cast<unsigned char>(value[i])==0xc2&&static_cast<unsigned char>(value[i+1])>=0x80&&static_cast<unsigned char>(value[i+1])<=0x9f),"Unicode control character is not allowed: "+p.id);
    for(const auto* bidi:{"\xe2\x80\xaa","\xe2\x80\xab","\xe2\x80\xac","\xe2\x80\xad","\xe2\x80\xae","\xe2\x81\xa6","\xe2\x81\xa7","\xe2\x81\xa8","\xe2\x81\xa9"})require(value.find(bidi)==value.npos,"Directional override is not allowed: "+p.id);
    require(value.front()!='-',"Parameter cannot begin with an option marker: "+p.id);
    const auto matches=[&](const char* pattern){return std::regex_match(value,std::regex(pattern));};
    switch(p.kind) {
    case Kind::choice:require(std::any_of(p.choices.begin(),p.choices.end(),[&](const auto& c){return c.value==value;}),"Unknown choice: "+p.id);break;
    case Kind::integer:{const auto n=integer(value);require(n>=p.minimum&&n<=p.maximum,"Value outside "+std::to_string(p.minimum)+".."+std::to_string(p.maximum)+": "+p.id);break;}
    case Kind::path:break;
    case Kind::absolute_path:require(value.front()=='/',"An absolute file path is required: "+p.id);break;
    case Kind::identifier:require(value.size()<=128&&matches("[A-Za-z0-9_][A-Za-z0-9_.+-]*"),"Invalid plain identifier: "+p.id);break;
    case Kind::interface:require(value.size()<=15&&matches("[A-Za-z0-9_][A-Za-z0-9_.:-]*"),"Invalid Linux interface name");break;
    case Kind::usb_device:require(value.size()<=64&&matches("[0-9]+-[0-9]+(\\.[0-9]+)*"),"Use an exact USB topology name");break;
    case Kind::input_device:require(matches("/dev/input/event[0-9]{1,3}"),"Use /dev/input/eventN");break;
    case Kind::partition_uuid:require(value.size()<=64&&matches("[A-Fa-f0-9]+(-[A-Fa-f0-9]+)*"),"Use an exact partition UUID");break;
    case Kind::color:require(matches("#[A-Fa-f0-9]{6}"),"Use #RRGGBB");break;
    case Kind::swap_size:if(value!="half"&&value!="full"){const auto n=integer(value);require(n>=1&&n<=1048576,"Swap input must be 1..1048576 MiB, half or full");}break;
    case Kind::repository:require(value.size()<=140&&matches("[A-Za-z0-9][A-Za-z0-9-]{0,38}/[A-Za-z0-9_.-]{1,100}"),"Use an exact OWNER/REPOSITORY");break;
    }
}
Value descriptor(const Action& a) {
    Value value;value["id"]=a.id;value["category"]=a.category;value["title"]=bilingual(a.title);value["description"]=bilingual(a.description);
    value["effect"]=effect_name(a.effect);value["effect_description"]=bilingual(effect_description(a.effect));
    value["access"]=access_name(a.access);value["presentation"]=a.presentation==Presentation::capture?"capture":"terminal";
    value["query_timeout_seconds"]=a.presentation==Presentation::capture?Value(a.timeout_seconds):Value(Json::nullValue);
    value["confirmation_required"]=a.effect!=Effect::read;value["physical_acceptance"]=Json::nullValue;
    value["inspect_action"]=a.inspect.empty()?Value(Json::nullValue):Value(a.inspect);
    value["preview_action"]=a.preview.empty()?Value(Json::nullValue):Value(a.preview);
    value["optional_tests_package"]=a.tests_package;value["parameters"]=Value(Json::arrayValue);
    for(const auto& p:a.parameters) {
        Value parameter;parameter["id"]=p.id;parameter["title"]=bilingual(p.title);parameter["description"]=bilingual(p.description);
        parameter["kind"]=kind_name(p.kind);parameter["default"]=p.initial;parameter["optional"]=p.optional;
        parameter["minimum"]=p.kind==Kind::integer?Value(Json::UInt64(p.minimum)):Value(Json::nullValue);
        parameter["maximum"]=p.kind==Kind::integer?Value(Json::UInt64(p.maximum)):Value(Json::nullValue);
        parameter["choices"]=Value(Json::arrayValue);
        for(const auto& c:p.choices){Value choice;choice["value"]=c.value;choice["title"]=bilingual(c.title);choice["description"]=bilingual(c.description);parameter["choices"].append(choice);}
        value["parameters"].append(parameter);
    }
    return value;
}
std::string lower(std::string text) {
    // Thread-local locale changes do not alter GTK/Qt's process-wide locale.
    static const locale_t unicode=newlocale(LC_CTYPE_MASK,"C.UTF-8",nullptr);
    if(!unicode)throw std::runtime_error("UTF-8 search locale is unavailable");
    const auto previous=uselocale(unicode);
    struct Restore {locale_t previous;~Restore(){uselocale(previous);}} restore{previous};
    std::string output;std::mbstate_t state{};
    for(size_t at=0;at<text.size();) {
        wchar_t wc{};const auto n=std::mbrtowc(&wc,text.data()+at,text.size()-at,&state);
        if(n==size_t(-1)||n==size_t(-2)||!n){output+='?';++at;state={};continue;}
        at+=n;wc=std::towlower(wc);
        switch(wc){case L'ı':wc=L'i';break;case L'ş':wc=L's';break;case L'ç':wc=L'c';break;case L'ğ':wc=L'g';break;case L'ö':wc=L'o';break;case L'ü':wc=L'u';break;}
        char bytes[MB_LEN_MAX];std::mbstate_t encoded{};const auto count=std::wcrtomb(bytes,wc,&encoded);
        if(count!=size_t(-1))output.append(bytes,count);
    }
    return output;
}
}
std::string effect_name(Effect effect) {
    switch(effect){case Effect::read:return "read";case Effect::local_file:return "local_file";case Effect::device:return "device";case Effect::packages:return "packages";case Effect::maintenance:return "maintenance";case Effect::publish:return "publish";}
    throw std::logic_error("Unknown action effect");
}
std::string access_name(Access access) {
    switch(access){case Access::user:return "user";case Access::active_session:return "active_local_session";case Access::root:return "root";}
    throw std::logic_error("Unknown action access");
}
Text effect_description(Effect effect) {
    switch(effect) {
    case Effect::read:return {"Read-only observation. It may contact existing providers but does not qualify physical hardware.","Salt okunur gözlem. Mevcut sağlayıcılara erişebilir; fiziksel donanımı doğrulamaz."};
    case Effect::local_file:return {"Writes private local state/evidence. Review the exact path and effect; existing backend guards remain authoritative.","Özel yerel durum/kanıt yazar. Tam yolu ve etkiyi inceleyin; mevcut backend denetimleri yetkilidir."};
    case Effect::device:return {"Requests an existing device/session control. Inspect current state, admission and readback; physical acceptance is separate.","Mevcut aygıt/oturum kontrolünü ister. Mevcut durumu, kabulü ve geri okumayı inceleyin; fiziksel kabul ayrıdır."};
    case Effect::packages:return {"Changes packages/repositories and possibly an admitted boot selection. Unknown RPM outcomes cannot be replayed automatically.","Paketleri/depoları ve kabul edilmiş boot seçimini değiştirebilir. Belirsiz RPM sonucu otomatik tekrarlanamaz."};
    case Effect::maintenance:return {"Opens existing guarded maintenance that can change storage or request reboot. Its original plan-bound approvals remain required.","Depolamayı değiştirebilen veya reboot isteyebilen mevcut korumalı bakımı açar. Özgün plana bağlı onaylar gereklidir."};
    case Effect::publish:return {"Explicit publication to the reviewed external destination. Inspect the generated summary before proceeding.","İncelenmiş harici hedefe açık yayımlama. Devam etmeden üretilmiş özeti inceleyin."};
    }throw std::logic_error("Unknown action effect");
}
Plan plan(std::string_view id,const Values& supplied) {
    const auto& a=action(id);auto values=defaults(a);
    for(const auto& [key,value]:supplied){require(values.contains(key),"Unknown parameter: "+key);values[key]=value;}
    for(const auto& p:a.parameters)validate(p,values.at(p.id));
    Plan result{a.id,values,{UKE_CLIENT_PROGRAM},a.effect,a.access,a.presentation,a.timeout_seconds,a.effect!=Effect::read};
    if(a.presentation==Presentation::capture)result.argv.push_back("--json");
    for(auto argument:a.arguments) {
        const auto begin=argument.find('{');
        if(begin!=argument.npos) {
            const auto end=argument.find('}',begin);require(end!=argument.npos,"Invalid catalog placeholder");
            require(argument.find('{',end+1)==argument.npos,"Repeated catalog placeholder");
            const auto key=argument.substr(begin+1,end-begin-1);require(values.contains(key),"Unknown catalog placeholder");
            if(values.at(key).empty())continue;
            argument.replace(begin,end-begin+1,values.at(key));
        }
        result.argv.push_back(std::move(argument));
    }
    return result;
}
void validate_value(const Parameter& parameter,const std::string& value){validate(parameter,value);}
std::vector<std::string> search(std::string_view query,std::string_view category,Language language) {
    std::istringstream words(lower(std::string(query)));std::vector<std::string> needles;
    for(std::string word;words>>word;)needles.push_back(word);
    std::vector<std::string> result;
    for(const auto& a:actions()) {
        if(!category.empty()&&a.category!=category)continue;
        auto haystack=lower(a.id+" "+a.title.get(language)+" "+a.description.get(language)+" "+a.title.en+" "+a.title.tr);
        for(const auto& p:a.parameters)haystack+=" "+lower(p.id+" "+p.title.get(language));
        if(std::all_of(needles.begin(),needles.end(),[&](const auto& word){return haystack.find(word)!=haystack.npos;}))result.push_back(a.id);
    }
    return result;
}
std::string catalog_json() {
    Value value;value["schema"]="org.uke.control.actions.v1";value["api_version"]=api_version;
    value["application_version"]=application_version;value["metadata_only"]=true;value["physical_acceptance"]=Json::nullValue;
    value["categories"]=Value(Json::arrayValue);value["actions"]=Value(Json::arrayValue);
    for(const auto& c:categories()){Value item;item["id"]=c.id;item["title"]=bilingual(c.title);item["description"]=bilingual(c.description);value["categories"].append(item);}
    for(const auto& a:actions())value["actions"].append(descriptor(a));
    return encode(value);
}
std::string describe_json(std::string_view id){auto value=descriptor(action(id));value["schema"]="org.uke.control.action.v1";value["api_version"]=api_version;return encode(value);}
std::string display_command(const std::vector<std::string>& arguments) {
    std::string text;
    for(const auto& argument:arguments) {
        if(!text.empty())text+=' ';
        text+='\'';for(char c:argument)text+=c=='\''?"'\\''":std::string(1,c);text+='\'';
    }
    return text;
}
std::string plan_json(const Plan& p) {
    const auto checked=plan(p.action_id,p.values);Value value;
    value["schema"]="org.uke.control.action-plan.v1";value["api_version"]=api_version;value["action_id"]=checked.action_id;
    value["argv"]=Value(Json::arrayValue);for(const auto& argument:checked.argv)value["argv"].append(argument);
    value["parameters"]=Value(Json::objectValue);for(const auto& [key,input]:checked.values)value["parameters"][key]=input;
    value["effect"]=effect_name(checked.effect);value["access"]=access_name(checked.access);
    value["terminal_required"]=checked.presentation==Presentation::terminal;
    value["confirmation_required"]=checked.confirmation_required;value["executed"]=false;value["physical_acceptance"]=Json::nullValue;
    return encode(value);
}
std::string format_output(const std::string& output,Language language,bool raw) {
    Value document;if(!decode(output,document))return utf8(output);
    if(raw){Json::StreamWriterBuilder writer;writer["indentation"]="  ";writer["emitUTF8"]=true;return Json::writeString(writer,document);}
    const bool tr=language==Language::turkish;
    static const std::map<std::string,Text> labels{
        {"available",{"Available","Kullanılabilir"}},{"availability",{"Availability","Kullanılabilirlik"}},{"component",{"Component","Bileşen"}},
        {"physical_acceptance",{"Physical acceptance","Fiziksel kabul"}},{"observed_boottime_ns",{"Observation boottime (ns)","Gözlem boot zamanı (ns)"}},
        {"source_sample_age_ms",{"Source sample age (ms)","Kaynak örnek yaşı (ms)"}},{"collection_span_ms",{"Collection duration (ms)","Toplama süresi (ms)"}},
        {"provider_verified",{"Provider identity verified","Sağlayıcı kimliği doğrulandı"}},{"profile_qualified",{"Profile qualified","Profil doğrulandı"}},
        {"query_exit_status",{"Query exit status","Sorgu çıkış kodu"}},{"snapshot_coherent",{"Snapshot coherent","Görüntü tutarlı"}},
        {"value",{"Value","Değer"}},{"validity",{"Validity","Geçerlilik"}},{"unit",{"Unit","Birim"}},{"reason",{"Reason","Neden"}},
        {"requested_percent",{"Requested brightness (%)","İstenen parlaklık (%)"}},{"brightness_percent",{"Applied brightness (%)","Uygulanan parlaklık (%)"}},
        {"status",{"Status","Durum"}},{"output",{"Output","Çıktı"}},{"error",{"Error","Hata"}},{"data",{"Observations","Okumalar"}},
        {"pending",{"Pending channel","Bekleyen kanal"}},{"pending_readable",{"Private pending state readable","Özel bekleyen durum okunabilir"}},
        {"selected",{"Selected channel","Seçili kanal"}},{"operation_id",{"Operation ID","İşlem kimliği"}},{"phase",{"Phase","Aşama"}},
        {"root_available_bytes",{"Available root space (bytes)","Boş kök alanı (bayt)"}},{"root_capacity_bytes",{"Root capacity (bytes)","Kök kapasitesi (bayt)"}},
        {"application_version",{"Application version","Uygulama sürümü"}},{"running_kernel",{"Running kernel","Çalışan kernel"}},
        {"schema",{"Data schema","Veri şeması"}},{"read_only",{"Read-only query","Salt okunur sorgu"}},{"sample_age_unknown",{"Sample age unknown","Örnek yaşı bilinmiyor"}}
    };
    const auto label=[&](std::string name){if(const auto it=labels.find(name);it!=labels.end())return it->second.get(language);std::replace(name.begin(),name.end(),'_',' ');return name;};
    std::ostringstream text;unsigned nodes=0;
    std::function<void(const Value&,const std::string&,unsigned)> render;
    render=[&](const Value& value,const std::string& name,unsigned depth) {
        if(++nodes>4096||depth>24){if(nodes==4097)text<<(tr?"[Görüntüleme sınırı; ham JSON'u inceleyin]":"[Display bound; inspect raw JSON]")<<'\n';return;}
        const std::string prefix(depth*2,' ');
        if(value.isObject()&&!value.empty()) {
            if(!name.empty())text<<prefix<<label(name)<<":\n";
            for(const auto& key:value.getMemberNames())render(value[key],key,depth+(name.empty()?0:1));
        } else if(value.isArray()&&!value.empty()) {
            text<<prefix<<label(name)<<":\n";for(Json::ArrayIndex i=0;i<value.size();++i)render(value[i],"["+std::to_string(i)+"]",depth+1);
        } else {
            text<<prefix;if(!name.empty())text<<label(name)<<": ";
            if(value.isNull())text<<(tr?"Bildirilmedi":"Not reported");
            else if(value.isBool())text<<(value.asBool()?(tr?"Evet":"Yes"):(tr?"Hayır":"No"));
            else if(value.isString())text<<utf8(value.asString());
            else text<<encode(value);
            text<<'\n';
        }
    };
    render(document,"",0);return text.str();
}
std::string receipt_json(const Result& result,const Values& values) {
    Value value;value["schema"]="org.uke.control.client-receipt.v1";value["application_version"]=application_version;
    value["action_id"]=utf8(result.action_id);value["exit_status"]=result.exit_status;value["running"]=result.running;
    value["cancelled"]=result.cancelled;value["timed_out"]=result.timed_out;value["truncated"]=result.truncated;
    value["elapsed_ms"]=Json::UInt64(result.elapsed_ms);value["output"]=utf8(result.output);value["stderr"]=utf8(result.error);
    value["physical_acceptance"]=Json::nullValue;value["parameters"]=Value(Json::objectValue);
    for(const auto& [key,input]:values)value["parameters"][utf8(key)]=utf8(input);
    return encode(value);
}
void save_receipt(const std::string& path,const Result& result,const Values& values) {
    Parameter filename;filename.id="receipt";filename.kind=Kind::path;
    validate(filename,path);
    require(!result.running,"Cannot save an unfinished result");
    const auto data=receipt_json(result,values)+'\n';
    const int fd=open(path.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW|O_CLOEXEC,0600);
    if(fd<0)throw std::runtime_error("Cannot create a new private result file: "+std::string(strerror(errno)));
    struct Close{int fd;~Close(){close(fd);}} close_file{fd};
    if(fchmod(fd,0600))throw std::runtime_error("Cannot protect the result file");
    size_t written=0;
    while(written<data.size()){const auto n=write(fd,data.data()+written,data.size()-written);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Result write incomplete; retain the private partial file");written+=size_t(n);}
    if(fsync(fd))throw std::runtime_error("Result synchronization failed; retain the private file");
}
}
