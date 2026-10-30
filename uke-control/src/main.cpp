// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include "storage.hpp"
#include "cli.hpp"
#include "charge-policy.hpp"
#include "front-color.hpp"
#include "sensors.hpp"
#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include "charge-notify.hpp"
#include "desktop.hpp"
#include "plasma.hpp"
#include "wake.hpp"
#include "diagnostic.hpp"
#include "lights.hpp"
#include "vrr.hpp"
#include "usb-request.hpp"
#include "link-policy.hpp"
static int run(int argc,char** argv,bool as_json) {
    try {
        if(argc==4&&std::string_view(argv[1])=="charge"&&std::string_view(argv[2])=="mode") {
            auto result=uke::charge_control_mode(argv[3]);uke::cli::print(result.output,as_json);return result.status;
        }
        if(const int result=uke::charge::policy_command(argc,argv,as_json);result>=0)return result;
        std::string command=argv[1];
        if(command=="singleboot"||command=="multiboot") {
            execv("/usr/libexec/uke-control/layout-manager",argv);
            throw std::runtime_error("The native boot layout manager is unavailable");
        }
        if(command=="flashlight" || command=="led")return uke::lights::command(command=="led",argc-2,argv+2,as_json);
        if(command=="battery") {
            if(argc!=3||(std::string_view(argv[2])!="status"&&std::string_view(argv[2])!="health"&&std::string_view(argv[2])!="cells"&&std::string_view(argv[2])!="protocol"))throw uke::cli::UsageError("expected battery status|health|cells|protocol");
            const auto result=uke::battery_query(argv[2]);uke::cli::print(result.output,as_json);return result.status;
        }
        if(command=="nfc") {
            if(argc!=3||(std::string_view(argv[2])!="status"&&std::string_view(argv[2])!="on"&&std::string_view(argv[2])!="off"))throw uke::cli::UsageError("expected nfc status|on|off");
            if(std::string_view(argv[2])!="status")uke::nfc_set(std::string_view(argv[2])=="on");
            uke::cli::print(uke::nfc_status(),as_json);return 0;
        }
        if(command=="resize") {
            const int count=argc-(as_json?1:0);
            if((count!=3&&count!=4) || std::string_view(argv[2])!="system" || (count==4&&std::string_view(argv[3])!="--check"))throw uke::cli::UsageError("expected resize system [--check]");
            if(as_json && count!=4)throw uke::cli::UsageError("--json requires resize system --check; preparation requires an interactive terminal");
            const char* arguments[]={"/usr/libexec/uke-control/resize-system",count==4?"--check":"--interactive",as_json?"--json":nullptr,nullptr};
            execv(arguments[0],const_cast<char* const*>(arguments));
            throw std::runtime_error("The native resize helper is unavailable");
        }
        if(command=="storage" || command=="chroot") {
            const std::string expected=command=="storage"?"status":"plan";
            if((argc!=3 && argc!=4) || std::string_view(argv[2])!=expected)
                throw uke::cli::UsageError("expected "+command+" "+expected+" [/dev/DEVICE]; execution is not implemented");
            uke::cli::print(uke::storage::snapshot("/",argc==4?argv[3]:""),as_json);return 0;
        }
        if(command=="diagnostics")return uke::diagnostic::command(argc-2,argv+2,as_json);
        if(command=="wake") {
            if(argc==4&&std::string_view(argv[2])=="status"){uke::cli::print(uke::wake::snapshot("/sys",argv[3]),as_json);return 0;}
            if(argc==3&&std::string_view(argv[2])=="check") return uke::wake::applicable()?0:1;
            if(argc==3&&std::string_view(argv[2])=="status") {uke::cli::print(uke::wake_status(),as_json);return 0;}
            if(argc==3&&std::string_view(argv[2])=="apply") {uke::wake_apply();return 0;}
            if(argc==5&&std::string_view(argv[2])=="set") {
                const std::string_view value=argv[4];
                if(value!="on"&&value!="off")throw std::runtime_error("Wake permission must be on or off");
                uke::wake_set(argv[3],value=="on");uke::cli::print(uke::wake_status(),as_json);return 0;
            }
        }
        if(command=="desktop"&&argc==3&&std::string_view(argv[2])=="status"){uke::cli::print(uke::desktop::status(),as_json);return 0;}
        if(command=="desktop"&&argc==3&&std::string_view(argv[2])=="plasma"){uke::cli::print(uke::desktop::plasma_status(),as_json);return 0;}
        if(command=="desktop"&&argc==4){auto result=uke::desktop::plasma_set(argv[2],argv[3]);uke::cli::print(result.output,as_json);return result.status;}
        if(command=="channel"||command=="update") {
            execv("/usr/libexec/uke-control/packages",argv+1);
            throw std::runtime_error("The native package manager helper is missing");
        }
        if(command=="sensors")return uke::sensors::command(argc-2,argv+2,as_json);
        if(command=="display"&&argc==3&&std::string_view(argv[2])=="bits") {
            uke::cli::print(uke::links::display_bits_status("/"),as_json);return 0;
        }
        if(command=="display"&&argc==3&&std::string_view(argv[2])=="profiles") {
            uke::cli::print(uke::usb::display_profiles("/"),as_json);return 0;
        }
        if(command=="display"&&argc==4&&std::string_view(argv[2])=="lane-limit") {
            const auto result=uke::display_lane_limit(argv[3]);uke::cli::print(result.output,as_json);return result.status;
        }
        if(command=="display"&&argc==3&&std::string_view(argv[2])=="compression") {
            std::cout<<uke::links::compression_status("/");return 0;
        }
        if(command=="display"&&argc==4&&std::string_view(argv[2])=="compression") {
            const auto result=uke::display_compression(argv[3]);std::cout<<result.output;return result.status;
        }
        if(command=="ethernet"&&argc==4&&std::string_view(argv[2])=="status") {
            uke::cli::print(uke::links::ethernet_status(argv[3]),as_json);return 0;
        }
        if(command=="ethernet"&&argc==5&&std::string_view(argv[2])=="profile") {
            const auto result=uke::ethernet_profile_set(argv[3],argv[4]);uke::cli::print(result.output,as_json);return result.status;
        }
        if(command=="display"&&argc==4&&std::string_view(argv[2])=="profile") {
            const auto result=uke::display_profile_set(argv[3]);
            uke::cli::print(result.output,as_json);return result.status;
        }
        if(command=="display"&&argc==3&&std::string_view(argv[2])=="sensor") {
            uke::cli::print(uke::display::front_color_status(),as_json);return 0;
        }
        if(command=="display"&&argc>=3&&std::string_view(argv[2])=="vrr-min") {
            if(argc==3||(argc==4&&std::string_view(argv[3])=="status")) {
                if(geteuid()!=0)throw std::runtime_error("Use sudo uke-control display vrr-min status to read the current driver floor");
                uke::cli::print(uke::vrr::status(),as_json);return 0;
            }
            if(argc==4&&std::string_view(argv[3])=="configured") {
                const auto value=uke::wake::applicable()?uke::vrr::configured():std::nullopt;
                uke::cli::print("{\"configured_minimum_hz\":"+(value?std::to_string(*value):"null")+"}\n",as_json);return 0;
            }
            if(argc==5&&std::string_view(argv[3])=="set") {
                uke::display_vrr_set(uke::vrr::minimum(argv[4]));
                uke::cli::print(uke::vrr::status(),as_json);
                std::cerr<<"Saved for the next boot. Restart to apply a changed minimum.\n";return 0;
            }
            throw std::runtime_error("Usage: uke-control display vrr-min [status|set HZ]");
        }
        if(command=="cdc"&&argc==3&&std::string_view(argv[2])=="apply") {
            auto result=uke::cdc_apply();uke::cli::print(result.output,as_json);return result.status;
        }
        if(command=="cdc"&&(argc==3||argc==4)) {
            const std::string_view setting=argv[2];
            if(setting!="on"&&setting!="off")throw uke::cli::UsageError("CDC setting must be on or off");
            if(argc==4&&std::string_view(argv[3])!="--persistent")throw uke::cli::UsageError("expected --persistent");
            auto result=argc==4?uke::cdc_persistent(setting=="on"):uke::cdc_set(setting=="on");
            uke::cli::print(result.output,as_json);return result.status;
        }
        if(command=="test"){execv("/usr/libexec/uke-control/tests",argv+1);throw std::runtime_error("Install uke-control-tests from the development repository");}
        if(command=="--self-test"&&argc==2){uke::self_test();uke::cli::print("{\"backend_self_test_passed\":true,\"device_access\":false}",as_json);return 0;}
        if(command=="status"&&argc==2){uke::cli::print(uke::status(),as_json);return 0;}
        if(command=="firmware"&&argc==3&&std::string_view(argv[2])=="status"){uke::cli::print(uke::firmware_status(),as_json);return 0;}
        if(command=="hardware"&&argc==3&&std::string_view(argv[2])=="status"){uke::cli::print(uke::hardware_status(),as_json);return 0;}
        if(command=="boot"&&argc==2){uke::boot();uke::cli::print("{\"startup_policies_applied\":true}",as_json);return 0;}
        if(command=="inventory"&&(argc==2||(argc==3&&std::string_view(argv[2])=="--refresh"))){uke::cli::print(uke::inventory(argc==3),as_json);return 0;}
        if(command=="swap") {
            if(argc==3&&std::string_view(argv[2])=="status"){uke::cli::print(uke::swap_status(),as_json);return 0;}
            if(argc==3&&std::string_view(argv[2])=="apply"){uke::swap_apply();uke::cli::print(uke::swap_status(),as_json);return 0;}
            if(argc==4&&std::string_view(argv[2])=="set"){uke::swap_set(argv[3]);uke::cli::print(uke::swap_status(),as_json);return 0;}
        }
        if(command=="charge") {
            if(argc==4&&std::string_view(argv[2])=="review") {
                auto result=uke::charge_review(argv[3]);uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="restore-current") {
                auto result=uke::charge_control_mode("profile-restore");uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="notifications") {uke::charge::moisture_notifications();return 0;}
            if(argc==3&&std::string_view(argv[2])=="moisture-enable") {
                auto result=uke::charge_enable_moisture();uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="enable-cap80") {
                auto result=uke::charge_enable_preconfigured_cap();uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="apply-cap80") {
                auto result=uke::charge_apply_preconfigured_cap();uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&(std::string_view(argv[2])=="capabilities"||std::string_view(argv[2])=="admission"||std::string_view(argv[2])=="protocols"||std::string_view(argv[2])=="protocol-inspect"||std::string_view(argv[2])=="settings"||std::string_view(argv[2])=="protection"||std::string_view(argv[2])=="protection-status")) {
                auto result=uke::charge_control_query(argv[2]);uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="status"){uke::cli::print(uke::charge_status(),as_json);return 0;}
            if(argc==4&&std::string_view(argv[2])=="limit") {
                unsigned value{};std::string_view arg=argv[3];auto p=std::from_chars(arg.data(),arg.data()+arg.size(),value);
                if(p.ec!=std::errc{}||p.ptr!=arg.data()+arg.size()||value<1||value>100)throw std::runtime_error("Charge limit must be 1..100 percent");
                if(value==80){auto result=uke::charge_control_mode("charge80");uke::cli::print(result.output,as_json);return result.status;}
                uke::charge_set(value);uke::cli::print(uke::charge_status(),as_json);return 0;
            }
        }
        if(command=="usb") {
            if(argc==5&&std::string_view(argv[2])=="power") {
                const auto result=uke::usb_power_set(argv[3],argv[4]);uke::cli::print(result.output,as_json);return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="status"){uke::cli::print(uke::usb_status(),as_json);return 0;}
            if(argc==4&&std::string_view(argv[2])=="role") {
                auto result=uke::usb_request_data(argv[3]);uke::cli::print(result.output,as_json);return result.status;
            }
            if((argc==3||argc==4)&&std::string_view(argv[2])=="watch") {
                unsigned seconds=30;
                if(argc==4){std::string_view arg=argv[3];auto p=std::from_chars(arg.data(),arg.data()+arg.size(),seconds);if(p.ec!=std::errc{}||p.ptr!=arg.data()+arg.size())throw std::runtime_error("Invalid USB watch duration");}
                uke::usb_watch(seconds,as_json);return 0;
            }
        }
        if(command=="report") {
            if(argc==2){uke::cli::print(uke::report(),as_json);return 0;}
            if(argc==5&&std::string_view(argv[2])=="github"){
                if(as_json)throw uke::cli::UsageError("--json supports local report collection; GitHub submission prints its issue URL");
                uke::report_github(argv[3],argv[4]);return 0;
            }
        }
        throw uke::cli::UsageError("unknown command or arguments; see 'uke-control "+command+" --help'");
    } catch(const uke::cli::UsageError& e){std::cerr<<"uke-control: "<<e.what()<<'\n';return 64;}
    catch(const std::exception& e){std::cerr<<"uke-control: "<<uke::cli::human(e.what());return 1;}
}
int main(int argc,char** argv) {
    try {
        auto args=uke::cli::arguments(argc,argv);
        if(args.version){
            if(args.json)uke::cli::print(std::string("{\"program\":\"uke-control\",\"version\":\"")+uke::cli::version+"\",\"license\":\"MIT\"}",true);
            else std::cout<<"uke-control "<<uke::cli::version<<"\nLicense: MIT\n";
            return 0;
        }
        if(args.help || args.words.size()==1){std::cout<<uke::cli::help(args.words.size()>1?args.words[1]:"");return 0;}
        const auto& command=args.words[1];
        if(command=="channel" || command=="update" || command=="sensors" || command=="test" || command=="singleboot" || command=="multiboot") {
            for(size_t i=2;i<args.words.size();++i)
                if(args.words[i]=="--json" || args.words[i]=="--help" || args.words[i]=="-h" || args.words[i]=="--version")
                    throw uke::cli::UsageError("unexpected operand after '--': "+args.words[i]);
        }
        if(args.json && (command=="channel" || command=="update" || command=="test" || command=="resize" || command=="singleboot" || command=="multiboot"))args.words.push_back("--json");
        std::vector<char*> words;for(auto& word:args.words)words.push_back(word.data());words.push_back(nullptr);
        return run(int(words.size()-1),words.data(),args.json);
    } catch(const uke::cli::UsageError& error){std::cerr<<"uke-control: "<<error.what()<<'\n';return 64;}
    catch(const std::exception& error){std::cerr<<"uke-control: "<<error.what()<<'\n';return 1;}
}
