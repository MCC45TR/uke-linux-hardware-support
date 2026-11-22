// SPDX-License-Identifier: MIT
#pragma once
#include "client.hpp"
#include <stdexcept>

namespace uke::tui_menu {
struct Menu {std::string id,parent;client::Text title,description;std::string action={},topic={};};
inline const std::vector<Menu>& menus() {
    static const std::vector<Menu> values{
        {"home","",{"Device settings","Cihaz ayarları"},{"Daily settings and device status","Günlük ayarlar ve cihaz durumu"}},
        {"general","home",{"Device status","Cihaz durumu"},{"Battery, device information and current workload","Batarya, cihaz bilgileri ve mevcut iş yükü"}},
        {"power","home",{"Power & battery","Güç ve batarya"},{"Battery health, charging preferences and wake settings","Batarya sağlığı, şarj tercihleri ve uyandırma ayarları"}},
        {"screen","home",{"Display & lights","Ekran ve ışıklar"},{"KDE automatic brightness and rotation, flashlight and LED","KDE otomatik parlaklık ve döndürme, fener ve LED"}},
        {"connections","home",{"Connections","Bağlantılar"},{"USB and Ethernet status, NFC permission","USB ve Ethernet durumu, NFC izni"}},
        {"media","home",{"Audio & sensors","Ses ve sensörler"},{"Microphone preference, audio, camera and sensor observations","Mikrofon tercihi, ses, kamera ve sensör okumaları"}},
        {"updates","home",{"Updates","Güncellemeler"},{"Current channel, update preview and installation","Mevcut kanal, güncelleme önizlemesi ve kurulum"}},
        {"advanced","home",{"Advanced","Gelişmiş"},{"Troubleshooting, expert controls and guarded maintenance","Sorun giderme, uzman kontrolleri ve korumalı bakım"}},
        {"support","advanced",{"Diagnostics & support","Tanılama ve destek"},{"Private reports, snapshots and sharing","Özel raporlar, anlık görüntüler ve paylaşım"}},
        {"disks","advanced",{"Storage & recovery","Depolama ve kurtarma"},{"Read disk state; review maintenance separately","Disk durumunu okuyun; bakımı ayrıca inceleyin"}},
        {"charge-expert","advanced",{"Charging: expert tools","Şarj: uzman araçları"},{"Provider admission, protocol inspection and helper setup","Sağlayıcı kabulü, protokol incelemesi ve yardımcı kurulumu"}},
        {"display","advanced",{"Display: expert tools","Ekran: uzman araçları"},{"Dock lanes, compression and next-boot refresh policy","Dock hatları, sıkıştırma ve sonraki açılış yenileme politikası"}},
        {"usb","advanced",{"USB: expert tools","USB: uzman araçları"},{"Data roles, power policy and serial export","Veri rolleri, güç politikası ve seri dışa sunum"}},
        {"network","advanced",{"Ethernet profiles","Ethernet profilleri"},{"Review link advertisements before changing connectivity","Bağlantıyı değiştirmeden hat ilanlarını inceleyin"}},
        {"input","advanced",{"Sensor & touch diagnostics","Sensör ve dokunmatik tanılaması"},{"Finite observations and optional development tests","Sonlu okumalar ve isteğe bağlı geliştirme testleri"}},
        {"general-expert","advanced",{"System information","Sistem bilgileri"},{"Detailed hardware, firmware and component snapshots","Ayrıntılı donanım, firmware ve bileşen görüntüleri"}},
        {"update-expert","advanced",{"Repository & update recovery","Depo ve güncelleme kurtarma"},{"Channel switching, synchronization and transaction receipts","Kanal değiştirme, eşitleme ve işlem kayıtları"}},
        {"developer","advanced",{"Developer tools","Geliştirici araçları"},{"API metadata and optional hardware tests","API metaverisi ve isteğe bağlı donanım testleri"}},
        {"battery","general-expert",{"Battery details","Batarya ayrıntıları"},{"Reported cell topology and charging protocol","Bildirilen hücre topolojisi ve şarj protokolü"}},
        {"sleep","general-expert",{"Sleep details","Uyku ayrıntıları"},{"Reported sleep and wake observations","Bildirilen uyku ve uyanma okumaları"}},
        {"desktop","general-expert",{"Desktop providers","Masaüstü sağlayıcıları"},{"Original session and power interfaces","Özgün oturum ve güç arayüzleri"}},
        {"lights","general-expert",{"LED test","LED testi"},{"Attended notification light color test","Gözetimli bildirim ışığı renk testi"}},
        {"media-expert","general-expert",{"Media details","Medya ayrıntıları"},{"Additional media and sensor component observations","Ek medya ve sensör bileşeni okumaları"}},
        {"storage","disks",{"Disks & swap","Diskler ve takas"},{"Inventory, root inspection and swap policy","Envanter, kök incelemesi ve takas politikası"}},
        {"recovery","disks",{"Boot & partition maintenance","Açılış ve bölüm bakımı"},{"May erase data or request reboot; explicit confirmation required","Veri silebilir veya yeniden başlatma isteyebilir; açık onay gerekir"}},
        {"wifi","support",{"Wi-Fi diagnostic report","Wi-Fi tanılama raporu"},{"Collect existing state without scanning or connection changes","Tarama veya bağlantı değişikliği olmadan mevcut durumu topla"},"diagnostics.collect","wifi"},
        {"bluetooth","support",{"Bluetooth diagnostic report","Bluetooth tanılama raporu"},{"Collect existing state without discovery or pairing","Keşif veya eşleştirme olmadan mevcut durumu topla"},"diagnostics.collect","bluetooth"}
    };
    return values;
}
inline const Menu& menu(std::string_view id) {
    for(const auto& value:menus())if(value.id==id)return value;
    throw std::invalid_argument("Unknown menu");
}
inline std::string owner(const client::Action& a) {
    // Only reviewed daily tasks appear on the main pages. New catalog entries land
    // in their expert area until their end-user workflow has been reviewed.
    static const std::map<std::string,std::string> daily{
        {"overview.battery","general"},{"overview.hardware","general"},{"overview.workload","general"},
        {"battery.status","power"},{"battery.health","power"},{"charge.status","power"},
        {"charge.mode","power"},{"charge.limit","power"},{"wake.status","power"},{"wake.set","power"},
        {"desktop.plasma","screen"},{"desktop.rotation","screen"},{"desktop.brightness","screen"},
        {"flashlight.status","screen"},{"flashlight.state","screen"},{"flashlight.level","screen"},
        {"led.status","screen"},{"led.auto","screen"},{"led.color","screen"},
        {"usb.status","connections"},{"ethernet.status","connections"},{"nfc.status","connections"},{"nfc.state","connections"},
        {"microphone.status","media"},{"microphone.select","media"},{"overview.audio","media"},
        {"overview.camera","media"},{"sensors.list","media"},{"sensors.once","media"},
        {"channel.status","updates"},{"update.preview","updates"},{"update.apply","updates"}
    };
    if(const auto it=daily.find(a.id);it!=daily.end())return it->second;
    if(a.id=="capabilities")return "developer";
    if(a.id.starts_with("usb."))return "usb";
    if(a.id.starts_with("resize.")||a.id.starts_with("singleboot.")||a.id.starts_with("multiboot."))return "recovery";
    if(a.category=="charge")return "charge-expert";
    if(a.category=="overview")return "general-expert";
    if(a.category=="wake")return "sleep";
    if(a.category=="sensors")return "input";
    if(a.category=="diagnostics")return "support";
    if(a.category=="updates")return "update-expert";
    if(a.category=="media")return "media-expert";
    (void)menu(a.category);
    return a.category;
}
inline std::string legacy(std::string_view id) {
    if(id=="overview")return "general";
    if(id=="charge"||id=="charging")return "power";
    if(id=="wake")return "sleep";
    if(id=="sensors")return "media";
    if(id=="diagnostics")return "support";
    return std::string(id);
}
// Concise UI labels do not change stable API descriptors or their detailed guides.
inline client::Text title(const client::Action& a) {
    static const std::map<std::string,client::Text> labels{
        {"overview.battery",{"Battery status","Batarya durumu"}},
        {"overview.hardware",{"Device information","Cihaz bilgileri"}},
        {"overview.workload",{"Memory & workload","Bellek ve iş yükü"}},
        {"battery.status",{"Battery readings","Batarya okumaları"}},
        {"battery.health",{"Battery health","Batarya sağlığı"}},
        {"charge.mode",{"Charging preference","Şarj tercihi"}},
        {"charge.limit",{"Charge limit","Şarj sınırı"}},
        {"desktop.plasma",{"Brightness & rotation status (KDE)","Parlaklık ve döndürme durumu (KDE)"}},
        {"desktop.rotation",{"Automatic rotation (KDE)","Otomatik döndürme (KDE)"}},
        {"desktop.brightness",{"Automatic brightness (KDE)","Otomatik parlaklık (KDE)"}},
        {"flashlight.state",{"Flashlight on/off","Feneri aç/kapat"}},
        {"microphone.select",{"Microphone channel","Mikrofon kanalı"}},
        {"nfc.state",{"NFC on/off permission","NFC açma/kapatma izni"}},
        {"sensors.list",{"Available sensors","Mevcut sensörler"}},
        {"sensors.once",{"Read sensors once","Sensörleri bir kez oku"}},
        {"update.preview",{"Check for updates","Güncellemeleri denetle"}},
        {"update.apply",{"Install updates","Güncellemeleri kur"}}
    };
    const auto it=labels.find(a.id);return it==labels.end()?a.title:it->second;
}
inline client::Text summary(const client::Action& a) {
    static const std::map<std::string,client::Text> labels{
        {"charge.mode",{"Choose a charging preset. The device provider checks availability and keeps its protection rules enabled.","Şarj tercihini seçin. Cihaz sağlayıcısı kullanılabilirliği denetler ve koruma kurallarını sürdürür."}},
        {"charge.limit",{"Choose the requested charge limit. The provider must support this limit; inspect charging status after applying.","İstenen şarj sınırını seçin. Sağlayıcı bu sınırı desteklemelidir; uyguladıktan sonra şarj durumunu inceleyin."}},
        {"desktop.rotation",{"Allow automatic screen rotation in the original KDE session. An available KScreen provider is required.","Özgün KDE oturumunda otomatik ekran döndürmeye izin verin. Kullanılabilir KScreen sağlayıcısı gerekir."}},
        {"desktop.brightness",{"Allow automatic brightness in the original KDE session. An available desktop provider is required.","Özgün KDE oturumunda otomatik parlaklığa izin verin. Kullanılabilir masaüstü sağlayıcısı gerekir."}},
        {"flashlight.state",{"Turn the flashlight on or off using the existing session control.","Mevcut oturum kontrolüyle feneri açın veya kapatın."}},
        {"flashlight.level",{"Choose flashlight brightness. The two lit hardware steps are 50% and 100%; zero turns it off.","Fener parlaklığını seçin. Donanımın açık kademeleri yüzde 50 ve yüzde 100'dür; sıfır kapatır."}},
        {"microphone.select",{"Choose the microphone channel for new recordings. Existing recordings keep their current channel.","Yeni kayıtlar için mikrofon kanalını seçin. Mevcut kayıtlar geçerli kanalını korur."}},
        {"nfc.state",{"Allow or block the NFC radio. This permission does not start scanning or establish tag-reading support.","NFC radyosuna izin verin veya engelleyin. Bu izin tarama başlatmaz veya etiket okuma desteğini doğrulamaz."}},
        {"update.apply",{"Install the reviewed distribution update. Check the preview first and keep a recovery route available.","İncelenmiş dağıtım güncellemesini kurun. Önce önizlemeyi denetleyin ve kurtarma yolunu hazır tutun."}}
    };
    const auto it=labels.find(a.id);return it==labels.end()?a.description:it->second;
}
struct Item {std::string id;bool action=false;};
inline std::vector<Item> items(std::string_view id) {
    std::vector<Item> result;
    for(const auto& value:menus())if(value.parent==id)result.push_back({value.id,false});
    for(const auto read:{true,false})for(const auto& a:client::actions())
        if(owner(a)==id&&(a.effect==client::Effect::read)==read)result.push_back({a.id,true});
    return result;
}
}
