// SPDX-License-Identifier: MIT
#include "resource-source.hpp"
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);const auto a=app.arguments();
    const QStringList domains{"manager-overview","cpu","gpu","memory","storage","thermal","profiles","diagnostics"};
    if(a.size()>2||(a.size()==2&&!domains.contains(a[1]))) {QTextStream(stderr)<<"Usage: uke-kernel-status [cpu|gpu|memory|storage|thermal|profiles|diagnostics]\n";return 64;}
    const auto d=uq::resourceSnapshot(a.size()==2?a[1]:"manager-overview");QTextStream out(stdout);
    out<<"Senemos Kernel Manager\nKernel: "<<QSysInfo::kernelVersion()<<"\n";
    for(const auto &v:d.value("sections").toArray()) {const auto s=v.toObject();out<<"\n"<<s.value("title").toString()<<"\n";for(const auto &r:s.value("rows").toArray()) {const auto row=r.toObject();QString value=row.value("value").toString();for(auto &c:value)if(c.unicode()<32&&c!='\n'&&c!='\t')c='?';out<<"  "<<row.value("title").toString()<<": "<<(value.isEmpty()?"Not reported":value)<<" ["<<row.value("status").toString()<<"]\n";}}
    return 0;
}
