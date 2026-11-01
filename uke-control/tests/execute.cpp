// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include <cassert>
#include <chrono>
#include <ctime>
#include <iostream>

int main() {
    const auto started=std::chrono::steady_clock::now();
    const auto cpu=std::clock();
    const auto closed=uke::execute({"/usr/bin/bash","-c","exec 1>&- 2>&-; /usr/bin/sleep 1; exit 7"},4,4096);
    const auto elapsed=std::chrono::steady_clock::now()-started;
    const auto cpu_ms=1000.0*(std::clock()-cpu)/CLOCKS_PER_SEC;
    std::cout<<"Closed output: wall_ms="<<std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
             <<" parent_cpu_ms="<<cpu_ms<<std::endl;
    assert(closed.status==7&&closed.output.empty());
    assert(elapsed>=std::chrono::milliseconds(900)&&cpu_ms<250);

    const auto timeout=uke::execute_bounded({"/usr/bin/bash","-c","exec 1>&- 2>&-; /usr/bin/sleep 5"},1,4096);
    assert(timeout.status==124&&timeout.output.find("time/output limit")!=std::string::npos);
    assert(timeout.timed_out&&!timeout.truncated);
    const auto output=uke::execute({"/usr/bin/printf","%s","literal;$HOME"},2,4096);
    assert(output.status==0&&output.output=="literal;$HOME");
    const auto limited=uke::execute_bounded({"/usr/bin/yes"},2,4096);
    assert(limited.status==124&&limited.output.starts_with("y\ny\n")&&limited.output.size()<4200);
    assert(limited.truncated&&!limited.timed_out);
    for(unsigned repeat=0;repeat<30;++repeat)for(unsigned bytes:{4095U,4096U,4097U,8192U}) {
        const auto finite=uke::execute_bounded({"/usr/bin/head","-c",std::to_string(bytes),"/dev/zero"},2,4096);
        assert(!finite.timed_out);
        assert(finite.truncated==(bytes>4096));
        assert(finite.status==(bytes>4096?124:0));
        if(bytes<=4096)assert(finite.output==std::string(bytes,'\0'));
    }
    const auto empty=uke::execute_bounded({"/usr/bin/true"},2,0);
    assert(empty.status==0&&empty.output.empty()&&!empty.truncated);
    const auto signaled=uke::execute_bounded({"/usr/bin/bash","-c","kill -TERM $$"},2,4096);
    assert(signaled.status==143&&signaled.signal==15);
    std::cout<<"Actual command runner: early EOF, exit status, timeout, literal arguments and output limit passed\n";
}
