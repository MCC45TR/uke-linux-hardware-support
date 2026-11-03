// SPDX-License-Identifier: MIT
#include "microphone-graph.hpp"
#include <cassert>
#include <iostream>
using Json::Value;
Value node(unsigned id,const char *kind) {
 Value v;v["id"]=id;v["type"]="PipeWire:Interface:Node";v["info"]["props"]["media.class"]=kind;v["info"]["state"]="running";return v;
}
Value port(unsigned id,unsigned owner,const char *channel) {
 Value v;v["id"]=id;v["type"]="PipeWire:Interface:Port";v["info"]["props"]["node.id"]=owner;v["info"]["props"]["audio.channel"]=channel;return v;
}
Value link(unsigned id,unsigned from,unsigned out,unsigned to,unsigned in) {
 Value v;v["id"]=id;v["type"]="PipeWire:Interface:Link";auto &i=v["info"];
 i["output-node-id"]=from;i["output-port-id"]=out;i["input-node-id"]=to;i["input-port-id"]=in;i["state"]="active";return v;
}
int main() {
 using uke::microphone::graph::Snapshot;
 Value a(Json::arrayValue);Value core;core["id"]=0;core["type"]="PipeWire:Interface:Core";a.append(core);
 a.append(node(10,"Audio/Source"));a.append(node(20,"Stream/Input/Audio"));
 a.append(port(11,10,"AUX2"));a.append(port(21,20,"MONO"));a.append(link(30,10,11,20,21));
 auto streams=Snapshot(a).streams(10);assert(streams.size()==1);
 assert(streams[0]["routes"][0]["physical_microphones"][0]=="uke-amic4");
 assert(streams[0]["routes"][0]["physical_route_verified"].asBool());
 assert(Snapshot(a).streams(0)[0]["routes"][0]["kind"]=="external_or_unverified_source");
 Value broken=a;broken[3]["info"]["props"]["node.id"]=999;
 assert(Snapshot(broken).streams(10)[0]["routes"].empty());
 Value duplicate=a;duplicate.append(core);bool rejected=false;try {Snapshot bad(duplicate);} catch(const std::exception &) {rejected=true;}assert(rejected);
 a.removeIndex(5,nullptr); // replace raw->application with a real processing path
 auto source=node(40,"Audio/Source/Internal");auto &p=source["info"]["props"];
 p["uke.microphone.internal"]=true;p["uke.microphone.path"]="fixture-path";p["uke.microphone.stage"]="source";
 p["uke.microphone.id"]="uke-amic4";p["uke.microphone.processing"]="webrtc";a.append(source);
 auto capture=node(50,"Stream/Input/Audio");capture["info"]["props"]=p;capture["info"]["props"]["uke.microphone.stage"]="capture";a.append(capture);
 auto reference=node(60,"Stream/Input/Audio");reference["info"]["props"]=p;reference["info"]["props"]["uke.microphone.stage"]="reference";a.append(reference);
 a.append(node(70,"Audio/Sink"));a.append(port(41,40,"MONO"));a.append(port(51,50,"AUX2"));
 a.append(port(61,60,"FL"));a.append(port(71,70,"FL"));a.append(link(80,10,11,50,51));a.append(link(81,40,41,20,21));a.append(link(82,70,71,60,61));
 auto route=Snapshot(a).streams(10)[0]["routes"][0];
 assert(route["kind"]=="speech" && route["physical_route_verified"].asBool());
 assert(route["aec_reference_linked"].asBool() && route["aec_reference_running"].asBool());
 assert(route["physical_microphones"][0]=="uke-amic4" && route["acoustic_acceptance"]=="not_measured");
 auto extra=capture;extra["id"]=90;a.append(extra);
 assert(!Snapshot(a).streams(10)[0]["routes"][0]["physical_route_verified"].asBool());
 a.removeIndex(a.size()-1,nullptr);
 a.append(port(91,70,"FR"));a.append(port(92,50,"FR"));a.append(link(93,70,91,50,92));
 assert(!Snapshot(a).streams(10)[0]["routes"][0]["physical_route_verified"].asBool());
 Value empty(Json::arrayValue);assert(Snapshot(empty).streams(10).empty());
 std::cout<<"Actual graph reader: direct/transitive channels, reference, ambiguous paths, port ownership, external and duplicate identities passed\n";
}
