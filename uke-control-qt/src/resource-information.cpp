// SPDX-License-Identifier: MIT
#include "resource-source.hpp"
namespace {
QString status(const QString &s){const QMap<QString,QString> names{{"available","OK"},{"not-exported","not provided by this kernel"},{"kernel-support-missing","kernel support missing"},{"permission-denied","permission denied"},{"no-data","driver has no data"},{"read-failed","reading failed"},{"module","module support; runtime provider still required"}};return names.value(s,s);}
QString plain(QString s){for(auto &c:s)if(c.unicode()<32&&c!='\n'&&c!='\t')c='?';return s;}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);const auto a=app.arguments();
    const QStringList domains{"manager-overview","cpu","gpu","memory","storage","thermal","profiles","diagnostics"};
    if(a.size()>2||(a.size()==2&&!domains.contains(a[1]))) {QTextStream(stderr)<<"Usage: uke-kernel-status [cpu|gpu|memory|storage|thermal|profiles|diagnostics]\n";return 64;}
    const auto d=uq::resourceSnapshot(a.size()==2?a[1]:"manager-overview");QTextStream out(stdout);
    out<<"Senemos Kernel Manager\nKernel: "<<QSysInfo::kernelVersion()<<"\n";
    for(const auto &v:d.value("sections").toArray()) {const auto s=v.toObject();out<<"\n"<<s.value("title").toString()<<"\n";for(const auto &r:s.value("rows").toArray()) {const auto row=r.toObject();const auto value=plain(row.value("value").toString());out<<"  "<<row.value("title").toString()<<": "<<(value.isEmpty()?"Not reported":value)<<" ["<<status(row.value("status").toString())<<"]\n";}}
    if(!d.value("controls").toArray().isEmpty())out<<"\nRuntime settings (not saved for the next boot)\n";
    for(const auto &v:d.value("controls").toArray()){const auto s=v.toObject();out<<"  "<<s.value("title").toString()<<": "<<plain(s.value("value").toString())<<" "<<s.value("unit").toString()<<" ["<<status(s.value("status").toString())<<"]\n";}
    return 0;
}
