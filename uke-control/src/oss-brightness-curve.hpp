// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: The LineageOS Project
// OEM numerical data: Xiaomi sm8635-common Apache-2.0 overlay at
// 8bd8e52a0245178256b70d5f8f093a0620ca8ce7, config.xml blob
// 7d07f3da845e46211b1201aa1fa993aa2020e873.
// Monotone interpolation follows Android Spline.java (Android 16),
// Copyright 2013 The Android Open Source Project.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace uke::brightness {
using Point = std::pair<double,double>;
using Points = std::vector<Point>;
inline constexpr std::array<Point,108> oss_lux_nits{{
    {0, 3.5},
    {1, 4.5},
    {2, 6},
    {4, 15},
    {6, 21},
    {8, 27},
    {10, 36},
    {15, 52},
    {20, 71},
    {25, 87},
    {30, 97},
    {35, 98},
    {40, 98},
    {45, 98},
    {50, 99},
    {55, 99},
    {60, 100},
    {65, 100},
    {70, 100},
    {75, 101},
    {80, 101},
    {85, 102},
    {90, 103},
    {95, 103},
    {100, 103},
    {120, 106},
    {140, 108},
    {160, 110},
    {180, 112},
    {200, 114},
    {220, 116},
    {240, 118},
    {260, 119},
    {280, 122},
    {300, 123},
    {320, 125},
    {340, 128},
    {360, 130},
    {380, 131},
    {400, 134},
    {420, 136},
    {440, 137},
    {460, 140},
    {480, 142},
    {500, 144},
    {700, 169},
    {900, 196},
    {1100, 222},
    {1300, 248},
    {1500, 270},
    {1700, 300},
    {1900, 320},
    {2000, 335},
    {2500, 400},
    {3000, 412.5},
    {3500, 425},
    {4000, 437.5},
    {4500, 450},
    {5000, 462.5},
    {5500, 475},
    {6000, 487.5},
    {6500, 500},
    {7000, 512.5},
    {7500, 525},
    {8000, 537.5},
    {8500, 550},
    {9000, 562.5},
    {9500, 575},
    {10000, 587.5},
    {10500, 600},
    {11000, 612.5},
    {11500, 625},
    {12000, 637.5},
    {12500, 650},
    {13000, 662.5},
    {13500, 675},
    {14000, 687.5},
    {14500, 700},
    {15000, 702.2},
    {16000, 706.6},
    {17000, 711},
    {18000, 715.4},
    {19000, 719.8},
    {20000, 724.2},
    {21000, 728.6},
    {22000, 733},
    {23000, 737.4},
    {24000, 741.8},
    {25000, 746.2},
    {26000, 750.5},
    {27000, 755},
    {28000, 759.3},
    {29000, 763.7},
    {30000, 768.1},
    {35000, 790.1},
    {40000, 800},
    {45000, 800},
    {50000, 800},
    {55000, 800},
    {60000, 800},
    {65000, 800},
    {70000, 800},
    {75000, 800},
    {80000, 800},
    {85000, 800},
    {90000, 800},
    {95000, 800},
    {100000, 800}
}};
inline constexpr double normal_max = 2047;
inline constexpr double normal_min = 21;
inline void require(bool ok,const char* message) {
    if(!ok)throw std::runtime_error(message);
}
inline void validate(const Points& points) {
    require(points.size()>=2&&points.size()<=4096,"Invalid brightness point count");
    for(size_t i=0;i<points.size();++i) {
        const auto [x,y]=points[i];
        require(std::isfinite(x)&&std::isfinite(y)&&x>=0&&x<=100000&&y>=0,"Invalid brightness point");
        require(!i||(x>points[i-1].first&&y>=points[i-1].second),"Non-monotone brightness curve");
    }
}
inline double linear(const Points& points,double x) {
    if(x<=points.front().first)return points.front().second;
    const auto upper=std::upper_bound(points.begin(),points.end(),x,
        [](double at,const Point& point){return at<point.first;});
    if(upper==points.end())return points.back().second;
    const auto lower=std::prev(upper);
    return std::lerp(lower->second,upper->second,(x-lower->first)/(upper->first-lower->first));
}
class Spline {
    Points points_;
    std::vector<double> slopes_;
public:
    explicit Spline(Points points):points_(std::move(points)),slopes_(points_.size()) {
        validate(points_);
        std::vector<double> secants(points_.size()-1);
        for(size_t i=0;i<secants.size();++i)
            secants[i]=(points_[i+1].second-points_[i].second)/(points_[i+1].first-points_[i].first);
        slopes_.front()=secants.front();slopes_.back()=secants.back();
        for(size_t i=1;i+1<slopes_.size();++i)slopes_[i]=(secants[i-1]+secants[i])/2;
        for(size_t i=0;i<secants.size();++i) {
            if(secants[i]==0)slopes_[i]=slopes_[i+1]=0;
            else {
                const double length=std::hypot(slopes_[i]/secants[i],slopes_[i+1]/secants[i]);
                if(length>3){slopes_[i]*=3/length;slopes_[i+1]*=3/length;}
            }
        }
    }
    double sample(double x)const {
        require(std::isfinite(x),"Non-finite ambient light");
        if(x<=points_.front().first)return points_.front().second;
        if(x>=points_.back().first)return points_.back().second;
        const auto upper=std::upper_bound(points_.begin(),points_.end(),x,
            [](double at,const Point& point){return at<point.first;});
        const auto i=size_t(upper-points_.begin()-1);
        const double h=points_[i+1].first-points_[i].first,t=(x-points_[i].first)/h;
        const double result=(points_[i].second*(1+2*t)+h*slopes_[i]*t)*(1-t)*(1-t)
             +(points_[i+1].second*(3-2*t)+h*slopes_[i+1]*(t-1))*t*t;
        return std::clamp(result,points_[i].second,points_[i+1].second);
    }
};
inline double oss_normal(const Spline& spline,double lux) {
    // Public product-map numerical points, not private calibration or XML:
    // (2 nit,0.001709819), (600 nit,0.49975574), (800 nit,1).
    // The normal/HBM boundary is 600 nit. Preserve native normal 0..2047.
    const double nits=spline.sample(lux);
    const double android=nits<=2?0.001709819:nits>=800?1.0:nits>600?
        std::lerp(0.49975574,1.0,(nits-600)/200):
        std::lerp(0.001709819,0.49975574,(nits-2)/598);
    return std::clamp(android*4095/normal_max,normal_min/normal_max,1.0);
}
inline Points oss_normal_curve() {
    const Spline spline(Points(oss_lux_nits.begin(),oss_lux_nits.end()));
    const auto at=[&](double lux){return oss_normal(spline,lux);};
    Points result{{0,at(0)}};
    const auto segment=[&](auto&& self,double x,double y,unsigned depth)->void {
        require(depth<=20&&result.size()<4096,"OSS brightness approximation exceeds bounds");
        bool split=false;
        for(const double fraction:{0.25,0.5,0.75}) {
            const double lux=std::lerp(x,y,fraction);
            split|=std::abs(at(lux)-std::lerp(at(x),at(y),fraction))>0.1/normal_max;
        }
        if(split){const double mid=(x+y)/2;self(self,x,mid,depth+1);self(self,mid,y,depth+1);}
        else result.emplace_back(y,at(y));
    };
    for(size_t i=1;i<oss_lux_nits.size();++i)segment(segment,oss_lux_nits[i-1].first,oss_lux_nits[i].first,0);
    validate(result);
    return result;
}
}
