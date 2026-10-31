// SPDX-License-Identifier: MIT
#pragma once
#include "client.hpp"
#include <stdexcept>

namespace uke::tui_menu {
struct Menu {std::string id,parent;client::Text title,description;std::string action={},topic={};};
inline const std::vector<Menu>& menus() {
    static const std::vector<Menu> values{
        {"home","",{"Categories","Kategoriler"},{"Choose what you want to manage.","Yönetmek istediğiniz alanı seçin."}},
        {"general","home",{"Device overview","Cihaza genel bakış"},{"Status, workload and hardware information","Durum, iş yükü ve donanım bilgileri"}},
        {"power","home",{"Power & battery","Güç ve batarya"},{"Battery health, charging and sleep","Batarya sağlığı, şarj ve uyku"}},
        {"screen","home",{"Display & desktop","Ekran ve masaüstü"},{"Brightness, rotation, displays and lights","Parlaklık, döndürme, ekranlar ve ışıklar"}},
        {"connections","home",{"Connections","Bağlantılar"},{"USB, Ethernet, NFC and wireless diagnostic reports","USB, Ethernet, NFC ve kablosuz tanılama raporları"}},
        {"input","home",{"Sensors & touch","Sensörler ve dokunmatik"},{"Sensor readings and input diagnostics","Sensör okumaları ve giriş tanılaması"}},
        {"media","home",{"Audio & camera","Ses ve kamera"},{"Available audio and camera observations","Mevcut ses ve kamera gözlemleri"}},
        {"disks","home",{"Storage & recovery","Depolama ve kurtarma"},{"Disks, swap and guarded maintenance","Diskler, takas ve korumalı bakım"}},
        {"updates","home",{"Updates","Güncellemeler"},{"Repository channel and package updates","Depo kanalı ve paket güncellemeleri"}},
        {"support","home",{"Diagnostics & support","Tanılama ve destek"},{"Reports, snapshots and troubleshooting","Raporlar, anlık görüntüler ve sorun giderme"}},
        {"advanced","home",{"Developer tools","Geliştirici araçları"},{"API information and optional hardware tests","API bilgileri ve isteğe bağlı donanım testleri"}},
        {"battery","power",{"Battery","Batarya"},{"Charge level, health and cell information","Doluluk, sağlık ve hücre bilgileri"}},
        {"charging","power",{"Charging","Şarj"},{"Charging status, policy and end threshold","Şarj durumu, politika ve bitiş eşiği"}},
        {"charge-expert","power",{"Charging: advanced","Şarj: gelişmiş"},{"Admission, protection and qualified helper operations","Kabul, koruma ve nitelendirilmiş yardımcı işlemleri"}},
        {"sleep","power",{"Sleep & wake","Uyku ve uyandırma"},{"Inspect or request wake settings","Uyandırma ayarlarını inceleyin veya isteyin"}},
        {"display","screen",{"Display connections","Ekran bağlantıları"},{"DisplayPort, lanes, compression and refresh policy","DisplayPort, hatlar, sıkıştırma ve yenileme politikası"}},
        {"desktop","screen",{"Desktop & brightness","Masaüstü ve parlaklık"},{"Session, rotation and screen brightness","Oturum, döndürme ve ekran parlaklığı"}},
        {"lights","screen",{"Flashlight & LED","Fener ve LED"},{"Flashlight and notification light","Fener ve bildirim ışığı"}},
        {"usb","connections",{"USB","USB"},{"Role, power, CDC and transition recovery","Rol, güç, CDC ve geçiş kurtarma"}},
        {"network","connections",{"Ethernet","Ethernet"},{"Wired network status and profile","Kablolu ağ durumu ve profili"}},
        {"nfc","connections",{"NFC","NFC"},{"NFC observations and state request","NFC gözlemleri ve durum isteği"}},
        {"wifi","connections",{"Wi-Fi diagnostic report","Wi-Fi tanılama raporu"},{"Collect existing state; no scanning or connection changes","Mevcut durumu topla; tarama veya bağlantı değişikliği yok"},"diagnostics.collect","wifi"},
        {"bluetooth","connections",{"Bluetooth diagnostic report","Bluetooth tanılama raporu"},{"Collect existing state; no discovery or pairing","Mevcut durumu topla; keşif veya eşleştirme yok"},"diagnostics.collect","bluetooth"},
        {"storage","disks",{"Disks & swap","Diskler ve takas"},{"Inventory, roots and swap settings","Envanter, kökler ve takas ayarları"}},
        {"recovery","disks",{"Recovery & maintenance","Kurtarma ve bakım"},{"Read checks first; maintenance requires explicit approval","Önce salt okunur denetimler; bakım açık onay gerektirir"}}
    };
    return values;
}
inline const Menu& menu(std::string_view id) {
    for(const auto& value:menus())if(value.id==id)return value;
    throw std::invalid_argument("Unknown menu");
}
inline std::string owner(const client::Action& a) {
    if(a.id=="capabilities")return "advanced";
    if(a.id=="overview.nfc"||a.id.starts_with("nfc."))return "nfc";
    if(a.id.starts_with("usb."))return "usb";
    if(a.id.starts_with("resize.")||a.id.starts_with("singleboot.")||a.id.starts_with("multiboot."))return "recovery";
    if(a.category=="charge") {
        for(const auto id:{"overview.charge","charge.status","charge.settings","charge.modes","charge.plan","charge.temporary-preview","charge.mode","charge.limit"})if(a.id==id)return "charging";
        return "charge-expert";
    }
    if(a.category=="overview")return "general";
    if(a.category=="wake")return "sleep";
    if(a.category=="sensors")return "input";
    if(a.category=="diagnostics")return "support";
    if(a.category=="developer")return "advanced";
    (void)menu(a.category);
    return a.category;
}
inline std::string legacy(std::string_view id) {
    if(id=="overview")return "general";
    if(id=="charge")return "charging";
    if(id=="wake")return "sleep";
    if(id=="sensors")return "input";
    if(id=="diagnostics")return "support";
    if(id=="developer")return "advanced";
    return std::string(id);
}
struct Item {std::string id;bool action=false;};
inline std::vector<Item> items(std::string_view id) {
    std::vector<Item> result;
    for(const auto& value:menus())if(value.parent==id)result.push_back({value.id,false});
    // Put observations before requests while preserving catalog order within each group.
    for(const auto read:{true,false})for(const auto& a:client::actions())
        if(owner(a)==id&&(a.effect==client::Effect::read)==read)result.push_back({a.id,true});
    return result;
}
}
