// SPDX-License-Identifier: MIT
#pragma once
#include "oss-brightness-curve.hpp"
#include <json/json.h>
#include <memory>
#include <regex>
#include <sstream>
#include <string>

namespace uke::brightness {
inline constexpr const char* profile_name="uke-oss-normal-v1";
inline Json::Value parse(const std::string& text) {
    require(!text.empty()&&text.size()<=1024*1024,"Brightness configuration exceeds bounds");
    Json::CharReaderBuilder builder;
    builder["rejectDupKeys"]=true;
    builder["allowComments"]=false;
    builder["allowTrailingCommas"]=false;
    builder["allowSpecialFloats"]=false;
    builder["failIfExtra"]=true;
    builder["stackLimit"]=64;
    Json::Value result;std::string errors;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    require(reader->parse(text.data(),text.data()+text.size(),&result,&errors),"Invalid brightness JSON configuration");
    return result;
}
inline std::string serialize(const Json::Value& value) {
    Json::StreamWriterBuilder writer;writer["indentation"]="    ";
    writer["precision"]=17;
    const auto result=Json::writeString(writer,value)+"\n";
    require(result.size()<=1024*1024,"Brightness configuration exceeds bounds");
    return result;
}
inline Json::Value& panel(Json::Value& document) {
    require(document.isArray()&&document.size()<=32,"Invalid KWin output configuration");
    Json::Value* outputs=nullptr;
    for(auto& record:document) {
        require(record.isObject(),"Invalid KWin output record");
        if(record.get("name","")!="outputs")continue;
        require(!outputs&&record["data"].isArray()&&record["data"].size()<=64,"Ambiguous KWin output records");
        outputs=&record["data"];
    }
    require(outputs,"Missing KWin output records");
    Json::Value* selected=nullptr;
    for(auto& output:*outputs) {
        require(output.isObject(),"Invalid KWin output entry");
        if(!output["connectorName"].isString())continue;
        const auto name=output["connectorName"].asString();
        if(!std::regex_match(name,std::regex("DSI-[0-9]{1,3}")))continue;
        require(!selected,"Ambiguous internal panel configuration");
        selected=&output;
    }
    require(selected,"Missing internal panel configuration");
    return *selected;
}
inline Json::Value curve_json(const Points& points) {
    validate(points);
    Json::Value result(Json::arrayValue);
    for(const auto& [x,y]:points) {
        require(y<=1,"Brightness exceeds native normal range");
        Json::Value point(Json::arrayValue);point.append(x);point.append(y);result.append(point);
    }
    return result;
}
inline void validate_curve(const Json::Value& curve) {
    require(curve.isArray()&&curve.size()>=1&&curve.size()<=4096,"Invalid saved brightness curve");
    double previous=-1;
    for(const auto& point:curve) {
        require(point.isArray()&&point.size()==2&&point[0].isNumeric()&&point[1].isNumeric(),"Invalid saved curve point");
        const double x=point[0].asDouble(),y=point[1].asDouble();
        require(std::isfinite(x)&&std::isfinite(y)&&x>=0&&x<=100000&&x>previous&&y>=0&&y<=1,"Invalid saved curve range");
        previous=x;
    }
}
inline Json::Value make_request(const std::string& current,bool restore,
                                const Json::Value& applied=Json::Value{}) {
    auto document=parse(current);auto& output=panel(document);
    Json::Value request(Json::objectValue);
    request["schema"]="org.uke.control.brightness-curve-request.v1";
    request["profile"]=profile_name;
    request["operation"]=restore?"restore":"oss-normal";
    request["original_config"]=current;
    request["connector"]=output["connectorName"];
    request["had_curve"]=output.isMember("autoBrightnessCurve");
    if(request["had_curve"].asBool()) {
        validate_curve(output["autoBrightnessCurve"]);
        request["old_curve"]=output["autoBrightnessCurve"];
    }
    if(restore) {
        require(applied.isObject()&&applied["schema"]=="org.uke.control.brightness-curve-applied.v1"&&
            applied["profile"]==profile_name&&applied["connector"]==output["connectorName"]&&
            applied["had_curve"].isBool(),"No matching saved brightness curve");
        require(output.isMember("autoBrightnessCurve")&&output["autoBrightnessCurve"]==applied["new_curve"],
            "The curve changed after OSS import; preserve current user learning");
        request["restore_had_curve"]=applied["had_curve"];
        if(applied["had_curve"].asBool()) {
            validate_curve(applied["old_curve"]);request["restore_curve"]=applied["old_curve"];
        }
    }
    return request;
}
struct Transition {std::string configuration;Json::Value applied;};
inline Transition apply_request(const std::string& current,const Json::Value& request) {
    require(request.isObject()&&request["schema"]=="org.uke.control.brightness-curve-request.v1"&&
        request["profile"]==profile_name&&request["original_config"].isString()&&
        request["original_config"].asString()==current&&request["had_curve"].isBool(),
        "Brightness request is stale or malformed; configuration preserved");
    auto document=parse(current);auto& output=panel(document);
    require(request["connector"]==output["connectorName"]&&
        request["had_curve"].asBool()==output.isMember("autoBrightnessCurve"),"Internal output changed");
    if(output.isMember("autoBrightnessCurve"))
        require(output["autoBrightnessCurve"]==request["old_curve"],"Original brightness curve changed");
    const bool restore=request["operation"]=="restore";
    require(restore||request["operation"]=="oss-normal","Unknown brightness operation");
    Json::Value applied;
    if(restore) {
        require(request["restore_had_curve"].isBool(),"Invalid restore request");
        if(request["restore_had_curve"].asBool()) {
            validate_curve(request["restore_curve"]);output["autoBrightnessCurve"]=request["restore_curve"];
        }else output.removeMember("autoBrightnessCurve");
    }else {
        applied["schema"]="org.uke.control.brightness-curve-applied.v1";
        applied["profile"]=profile_name;applied["connector"]=output["connectorName"];
        applied["had_curve"]=request["had_curve"];
        if(request["had_curve"].asBool())applied["old_curve"]=request["old_curve"];
        applied["new_curve"]=curve_json(oss_normal_curve());
        output["autoBrightnessCurve"]=applied["new_curve"];
    }
    return {serialize(document),applied};
}
}
