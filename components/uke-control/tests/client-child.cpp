// SPDX-License-Identifier: MIT
// Stand-in executable selected at compile time by the client tests only.
#include <json/json.h>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
int main(int argc,char** argv) {
    const auto mode=std::string(getenv("UKE_CLIENT_STUB_MODE")?getenv("UKE_CLIENT_STUB_MODE"):"echo");
    if(mode=="hang"||mode=="descendant") {
        std::signal(SIGTERM,SIG_IGN);
        if(mode=="descendant"&&fork()!=0)return 0;
        std::cout<<"ready\n"<<std::flush;
        for(;;)pause();
    }
    if(mode=="overflow") {
        const std::string bytes(8192,'x');
        for(unsigned i=0;i<512;++i)std::cout<<bytes;
        return 0;
    }
    Json::Value result;result["argv"]=Json::Value(Json::arrayValue);
    for(int i=1;i<argc;++i)result["argv"].append(argv[i]);
    result["cycles"]=0;result["current_ma"]=-42;result["unknown"]=Json::nullValue;
    result["unicode"]="Şarj ölçümü / 电池";
    Json::StreamWriterBuilder writer;writer["indentation"]="";
    std::cout<<Json::writeString(writer,result)<<'\n';std::cerr<<"separate error stream\n";
    return 7;
}
