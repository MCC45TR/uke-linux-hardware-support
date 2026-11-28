// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <cerrno>
namespace uq {
namespace {
QString text(const QJsonValue &v) {
    if(v.isNull()||v.isUndefined())return i18n("Not exported by this driver");
    if(v.isBool())return v.toBool()?i18n("Yes"):i18n("No");
    const auto result=safe(v.toVariant().toString().left(4096).toStdString());
    return result.isEmpty()?i18n("Not exported by this driver"):result;
}
QVariantMap fact(const QString &title,const QJsonValue &v,bool primary=false,bool detail=false) {
    return {{"title",title},{"value",text(v)},{"progress",-1},{"primary",primary},{"detail",detail}};
}
QVariantMap section(const QString &key,const QString &title,const QVariantList &rows,bool compact=false,const QString &note={}) {
    return {{"key",key},{"title",title},{"rows",rows},{"compact",compact},{"note",note}};
}
QString percent(const QJsonValue &value) {
    return value.isDouble() && value.toDouble()>=0 && value.toDouble()<=100
        ? i18nc("percentage", "%1%", QLocale().toString(value.toDouble(),'g',4)) : i18n("Not reported");
}
QString role(const QJsonValue &v) {
    const auto s=v.toString();const auto m=QRegularExpression("\\[([^]]+)\\]").match(s);return m.hasMatch()?m.captured(1):text(v);
}
QString speed(const QJsonValue &v) {bool ok=false;const auto n=v.toString().toDouble(&ok);return ok&&n>0?(n>=1000?QLocale().toString(n/1000,'g',4)+" Gbit/s":QLocale().toString(n,'g',4)+" Mbit/s"):i18n("Not reported");}
}
QVariantList kernelInformationSections(const QString &id,const QJsonObject &d) {
    QVariantList out;
    if(id=="kernel.charging") {
        const auto s=d.value("settings").toObject(),a=d.value("admission").toObject();
        const bool valid=s.value("firmware_get_complete").toBool()&&s.value("exit_status").toInt(-1)==0;
        QVariantList rows;
        rows<<fact(i18n("Firmware settings read"),valid,false,true);
        if(valid) {
            const auto end=s.value("reported_end_percent");
            rows<<fact(i18n("Charge limit enabled"),s.value("reported_enabled"));
            rows<<fact(i18n("Start charging below"),percent(s.value("reported_start_percent")));
            rows<<fact(i18n("Saved stop threshold"),percent(end),end.isDouble()&&end.toDouble()>=0&&end.toDouble()<=100);
        } else rows<<fact(i18n("Settings read unavailable"),i18n("The charging driver did not provide a verified settings snapshot. Refresh to retry."));
        out<<section("firmware-charging",i18n("Saved charge limit"),rows,false,
                     valid?i18n("These settings come from the firmware. A charging measurement is needed to verify when charging stops."):QString());
        const auto reason=a.value("reason").toString();
        const QMap<QString,QString> reasons{{"control_profile_unqualified",i18n("This kernel profile has not been qualified for charge control")},{"control_root_required",i18n("Administrator access is required to inspect control admission")}};
        out<<section("charging-availability",i18n("Changing charging settings"),
                     {fact(i18n("Charge limit can be changed"),a.value("threshold_admission_ready")),
                      fact(i18n("Automatic control available"),a.value("automatic_admission_ready")),
                      fact(i18n("Provider explanation"),reasons.value(reason,reason.isEmpty()?i18n("No admission result was returned"):text(reason))),
                      fact(i18n("Physical cutoff verified"),i18n("Not reported"),false,true)});
        return out;
    }
    if(id=="kernel.tag") {
        const bool present=d.value("present").toBool();
        QVariantList rows{fact(i18n("Built-in NFC tag detected"),d.value("present")),fact(i18n("Device function"),i18n("FM19511 Type 2 tag; external tag/card reading requires a separate reader"))};
        if(present) {
            rows<<fact(i18n("Bound driver"),d.value("driver"));
            rows<<fact(i18n("Wake permission"),d.value("wakeup"));
            rows<<fact(i18n("Runtime power state"),d.value("runtime_status"));
            const int error=d.value("info_errno").toInt(-1);
            rows<<fact(i18n("Driver capability query"),error==0?i18n("Read successfully"):error==ENOTTY?i18n("This kernel does not expose the capability query"):error==EBUSY?i18n("The tag is in use by another application"):i18n("The driver capability query could not be read"));
            if(error==0&&d.value("abi").toInt()==1) {
                rows<<fact(i18n("Read-only memory interface"),d.value("memory_read"));
                rows<<fact(i18n("RF event interface"),d.value("rf_events"));
            }
        }
        rows<<fact(i18n("Tag contents"),i18n("No EEPROM contents are read or written by this information page."));
        out<<section("built-in-nfc",i18n("Built-in NFC tag"),rows);return out;
    }
    if(id!="kernel.dock")return out;
    for(const auto &v:d.value("ports").toArray()) {
        const auto p=v.toObject();
        const auto partner=p.value("partner");
        const auto connection=partner.isBool()?(partner.toBool()?i18n("Connected"):i18n("Disconnected")):i18n("Not reported");
        out<<section("port-"+p.value("name").toString(),i18n("USB-C connection · %1",text(p.value("name"))),
                     {fact(i18n("Connection"),connection,partner.isBool()),
                      fact(i18n("Current data role"),role(p.value("data_role"))),fact(i18n("Current power role"),role(p.value("power_role"))),
                      fact(i18n("Reported power operation mode"),p.value("power_operation_mode")),fact(i18n("Port PD capability revision"),p.value("usb_power_delivery_revision")),
                      fact(i18n("Connection handling"),d.value("kernel_managed").toBool()?i18n("Automatic — managed by the kernel Type-C controller"):i18n("Automatic role ownership could not be verified")),
                      fact(i18n("Role interpretation"),i18n("Host carries USB data. Sink receives power. These roles are independent."))});
    }
    const auto devices=d.value("devices").toArray();
    out<<section("accessories",i18n("Connected accessories"),{fact(i18n("USB devices detected"),d.value("devices").isArray()?QJsonValue(devices.size()):QJsonValue(),d.value("devices").isArray())},
                 false,i18n("The link speed shows what each accessory negotiated. A file transfer can run at a different speed."));
    for(const auto &v:devices) {
        const auto p=v.toObject();const auto name=text(p.value("name"));
        const auto label=p.value("product").isString()?text(p.value("product")):i18n("USB accessory");
        const auto link=speed(p.value("speed"));
        out<<section("usb-"+p.value("name").toString(),label+" · "+name,{fact(i18n("Manufacturer"),p.value("manufacturer")),fact(i18n("Current USB link speed"),link,link!=i18n("Not reported")),fact(i18n("USB specification reported by device"),p.value("version")),fact(i18n("Driver"),p.value("driver")),fact(i18n("Power saving policy"),p.value("power/control")),fact(i18n("Runtime power state"),p.value("power/runtime_status")),fact(i18n("Advertised maximum current"),p.value("bMaxPower")),fact(i18n("Vendor / product ID"),text(p.value("idVendor"))+" / "+text(p.value("idProduct")))},true);
    }
    for(const auto &v:d.value("networks").toArray()) {
        const auto p=v.toObject();QVariantList rows{fact(i18n("Connection state"),p.value("operstate")),fact(i18n("Negotiated Ethernet speed"),speed(p.value("speed"))),fact(i18n("Carrier detected"),p.value("carrier")),fact(i18n("Duplex"),p.value("duplex")),fact(i18n("Driver"),p.value("driver"))};
        for(const auto &key:{"rx_bytes","tx_bytes","rx_errors","tx_errors","rx_dropped","tx_dropped"}) {
            const QMap<QString,QString> labels{{"rx_bytes",i18n("Received")},{"tx_bytes",i18n("Sent")},{"rx_errors",i18n("Receive errors")},{"tx_errors",i18n("Transmit errors")},{"rx_dropped",i18n("Receive drops")},{"tx_dropped",i18n("Transmit drops")}};
            const auto value=p.value("statistics/"+QString(key));bool ok=false;const auto n=value.toString().toULongLong(&ok);
            rows<<fact(labels.value(key),ok&&QString(key).endsWith("bytes")?QLocale().formattedDataSize(qint64(n)):text(value));
        }
        rows<<fact(i18n("Counter period"),i18n("Since the interface was created; counters are not a speed test."));
        out<<section("network-"+p.value("name").toString(),i18n("Dock Ethernet · %1",text(p.value("name"))),rows,true);
    }
    for(const auto &v:d.value("displays").toArray()) {
        const auto p=v.toObject();out<<section("display-"+p.value("name").toString(),i18n("External display · %1",text(p.value("name"))),{fact(i18n("Connection"),p.value("status")),fact(i18n("Output enabled"),p.value("enabled")),fact(i18n("Advertised resolutions"),p.value("modes")),fact(i18n("Display interpretation"),i18n("Supported resolutions do not identify the active mode, refresh rate or a verified image. Use KDE display settings for the active layout."))},true);
    }
    if(d.value("ports").toArray().isEmpty())out<<section("no-usb-port",i18n("USB-C connection"),{fact(i18n("Type-C ports"),i18n("No Type-C port is currently exported by the kernel"))});
    return out;
}
}
