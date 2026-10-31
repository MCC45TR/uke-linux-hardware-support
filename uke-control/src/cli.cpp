// SPDX-License-Identifier: MIT
#include "cli.hpp"
#include "error.hpp"
#include "sysfs.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <systemd/sd-json.h>
namespace uke::cli {
Arguments arguments(int argc,char** argv) {
    Arguments result;result.words.emplace_back("uke-control");bool options=true;
    for(int i=1;i<argc;++i) {
        const std::string word=argv[i];
        if(options && word=="--"){options=false;continue;}
        if(options && word=="--json") {
            if(result.json)throw UsageError("--json was specified more than once");
            result.json=true;
        } else if(options && (word=="--help" || word=="-h"))result.help=true;
        else if(options && word=="--version")result.version=true;
        else result.words.push_back(word);
    }
    auto& w=result.words;
    if(w.size()<2)return result;
    if(w[1]=="branch" || w[1]=="brunch")w[1]="channel";
    if(w[1]=="channel" && w.size()>2 && (w[2]=="stable" || w[2]=="test" || w[2]=="unstable"))w.insert(w.begin()+2,"set");
    if(w[1]=="diagnostic")w[1]="diagnostics";
    if(w.size()>2 && w[1]=="set" && w[2]=="cdc"){w.erase(w.begin()+1);}
    if(w.size()>3 && w[1]=="usb" && w[2]=="request" && w[3]=="data"){w[2]="role";w.erase(w.begin()+3);}
    return result;
}
namespace {
struct Group {const char* name;const char* summary;const char* usage;};
constexpr Group groups[]={
    {"tui","Bilingual native terminal interface with guided parameters", "tui [--language tr|en] [--read-only] [--no-color] [--start CATEGORY|ACTION] [--fixture FILE]\n\nRequires uke-control-tui and an interactive UTF-8 terminal. F1 opens full help.\nRun as the session user; privileged actions authorize only their validated command.\nFixture mode executes no commands. Read-only mode also disables receipt writes."},
    {"actions","Shared frontend catalog and validated plans (metadata only)", "actions\nactions describe ID\nactions plan ID KEY=VALUE...\n\nUse --json for versioned bilingual metadata and exact argv plans.\nThese commands do not query hardware, execute plans or authorize changes."},
    {"status","Device overview", "status"},
    {"microphone","Internal array status and saved channel preference", "microphone status|list\nmicrophone select auto|uke-amic1|uke-amic3|uke-amic4|uke-amic5\nmicrophone processing status\n\nManual selection affects new implicit mono/stereo streams. Explicit targets and raw four-channel capture remain available.\nAutomatic camera/orientation profiles and audio effects require separate qualification."},
    {"overview","Read-only component snapshots and private snapshot files", "overview [battery|charge|display|usb|desktop|hardware|sensors|audio|camera|nfc|sleep|storage|updates|recovery|workload]\noverview save FILE [COMPONENT]\noverview --fixture FILE\n\nSnapshots separate observations from physical acceptance. Fixture mode performs no queries."},
    {"capabilities","Discover the versioned read-only interface", "capabilities\n\nStatic command metadata does not qualify hardware controls."},
    {"complete","Print command or global-option completion candidates", "complete [PREFIX]\n\nOne candidate per line; --json returns the versioned completion object."},
    {"compare","Compare two local overview snapshots", "compare BEFORE AFTER\n\nMissing data stays distinct from an unchanged value. Correlation does not establish cause."},
    {"support","Create a reviewed public summary from a private snapshot", "support export SNAPSHOT NEW-FILE\n\nExports only fixed device-family/application metadata and component availability. No upload is performed."},
    {"roots","Read-only root candidate discovery and mounted-root inspection", "roots list\nroots inspect PARTUUID\n\nDoes not mount, repair or execute a target. Inspection requires one verified current mount."},
    {"battery","Battery readings, pack health, cells and charging protocol", "battery status\nbattery health\nbattery cells\nbattery protocol\n\nReuses uke-battery and uke-charge-control. Reports unavailable values explicitly.\nPack health, vendor SOH and calculated retention are separate observations.\nIndividual cell health requires individual cell measurements; it is not inferred.\nA reported USB class is not a negotiated PD revision or a verified QC/MI session."},
    {"nfc","Linux NFC controller and radio permission", "nfc status\nnfc on|off\n\nOn/off changes only a unique NFC controller's rfkill permission and requires root.\nIt does not start tag polling or provide card emulation.\nA passive tap-to-share feature does not establish a controllable Linux reader."},
    {"channel","COPR channel selection and synchronization", "channel status\nchannel set stable|test|unstable [--dry-run] [--yes]\nchannel sync [--dry-run] [--yes]"},
    {"update","Synchronize the channel and update distribution packages", "update [--check] [--yes]"},
    {"swap","Swap status and saved policy", "swap status\nswap set half|full|MiB\nswap apply"},
    {"inventory","Partition and filesystem inventory", "inventory [--refresh]"},
    {"charge","Charging status, qualification and guarded requests", "charge status\ncharge capabilities|admission|protocols|protocol-inspect|settings|protection|protection-status\ncharge modes\ncharge plan firmware|charge80|hold70-80|hold80-90|slow|automatic\ncharge plan temporary-full MINUTES\ncharge mode firmware|charge80|hold70-80|hold80-90|slow|automatic\ncharge review /ABSOLUTE/REVIEW_FILE\ncharge restore-current\ncharge limit PERCENT\ncharge enable-cap80|apply-cap80|moisture-enable\ncharge notifications\n\nReview installs expiring operator evidence; it does not start charging control.\nRestore-current uses the reviewed positive FCC value and is not a factory reset."},
    {"usb","USB topology, roles and per-device power policy", "usb status\nusb watch [SECONDS]\nusb role host|device\nusb power DEVICE performance|auto\nusb transition status|recover\n\nRecovery requires root and stable, unused Uke role readbacks. Unknown legacy records are preserved for review."},
    {"cdc","Manual USB serial export", "cdc on|off [--persistent]"},
    {"ethernet","Ethernet capabilities and link profiles", "ethernet status IFACE\nethernet profile IFACE auto|1000-full|100-full|10-full"},
    {"display","Display stream, lane profiles and VRR minimum", "display bits\ndisplay profiles\ndisplay profile usb3-display|display4\ndisplay lane-limit 1|2|4\ndisplay compression [on|off]\ndisplay vrr-min [status|configured]\ndisplay vrr-min set HZ\ndisplay sensor"},
    {"desktop","Original desktop interfaces and KDE policies", "desktop status\ndesktop plasma\ndesktop rotation on|off\ndesktop brightness on|off"},
    {"hardware","Cached chipset, audio, media and input paths", "hardware status"},
    {"firmware","Cached remote processors and SCMI discovery", "firmware status"},
    {"wake","Driver wake permissions", "wake status [SOURCE]\nwake set SOURCE on|off\nwake apply\n\nStatus lists available source names, including supported usb-device-PORT, lid-open and double-tap.\nSaved choices override supported USB, Bluetooth, lid and gesture wake defaults.\nPermission readback is not physical wake validation."},
    {"flashlight","Torch on/off and requested brightness", "flashlight status\nflashlight on|off\nflashlight set 1..100\n\nThe retained 20 mA limit provides two lit steps: 1–99 requests use 50%; 100 uses 100%.\nCLI zero also means off. On restores the in-service request, default 100.\nAn active local user may request changes without sudo."},
    {"led","Kernel notification LED requests", "led status\nled auto\nled test\nled set '#RRGGBB' [0..100]\n\nTest requests one ten-second color transition. Auto restores kernel policy.\nAn active local user may request changes without sudo."},
    {"sensors","Exported sensor channels and live readings", "sensors [list] [-n SECONDS] [--count=N] [--once]\n\nDefault: one snapshot. -n5 also repeats every five seconds. Ctrl-C stops.\n--json emits one JSON object per sample; polling time is not acquisition time."},
    {"diagnostics","Collect a private local diagnostic log", "diagnostics all|TOPIC\n\nTopics: all system hardware kernel boot packages services wifi bluetooth\n        sensors sleep power usb display audio input camera storage firmware security\n\nWrites uke-diagnostic-TOPIC.log in the current directory, mode 0600.\nExisting files are never overwritten. Logs can contain private identifiers; review before sharing."},
    {"report","Collect a private report bundle", "report\nreport github DIRECTORY OWNER/REPOSITORY\n\nThe GitHub command publishes only the generated anonymous summary."},
    {"resize","Check or prepare offline system filesystem growth", "resize system --check\nresize system\n\nThe second form requires interactive confirmation and may request a normal reboot.\n--json is supported with --check only."},
    {"singleboot","Restore the proven userdata allocation", "singleboot [--dry-run]\nsingleboot status\n\nRoot and an interactive terminal are required. Original GPT and ownership proof protect OEM partitions.\n--dry-run creates a private metadata journal only; it never prepares a reboot or disk write.\n--json is supported for status only."},
    {"multiboot","Allocate volumes after userdata in its original extent", "multiboot [--dry-run]\nmultiboot status\n\nRoot and a mouse-capable dialog terminal wizard are required.\nErasing userdata or existing project volumes requires two plan-bound approvals.\nKeep a separate file backup and a durable external ext4/XFS metadata journal.\n--json is supported for status only."},
    {"test","Development diagnostics from the optional tests package", "test display [SECONDS] [--csv]\ntest cadence [MAXIMUM-FPS] [--csv]\ntest touch /dev/input/eventN SECONDS\ntest sensors [SECONDS]\ntest desktop|hardware\ntest discover [all|DATATYPE]\ntest sample DATATYPE\n\n--json is supported for desktop and hardware snapshots. Numeric data is not physical acceptance."}
};
std::string safe(std::string_view value) {
    std::ostringstream out;
    for(unsigned char c:value) {
        if((c<32 && c!='\n' && c!='\t') || c==127)out<<"\\x"<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(c)<<std::dec;
        else out<<char(c);
    }
    return out.str();
}
std::string label(std::string key) {
    if(key=="usb")return "USB";
    if(key=="rgb")return "Notification LED";
    if(key=="scmi")return "SCMI";
    if(key=="physical_acceptance")return "Physical validation";
    if(key=="requested_percent")return "Requested brightness (%)";
    if(key=="brightness_percent")return "Applied brightness (%)";
    if(key=="max_microamp")return "Maximum current (µA)";
    if(key=="io_error" || key=="kernel_io_error")return "Kernel I/O error";
    std::string unit;
    for(const auto& [suffix,name]:{std::pair{"_bytes","bytes"},{"_percent","%"},{"_microamp","µA"},{"_hz","Hz"},{"_ns","ns"},{"_ms","ms"},{"_uv","µV"},{"_ua","µA"},{"_deci_c","0.1 °C"}}) {
        if(key.ends_with(suffix)){key.resize(key.size()-std::char_traits<char>::length(suffix));unit=" ("+std::string(name)+")";break;}
    }
    std::replace(key.begin(),key.end(),'_',' ');
    if(!key.empty())key[0]=char(std::toupper(static_cast<unsigned char>(key[0])));
    return safe(key)+unit;
}
using Json=std::unique_ptr<sd_json_variant,decltype(&sd_json_variant_unref)>;
Json parse(const std::string& text) {
    sd_json_variant* value=nullptr;
    const auto result=sd_json_parse(text.c_str(),static_cast<sd_json_parse_flags_t>(0),&value,nullptr,nullptr);
    if(result<0)return Json(nullptr,sd_json_variant_unref);
    return Json(value,sd_json_variant_unref);
}
std::string scalar(sd_json_variant* value) {
    if(sd_json_variant_is_null(value))return "not reported";
    if(sd_json_variant_is_boolean(value))return sd_json_variant_boolean(value)?"yes":"no";
    if(sd_json_variant_is_string(value))return safe(sd_json_variant_string(value));
    char* text=nullptr;
    if(sd_json_variant_format(value,static_cast<sd_json_format_flags_t>(0),&text)<0 || !text)throw std::runtime_error("Cannot format backend value");
    const std::unique_ptr<char,decltype(&std::free)> owned(text,std::free);return text;
}
void render(std::ostream& out,sd_json_variant* value,unsigned depth,std::string_view key={}) {
    if(depth>64)throw std::runtime_error("Backend response exceeds the display nesting limit");
    if(sd_json_variant_is_string(value)) {
        const std::string text=sd_json_variant_string(value);auto nested=parse(text);
        if(nested && (sd_json_variant_is_object(nested.get()) || sd_json_variant_is_array(nested.get()))) {
            if(!key.empty())out<<std::string(depth*2,' ')<<label(std::string(key))<<":\n";
            render(out,nested.get(),depth+1);return;
        }
    }
    if(sd_json_variant_is_object(value) && !key.empty()) {
        out<<std::string(depth*2,' ')<<label(std::string(key))<<":\n";++depth;
    }
    const std::string indent(depth*2,' ');
    if(sd_json_variant_is_object(value)) {
        if(!sd_json_variant_elements(value))out<<indent<<"No entries\n";
        for(size_t i=0;i<sd_json_variant_elements(value);i+=2) {
            const char* name=sd_json_variant_string(sd_json_variant_by_index(value,i));
            if(!name)throw std::runtime_error("Invalid backend field name");
            if(std::string_view(name)=="schema" || std::string_view(name)=="schema_version" || std::string_view(name)=="kind")continue;
            render(out,sd_json_variant_by_index(value,i+1),depth,name);
        }
    } else if(sd_json_variant_is_array(value)) {
        if(!key.empty())out<<indent<<label(std::string(key))<<":\n";
        if(!sd_json_variant_elements(value))out<<indent<<"  None\n";
        for(size_t i=0;i<sd_json_variant_elements(value);++i) {
            auto* item=sd_json_variant_by_index(value,i);
            if(sd_json_variant_is_object(item) || sd_json_variant_is_array(item)) {
                out<<indent<<"  Item "<<i+1<<":\n";render(out,item,depth+2);
            } else out<<indent<<"  "<<scalar(item)<<'\n';
        }
    } else {
        std::string text=scalar(value);
        if(std::string_view(key)=="reason" || std::string_view(key)=="state" || std::string_view(key)=="effective_state")std::replace(text.begin(),text.end(),'_',' ');
        if(sd_json_variant_is_null(value) && key=="physical_acceptance")text="not established";
        out<<indent;if(!key.empty())out<<label(std::string(key))<<": ";
        std::istringstream lines(text);std::string line;bool first=true;
        while(std::getline(lines,line)){
            auto nested=parse(line);
            if(nested && (sd_json_variant_is_object(nested.get()) || sd_json_variant_is_array(nested.get()))) {
                if(first)out<<'\n';
                render(out,nested.get(),depth+1);
            } else {if(!first)out<<indent<<"  ";out<<line<<'\n';}
            first=false;
        }
        if(first)out<<"(empty)\n";
    }
}
}
std::vector<std::string> command_names() {
    std::vector<std::string> names;for(const auto& group:groups)names.emplace_back(group.name);return names;
}
std::string help(std::string_view group) {
    std::ostringstream out;out<<"Usage: uke-control ";
    if(group.empty()) {
        out<<"[OPTIONS] COMMAND [ARGUMENTS]\n\nUke device settings and diagnostics.\n\nCommands:\n";
        for(const auto& entry:groups)out<<"  "<<std::left<<std::setw(14)<<entry.name<<entry.summary<<'\n';
        out<<"\nRun 'uke-control COMMAND --help' for command syntax.\n";
    } else {
        const auto found=std::find_if(std::begin(groups),std::end(groups),[&](const Group& entry){return group==entry.name;});
        if(found==std::end(groups))throw UsageError("unknown command '"+safe(group)+"'; see 'uke-control --help'");
        std::istringstream lines(found->usage);std::string line;bool syntax=true,first=true;
        while(std::getline(lines,line)) {
            if(line.empty())syntax=false;
            if(syntax && !first)out<<"       uke-control ";
            out<<line<<'\n';first=false;
        }
    }
    out<<"\nOptions:\n  -h, --help    Show help without running a device operation\n  --version     Show version\n  --json        Machine-readable output; explicit even when redirected\n\n";
    if(group.empty() || group=="channel" || group=="update")out<<"Package changes require root. With --json, use --dry-run/--check or explicit --yes.\n";
    out<<"Hardware changes retain their existing privilege and qualification checks.\n"
          "Exit status: 0 success; 64 command usage; other nonzero values report operation failure.\n";
    return out.str();
}
std::string human(const std::string& data) {
    auto value=parse(data);std::ostringstream out;
    if(value) {
        auto* schema=sd_json_variant_by_key(value.get(),"schema");
        if(sd_json_variant_is_string(schema) && std::string_view(sd_json_variant_string(schema))=="org.uke.control.lights.v1") {
            auto* torch=sd_json_variant_by_key(value.get(),"flashlight");
            if(!sd_json_variant_is_object(torch))throw std::runtime_error("Malformed torch response from backend");
            render(out,torch,0);
            auto* physical=sd_json_variant_by_key(value.get(),"physical_acceptance");
            if(physical)render(out,physical,0,"physical_acceptance");
        } else render(out,value.get(),0);
    }
    else {
        std::istringstream lines(data);std::string line;
        while(std::getline(lines,line)) {
            auto nested=parse(line);
            if(nested)render(out,nested.get(),0);
            else {
                const auto first=line.find_first_not_of(" \t");
                if(first!=std::string::npos && (line[first]=='{' || line[first]=='['))throw std::runtime_error("Malformed structured response from backend");
                out<<safe(line)<<'\n';
            }
        }
    }
    return out.str();
}
void print(const std::string& data,bool json,std::string_view title) {
    if(json) {
        auto value=parse(data);
        std::cout<<(value?data:"{\"schema\":\"org.uke.control.result.v1\",\"output\":"+sysfs::quote(data)+"}");
        if(data.empty() || !value || data.back()!='\n')std::cout<<'\n';
    } else {
        if(!title.empty())std::cout<<safe(title)<<'\n';
        std::cout<<human(data);
    }
}
bool json_requested(int argc,char** argv){return json_requested_inline(argc,argv);}
int error(const std::string& message,bool json,int status) {
    if(json)return json_failure(message,status);
    std::cerr<<"uke-control: "<<safe(message)<<'\n';return status;
}
}
