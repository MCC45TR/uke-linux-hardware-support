// SPDX-License-Identifier: MIT
#include "../src/boot-health-report.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    unsigned scenarios=0;
    for(const std::string mode:{"enforced","permissive","off"}) {
        for(const bool acceptance:{false,true}) {
            for(const bool healthy:{false,true}) {
                int64_t now=0;unsigned samples=0;
                std::vector<BootHealthReport> reports;
                bool result=false,threw=false;
                try {
                    result=observe_boot_health("7.3.3-7-unstable",acceptance,
                        [&]{return mode;},[&] {
                            ++samples;
                            if(!healthy)throw std::runtime_error("Boot health unit is not active: display-manager.service");
                        },[&]{return now;},[&]{now+=1000;},
                        [&](const BootHealthReport &report){reports.push_back(report);});
                } catch(const TrialHealthFailure &) {threw=true;}
                assert(threw==(!healthy && acceptance && mode=="enforced"));
                assert(result==healthy && reports.size()==2);
                assert(reports.front().verdict=="observing");
                assert(reports.back().verdict==(healthy?"healthy-unapproved":"rejected"));
                assert(reports.back().mode==mode && reports.back().observation_only==!acceptance);
                assert(now==(healthy?30000:90000) && samples==(healthy?31u:90u));
                if(!healthy)assert(reports.back().detail.find("display-manager.service")!=std::string::npos);
                const auto line=boot_health_line(reports.back());
                assert(line.starts_with("<4>UKE_KERNEL_HEALTH schema=1 "));
                assert(line.find("approval=pending")!=std::string::npos);
                ++scenarios;
            }
        }
    }
    for(const auto &[initial,final]:std::vector<std::pair<std::string,std::string>>{
            {"enforced","off"},{"permissive","enforced"},{"off","enforced"}}) {
        for(const bool acceptance:{false,true}) {
            int64_t now=0;bool threw=false;
            std::vector<BootHealthReport> reports;
            try {
                assert(!observe_boot_health("7.3.3-7-unstable",acceptance,
                    [&]{return now==0?initial:final;},[]{throw std::runtime_error("Missing input device");},
                    [&]{return now;},[&]{now+=1000;},[&](auto report){reports.push_back(report);}));
            } catch(const TrialHealthFailure &) {threw=true;}
            assert(threw==(acceptance && final=="enforced"));
            assert(reports.size()==2 && reports.back().mode==final);
            assert(reports.back().observation_only==!acceptance);
            ++scenarios;
        }
    }
    {
        bool probed=false,emitted=false,rejected=false;
        try {
            (void)observe_boot_health("kernel",false,[]{return std::string("invalid");},
                [&]{probed=true;},[]{return int64_t(0);},[]{},[&](auto){emitted=true;});
        }catch(const std::runtime_error &){rejected=true;}
        assert(rejected && !probed && !emitted);++scenarios;
    }
    {
        const auto text=boot_health_line({"rejected","kernel\nnext","off",
            std::string("bad\ncheck\r\t\033\"\\")+std::string(1024,'x'),true});
        assert(text.find('\n')==text.size()-1 && text.find('\r')==std::string::npos);
        assert(text.find('\t')==std::string::npos && text.find('\033')==std::string::npos);
        assert(text.find("kernel?next")!=std::string::npos && text.size()<760);++scenarios;
    }
    const auto previous_scenarios=scenarios;
    for(const std::string mode:{"enforced","permissive","off"})for(bool acceptance:{false,true}) {
        bool threw=false;std::vector<BootHealthReport> reports;unsigned samples=0;
        try {
            assert(!observe_boot_health("7.3.3-9-unstable",acceptance,[&]{return mode;},
                [&]{samples++;throw CriticalHealthFailure("critical: ADSP crashed");},
                []{return int64_t(0);},[]{assert(false);},[&](auto r){reports.push_back(r);}));
        }catch(const CriticalHealthFailure &){threw=true;}
        assert(threw==(acceptance && mode!="off"));
        assert(samples==1 && reports.size()==2 && reports.back().critical && reports.back().verdict=="rejected");
        assert(boot_health_line(reports.back()).find("severity=critical")!=std::string::npos);scenarios++;
    }
    std::cout<<"Actual boot-health reporting: "<<previous_scenarios
             <<" policy/window/mode-change/log-integrity scenarios passed; no boot-state or reboot provider.\n";
    std::cout<<"Actual critical boot-health reporting: "<<(scenarios-previous_scenarios)
             <<" immediate mode/authority scenarios passed; no writer or reboot provider.\n";
}
