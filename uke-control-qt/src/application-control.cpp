// SPDX-License-Identifier: MIT
#include "application-source.hpp"
#include <unistd.h>
int main(int argc,char **argv){alarm(30);QCoreApplication app(argc,argv);app.setApplicationName("uke-control-qt");const auto a=app.arguments();if(a.size()!=5)return 64;const auto r=uq::applyApplicationPreset(a[1],a[2],a[3],a[4]);QFile out;out.open(STDOUT_FILENO,QIODevice::WriteOnly);out.write(QJsonDocument(r).toJson(QJsonDocument::Compact)+'\n');return r.value("error").toString().isEmpty()?0:69;}
