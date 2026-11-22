// SPDX-License-Identifier: MIT
#define UKE_REBOOT_TEST
#include "../src/reboot.cpp"
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include <sys/socket.h>
struct FakeLogin {
    std::string parameter="normal";
    std::vector<std::string> calls;
    bool setter_failure=false,reboot_failure=false;
};
static int get_parameter(sd_bus *,const char *,const char *,const char *,sd_bus_message *reply,void *data,sd_bus_error *) {
    return sd_bus_message_append(reply,"s",static_cast<FakeLogin *>(data)->parameter.c_str());
}
static int set_parameter(sd_bus_message *message,void *data,sd_bus_error *error) {
    auto &fake=*static_cast<FakeLogin *>(data);
    const char *value=nullptr;
    int result=sd_bus_message_read(message,"s",&value); if(result<0) return result;
    fake.calls.emplace_back(std::string("set:")+value);
    if(fake.setter_failure) return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"fixture setter refusal");
    fake.parameter=value;
    return sd_bus_reply_method_return(message,"");
}
static int reboot_request(sd_bus_message *message,void *data,sd_bus_error *error) {
    auto &fake=*static_cast<FakeLogin *>(data);
    int interactive=1;
    assert(sd_bus_message_read(message,"b",&interactive)>0 && !interactive);
    fake.calls.emplace_back("reboot");
    if(fake.reboot_failure) return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"fixture inhibitor");
    return sd_bus_reply_method_return(message,"");
}
static void trial(const char *mode,bool setter_failure,bool reboot_failure) {
    int sockets[2]; assert(socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,sockets)==0);
    sd_bus *server=nullptr,*client=nullptr; sd_id128_t id{};
    assert(sd_id128_randomize(&id)>=0);
    assert(sd_bus_new(&server)>=0 && sd_bus_new(&client)>=0);
    assert(sd_bus_set_fd(server,sockets[0],sockets[0])>=0);
    assert(sd_bus_set_server(server,1,id)>=0);
    assert(sd_bus_set_fd(client,sockets[1],sockets[1])>=0);
    FakeLogin fake; fake.setter_failure=setter_failure; fake.reboot_failure=reboot_failure;
    static const sd_bus_vtable table[]={
        SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("RebootParameter","s",get_parameter,0,0),
        SD_BUS_METHOD("SetRebootParameter","s","",set_parameter,0),
        SD_BUS_METHOD("Reboot","b","",reboot_request,0),
        SD_BUS_VTABLE_END
    };
    assert(sd_bus_add_object_vtable(server,nullptr,"/org/freedesktop/login1","org.freedesktop.login1.Manager",table,&fake)>=0);
    assert(sd_bus_start(server)>=0 && sd_bus_start(client)>=0);
    std::atomic<bool> stop=false;
    std::thread worker([&] {
        while(!stop) {int r=sd_bus_process(server,nullptr); if(r<0) break; if(!r) sd_bus_wait(server,10000);}
    });
    const int result=request_reboot(client,mode);
    stop=true; worker.join();
    sd_bus_flush_close_unref(client); sd_bus_flush_close_unref(server);
    assert((result<0)==(setter_failure||reboot_failure));
    assert(fake.calls.front()==std::string("set:")+mode);
    if(setter_failure) {assert(fake.calls.size()==1 && fake.parameter=="normal");}
    else if(reboot_failure) {
        assert(fake.calls==std::vector<std::string>({std::string("set:")+mode,"reboot","set:normal"}));
        assert(fake.parameter=="normal");
    } else {assert(fake.calls.size()==2 && fake.calls[1]=="reboot" && fake.parameter==mode);}
}
int main() {
    assert(std::string(reboot_mode("recovey"))=="recovery");
    assert(reboot_mode("bootloader") && reboot_mode("recovery"));
    assert(!reboot_mode("boot_a") && !reboot_mode("--force") && !reboot_mode("recovery; reboot"));
    char directory[]="/tmp/uke-reboot-test.XXXXXX"; assert(mkdtemp(directory));
    const std::filesystem::path root=directory,base=root/"dt",driver=root/"driver";
    std::filesystem::create_directories(base); std::filesystem::create_directories(driver/"device");
    std::ofstream(base/"mode-recovery",std::ios::binary).write("\0\0\0\1",4);
    std::ofstream(base/"mode-bootloader",std::ios::binary).write("\0\0\0\2",4);
    assert(!reboot_backend(base,driver));
    std::filesystem::create_directory_symlink(base,driver/"device/of_node");
    assert(reboot_backend(base,driver));
    std::ofstream(base/"mode-recovery",std::ios::binary).write("\0\0\0\2",4);
    assert(!reboot_backend(base,driver));
    std::filesystem::remove_all(root);
    trial("recovery",false,false); trial("bootloader",false,false);
    trial("recovery",true,false); trial("bootloader",false,true);
    std::cout<<"Native reboot mapping, bound backend, D-Bus order and refusal rollback tests passed; no host reboot call\n";
}
