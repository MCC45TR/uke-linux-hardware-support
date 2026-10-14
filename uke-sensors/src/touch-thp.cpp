// SPDX-License-Identifier: GPL-3.0-or-later
#define UKE_TOUCH_NORMAL_TEST
#include "touch-normal-probe.cpp"
#include "touch-thp-geometry.hpp"
#include "touch-thp-packet.hpp"
#include <csignal>
#include <linux/gpio.h>
#include <linux/uinput.h>
#include <poll.h>

static volatile std::sig_atomic_t running=1;
static void stop_touch(int) { running=0; }
static int touch_irq() {
    // shortcut: the lab admits this measured GPIO controller; the kernel port will resolve the OF GPIO descriptor.
    require(compatible("/sys/bus/gpio/devices/gpiochip6/of_node/compatible","qcom,cliffs-pinctrl"),"Foreign GPIO controller");
    std::ifstream pin("/sys/bus/spi/devices/spi0.0/of_node/novatek,irq-gpio",std::ios::binary);
    std::array<unsigned char,12> cells{};
    require(bool(pin.read(reinterpret_cast<char *>(cells.data()),cells.size())) &&
            cells[4]==0 && cells[5]==0 && cells[6]==0 && cells[7]==54 &&
            cells[8]==0 && cells[9]==0 && cells[10]==0x20 && cells[11]==1,"Unqualified touch IRQ mapping");
    const int chip=open("/dev/gpiochip6",O_RDONLY|O_CLOEXEC); require(chip>=0,"GPIO controller unavailable");
    gpio_v2_line_request request{}; request.offsets[0]=54; request.num_lines=1;
    request.config.flags=GPIO_V2_LINE_FLAG_INPUT|GPIO_V2_LINE_FLAG_EDGE_RISING;
    const char name[]="uke-nt36532-thp-lab"; std::copy(std::begin(name),std::end(name),request.consumer);
    const int result=ioctl(chip,GPIO_V2_GET_LINE_IOCTL,&request); close(chip);
    require(result==0,"Touch IRQ input request failed"); return request.fd;
}
static unsigned consume_touch_irq(int fd) {
    // SPI exposes the latest matrix, so queued edges must not trigger repeated stale reads.
    std::array<gpio_v2_line_event,16> edges{};
    const auto bytes=::read(fd,edges.data(),sizeof(edges));
    require(bytes>0 && bytes%sizeof(edges[0])==0,"Touch IRQ batch read failed");
    return bytes/sizeof(edges[0]);
}
static void input_setup(int fd,const nvt::Geometry &geometry) {
    require(ioctl(fd,UI_SET_EVBIT,EV_ABS)==0 && ioctl(fd,UI_SET_EVBIT,EV_KEY)==0 &&
            ioctl(fd,UI_SET_KEYBIT,BTN_TOUCH)==0 && ioctl(fd,UI_SET_PROPBIT,INPUT_PROP_DIRECT)==0,
            "Touch input capabilities failed");
    for(const auto &[code,maximum]:std::array<std::pair<unsigned,int>,4>{{
        {ABS_MT_SLOT,nvt::kFingerSlots-1},{ABS_MT_TRACKING_ID,65535},
        {ABS_MT_POSITION_X,geometry.width-1},{ABS_MT_POSITION_Y,geometry.height-1}}}) {
        require(ioctl(fd,UI_SET_ABSBIT,code)==0,"Touch axis capability failed");
        uinput_abs_setup axis{}; axis.code=code; axis.absinfo.maximum=maximum;
        require(ioctl(fd,UI_ABS_SETUP,&axis)==0,"Touch axis setup failed");
    }
    uinput_setup device{};
    device.id.bustype=BUS_SPI;
    const char name[]="Novatek NT36532 Uke Touchscreen";
    std::copy(std::begin(name),std::end(name),device.name);
    require(ioctl(fd,UI_DEV_SETUP,&device)==0 && ioctl(fd,UI_SET_PHYS,"spi0.0/nt36532-uke-thp")==0 &&
            ioctl(fd,UI_DEV_CREATE)==0,"Touch input registration failed");
}
static void input_report(int fd,const std::vector<nvt::Slot> &slots,std::array<int,nvt::kFingerSlots> &ids,const nvt::Geometry &geometry) {
    std::array<bool,nvt::kFingerSlots> seen{};
    for(const auto &slot:slots) {
        require(slot.number>=0 && slot.number<int(ids.size()) && !seen[slot.number],"Invalid or duplicate touch slot");
        seen[slot.number]=true;
        require(slot.tracking_id>=0 && slot.tracking_id<=65535 &&
                slot.contact.x>=0 && slot.contact.x<geometry.width &&
                slot.contact.y>=0 && slot.contact.y<geometry.height,"Touch contact outside admitted range");
        require(std::count_if(slots.begin(),slots.end(),[&](const auto &other){return other.tracking_id==slot.tracking_id;})==1,
                "Duplicate touch tracking ID");
    }
    std::vector<input_event> events;
    const auto event=[&](unsigned type,unsigned code,int value) {
        input_event e{}; e.type=type; e.code=code; e.value=value; events.push_back(e);
    };
    // Legacy mousedev requires BTN_TOUCH before the coordinate events.
    event(EV_KEY,BTN_TOUCH,!slots.empty());
    for(unsigned index=0;index<ids.size();index++) {
        const auto found=std::find_if(slots.begin(),slots.end(),[&](const auto &s){return s.number==int(index);});
        const int id=found==slots.end() ? -1 : found->tracking_id;
        require(id>=-1 && id<=65535,"Touch tracking ID outside admitted range");
        if(id==ids[index] && id<0) continue;
        event(EV_ABS,ABS_MT_SLOT,index);
        if(id!=ids[index]) event(EV_ABS,ABS_MT_TRACKING_ID,id);
        if(found!=slots.end()) {
            event(EV_ABS,ABS_MT_POSITION_X,found->contact.x);
            // The stock framebuffer route is mirrored against the O82 electrode row axis.
            event(EV_ABS,ABS_MT_POSITION_Y,uke_touch::display_y(found->contact.y,geometry.height));
        }
        ids[index]=id;
    }
    event(EV_SYN,SYN_REPORT,0);
    require(::write(fd,events.data(),events.size()*sizeof(input_event))==ssize_t(events.size()*sizeof(input_event)),
            "Touch event publication failed");
}
int main(int argc,char **argv) {
    int spi=-1,input=-1,irq=-1;
    std::array<int,nvt::kFingerSlots> ids; ids.fill(-1);
    nvt::Geometry geometry;
    try {
        require(argc==2 && std::string(argv[1])=="--live-lab","Usage: uke-touch-thp --live-lab (600-second bounded trial)");
        require(compatible("/sys/firmware/devicetree/base/compatible","xiaomi,uke") &&
                compatible("/sys/bus/spi/devices/spi0.0/of_node/compatible","novatek,NVT-ts-spi"),"Foreign touch device");
        geometry=touch_geometry("/run/uke-touch-config.ini");
        nvt::TouchCore core(geometry);
        std::ifstream reference("/run/uke-touch-reference.bin",std::ios::binary);
        std::array<std::uint8_t,uke_touch::frame_size> reference_data{};
        require(bool(reference.read(reinterpret_cast<char *>(reference_data.data()),reference_data.size())) &&
                reference.peek()==EOF,"No qualified reference packet");
        const auto reference_frame=uke_touch::decode(reference_data);
        require(bool(reference_frame),"Reference packet is corrupt");
        core.process(reference_frame->matrix,reference_frame->counter,reference_frame->scan_mode);
        std::ifstream admission("/run/uke-touch-csot-admitted"); std::string token;
        require(bool(std::getline(admission,token)) && token=="uke-gpio26-input-high-cascade","Supplier preflight missing");
        spi=open("/dev/spidev0.0",O_RDWR|O_CLOEXEC); require(spi>=0,"Reviewed SPI leaf unavailable");
        lock_touch(spi);
        std::uint8_t mode=255;
        require(ioctl(spi,SPI_IOC_RD_MODE,&mode)==0 && mode==SPI_MODE_0,"Unexpected SPI mode");
        require(nt36532(read<6>(spi,0x1fb104)) && !(read<1>(spi,0x1fb12c)[0]&1),"Foreign chip/topology");
        const auto info=read<16>(spi,firmware_address(0));
        require(info[0]==0x11 && info[1]==0xee && info[2]==60 && info[3]==40 &&
                info[4]==0x7d && info[5]==0 && info[6]==0x53 && info[7]==0x70,"Resident normal firmware unavailable");
        std::ifstream ceiling("/sys/bus/spi/devices/spi0.0/of_node/spi-max-frequency",std::ios::binary);
        std::array<unsigned char,4> speed{};
        require(bool(ceiling.read(reinterpret_cast<char *>(speed.data()),speed.size())) &&
                speed==std::array<unsigned char,4>{1,0x24,0xf8,0},"Unqualified SPI ceiling");
        require(custom_read(spi,1)==1 && custom_read(spi,5)==0 && custom_read(spi,0x0e)==0,
                "Touch operating mode unavailable");
        irq=touch_irq();
        input=open("/dev/uinput",O_WRONLY|O_CLOEXEC); require(input>=0,"Kernel uinput is unavailable");
        input_setup(input,geometry);
        std::signal(SIGTERM,stop_touch); std::signal(SIGINT,stop_touch);
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(600);
        auto last_valid=std::chrono::steady_clock::now();
        unsigned frames=0,rejected=0,contacts=0,coalesced_edges=0;
        std::size_t previous_contacts=0;
        while(running && std::chrono::steady_clock::now()<deadline) {
            pollfd ready{irq,POLLIN,0};
            const int status=poll(&ready,1,100);
            if(status<0 && !running) break;
            require(status>=0,"Touch IRQ wait failed");
            if(!status) {
                input_report(input,{},ids,geometry); previous_contacts=0;
                continue;
            }
            coalesced_edges+=consume_touch_irq(irq)-1;
            auto page_select=select_page(0x11c400); transfer(spi,page_select.data(),nullptr,page_select.size());
            std::array<std::uint8_t,5434> tx{},rx{};
            transfer(spi,tx.data(),rx.data(),rx.size(),19200000);
            const auto frame=uke_touch::decode(std::span(rx).subspan(2));
            const auto now=std::chrono::steady_clock::now();
            if(frame) {
                last_valid=now; frames++;
                const auto result=core.process(frame->matrix,frame->counter,frame->scan_mode);
                input_report(input,result.slots,ids,geometry);
                contacts+=!result.slots.empty();
                if(result.slots.size()!=previous_contacts || (!result.slots.empty() && frames%30==0)) {
                    std::cout<<"frame="<<frames<<" contacts="<<result.slots.size();
                    for(const auto &s:result.slots) std::cout<<" [slot="<<s.number<<" id="<<s.tracking_id<<" x="<<s.contact.x<<" y="<<s.contact.y<<']';
                    std::cout<<std::endl; previous_contacts=result.slots.size();
                }
            } else {
                rejected++;
                if(now-last_valid>std::chrono::milliseconds(100)) {
                    input_report(input,{},ids,geometry); core.reset();
                    core.process(reference_frame->matrix,reference_frame->counter,reference_frame->scan_mode);
                    previous_contacts=0;
                }
            }
        }
        input_report(input,{},ids,geometry);
        require(ioctl(input,UI_DEV_DESTROY)==0,"Touch input cleanup failed");
        close(input); input=-1; close(spi); spi=-1; close(irq); irq=-1;
        std::cout<<"valid="<<frames<<" rejected="<<rejected<<" contact_frames="<<contacts<<" coalesced_edges="<<coalesced_edges<<'\n';
        (void)&load_normal; (void)&normal_image; (void)&inspect_normal; (void)&probe_coordinates;
        return 0;
    } catch(const std::exception &e) {
        if(input>=0) { try{input_report(input,{},ids,geometry);}catch(...){} ioctl(input,UI_DEV_DESTROY); close(input); }
        if(spi>=0) close(spi);
        if(irq>=0) close(irq);
        std::cerr<<e.what()<<'\n'; return 1;
    }
}
