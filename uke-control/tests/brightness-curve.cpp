// SPDX-License-Identifier: MIT
#define UKE_BRIGHTNESS_CURVE_TEST
#include "../src/brightness-curve.cpp"
#include <cassert>
#include <limits>
#include <sys/wait.h>

namespace {
unsigned checks=0;
void check(bool ok){++checks;assert(ok);}
template<class Operation> void refuses(Operation operation) {
    bool refused=false;try{operation();}catch(const std::exception&){refused=true;}
    check(refused);
}
const std::string config=R"([
 {"name":"outputs","data":[
  {"connectorName":"DP-1","autoBrightnessCurve":[[0,0.5]],"mode":{"width":3840},"iccProfilePath":"private-profile.icc"},
  {"connectorName":"DSI-1","autoBrightnessCurve":[[0,0.07],[103,0.19],[107,0.98],[10000,1]],"automaticBrightness":false,"vrrPolicy":"Always","autoRotation":"Never","brightness":0.07,"customUnknown":{"keep":true}}]},
 {"name":"setups","data":[{"outputs":[0,1],"priority":1}]},
 {"name":"future-section","data":{"preserve":"yes"}}
])";
void numerical() {
    using namespace uke::brightness;
    const auto points=oss_normal_curve();
    check(points.size()>=108&&points.size()<=4096);
    check(points.front().first==0&&points.back().first==100000);
    check(points.front().second==21.0/2047&&points.back().second==1);
    const Spline spline(Points(oss_lux_nits.begin(),oss_lux_nits.end()));
    // Reference knot equality and direct public piecewise map, independent of
    // the adaptive approximation's segment selection.
    for(const auto& [lux,nits]:oss_lux_nits) {
        check(spline.sample(lux)==nits);
        double normalized;
        if(nits<2)normalized=.001709819;
        else if(nits<600)normalized=.001709819+(nits-2)*(.49975574-.001709819)/598;
        else normalized=.49975574+(nits-600)*(1-.49975574)/200;
        normalized=std::clamp(normalized*4095/2047,21.0/2047,1.0);
        check(std::abs(linear(points,lux)-normalized)*2047<0.25);
    }
    double prior=0,error=0;
    for(unsigned tenth=0;tenth<=1000000;++tenth) {
        const double lux=tenth/10.0,value=linear(points,lux);
        check(value>=prior-1e-15&&value>=21.0/2047&&value<=1);
        error=std::max(error,std::abs(value-oss_normal(spline,lux))*2047);prior=value;
    }
    check(error<=.25);
    check(linear(points,500)>.239&&linear(points,500)<.241);
    const Spline straight({{0,0},{10,100},{20,200}});
    check(straight.sample(5)==50&&straight.sample(-1)==0&&straight.sample(21)==200);
    const Spline plateau({{0,1},{1,3},{2,3},{10,20}});
    check(plateau.sample(1.5)==3);
    refuses([]{Spline({{0,0},{0,1}});});
    refuses([]{Spline({{0,1},{1,0}});});
    refuses([&]{spline.sample(std::numeric_limits<double>::quiet_NaN());});
    std::cout<<"OSS normal curve: "<<points.size()<<" points; dense 0.1-lux error "<<error<<" native steps.\n";
}
void configuration() {
    using namespace uke::brightness;
    check(internal_connector("DSI-1")&&internal_connector("DSI-012"));
    for(const auto* name:{"DSI-","DSI--1","DSI-1234","DSI-1x","DP-1"})
        check(!internal_connector(name));
    const auto request=make_request(config,false);
    const auto transition=apply_request(config,request);
    auto original=parse(config),next=parse(transition.configuration);
    check(panel(next)["automaticBrightness"]==false);
    const auto imported=panel(next)["autoBrightnessCurve"];
    check(imported.size()>=108);
    panel(next)["autoBrightnessCurve"]=panel(original)["autoBrightnessCurve"];
    check(next==original); // All output/session/unknown data is retained.
    auto restore=make_request(transition.configuration,true,transition.applied);
    check(parse(apply_request(transition.configuration,restore).configuration)==original);
    auto without=parse(config);panel(without).removeMember("autoBrightnessCurve");
    const auto no_curve=serialize(without);
    const auto insert=apply_request(no_curve,make_request(no_curve,false));
    const auto undo=make_request(insert.configuration,true,insert.applied);
    check(parse(apply_request(insert.configuration,undo).configuration)==without);
    auto learned=parse(transition.configuration);panel(learned)["autoBrightnessCurve"][0][1]=.2;
    refuses([&]{make_request(serialize(learned),true,transition.applied);});
    refuses([&]{apply_request(config+" ",request);});
    for(const char* field:{"schema","profile","operation","original_config","connector","had_curve"}) {
        auto changed=request;changed.removeMember(field);
        refuses([&]{apply_request(config,changed);});
    }
    auto altered=request;altered["old_curve"][0][1]=.3;
    refuses([&]{apply_request(config,altered);});
    refuses([&]{make_request(config,true);});
    for(const auto* text:{"null","{}","[1]","[{}]","[]","invalid", "[1] []",
        R"([{"name":"outputs","name":"other","data":[]}])",
        R"([{"name":"outputs","data":[]},{"name":"outputs","data":[]}])",
        R"([{"name":"outputs","data":[{"connectorName":"DP-1"}]}])",
        R"([{"name":"outputs","data":[{"connectorName":"DSI-1"},{"connectorName":"DSI-2"}]}])"})
        refuses([&]{make_request(text,false);});
    for(const auto* text:{"[[0,-1]]","[[0,1.1]]","[[0,0],[0,1]]","[[1,0],[0,1]]","[[100001,0]]","[[0,NaN]]","[[]]","[]","[0]"})
        refuses([&]{validate_curve(parse(text));});
    refuses([&]{parse(std::string(1024*1024+1,' '));});
}
void file_operations() {
    char name[]="/tmp/uke-curve-test-XXXXXX";
    const char* raw=mkdtemp(name);check(raw);
    const fs::path root=raw,file=root/"config.json";
    try {
        publish(file,config);check(*read_file(file)==config);
        publish(file,"replacement\n");check(*read_file(file)=="replacement\n");
        check(!read_file(root/"absent",true));
        refuses([&]{read_file(root/"absent");});
        fs::create_symlink(file,root/"symlink");
        refuses([&]{read_file(root/"symlink");});
        fs::create_hard_link(file,root/"hardlink");
        refuses([&]{read_file(file);});fs::remove(root/"hardlink");
        chmod(file.c_str(),0666);refuses([&]{read_file(file);});chmod(file.c_str(),0600);
        const auto settings=root/"settings";directory(settings,true);
        chmod(settings.c_str(),0777);refuses([&]{directory(settings,false);});chmod(settings.c_str(),0700);
        fs::create_directory(root/"config");publish(root/"config/kwinoutputconfig.json",config);
        setenv("XDG_CONFIG_HOME",(root/"config").c_str(),1);
        check(command("status")==0);
        refuses([&]{command("wrong-operation");});
        check(!fs::exists(root/"config/uke-control"));
        check(command("apply-pending")==0);
        check(!fs::exists(root/"config/uke-control"));
        unsetenv("XDG_CONFIG_HOME");
    }catch(...){fs::remove_all(root);throw;}
    fs::remove_all(root);
}
}
int main() {
    numerical();configuration();file_operations();
    std::cout<<"Actual OSS curve/configuration/file callbacks: "<<checks<<" checks passed; no compositor, sensor or hardware change.\n";
}
