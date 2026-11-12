// SPDX-License-Identifier: MIT
// Transform only the authenticated copies extracted from the accepted SRPMs.
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const [root, owner, manifest] = process.argv.slice(2);
const contract = JSON.parse(fs.readFileSync(manifest));
const digest = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
for (const name of ['auth', 'desktop']) {
    for (const entry of contract.inputs[name].files)
        assert.equal(digest(`${root}/${name}/${entry.path}`), entry.sha256, entry.path);
}
const path = `${root}/auth/uke-pogo-auth/src/main.cpp`;
const before = fs.readFileSync(path, 'utf8');
const admission = `
// Public attachment metadata only; no firmware, authentication or TEE access.
static std::string admission_sysfs(const fs::path &path) {
    const int descriptor=open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(descriptor<0)return {};
    Fd fd(descriptor);struct statfs info{};char bytes[1024];
    const auto size=fstatfs(fd.value,&info)==0&&info.f_type==SYSFS_MAGIC?read(fd.value,bytes,sizeof(bytes)):-1;
    return size>0&&size<ssize_t(sizeof(bytes))?std::string(bytes,size):std::string{};
}
static int attached_condition() {
    const auto identity=admission_sysfs("/sys/firmware/devicetree/base/compatible");
    if(!uke_pogo_desktop::compatible(identity,"xiaomi,uke")||
       !uke_pogo_desktop::compatible(identity,"qcom,sm7675"))return 255;
    return uke_pogo_desktop::present("/sys",admission_sysfs)?0:255;
}
`;
const oldWait = `    void wait() {
        pollfd event{request_.value,POLLPRI|POLLERR,0};const auto status=poll(&event,1,1000);
        need(status>=0||errno==EINTR,"Cannot poll the pogo authentication bridge");
        if(status>0)need(event.revents&POLLPRI,"Pogo authentication bridge was removed");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
`;
const replacements = [
    ['#include <thread>\n', '#include "control-pogo-runtime.hpp"\n#include "control-pogo-state.hpp"\n'],
    ['static volatile sig_atomic_t stopping=0;\nstatic void stop(int){stopping=1;}\n', 'using uke_pogo_runtime::stopping;\n'],
    ['token(release)&&release.starts_with("7.3.1-")&&release.ends_with("-unstable")',
     'token(release)&&uke_pogo_runtime::published_unstable_release(release)'],
    ['Usage: uke-pogo-auth --check [KERNEL_RELEASE] | --serve | --once',
     'Usage: uke-pogo-auth --check [KERNEL_RELEASE] | --serve | --once | --bridge-attached'],
    [oldWait, '    void wait(int stop_fd) {\n        (void)uke_pogo_runtime::wait_for_bridge_change(request_.value,stop_fd);\n    }\n'],
    ['static int run(int argc,char **argv) {\n', admission+'static int run(int argc,char **argv) {\n    if(argc==2&&std::string_view(argv[1])=="--bridge-attached")return attached_condition();\n'],
    ['    struct sigaction action{};action.sa_handler=stop;sigemptyset(&action.sa_mask);\n    need(!sigaction(SIGTERM,&action,nullptr)&&!sigaction(SIGINT,&action,nullptr),"Cannot install stop handlers");\n',
     '    uke_pogo_runtime::StopWakeup stop_channel;\n'],
    ['        bridge.wait();\n', '        bridge.wait(stop_channel.descriptor());\n'],
];
let after = before;
for (const [old, value] of replacements) {
    assert.equal(after.split(old).length, 2, 'Ambiguous pogo source delta');
    after = after.replace(old, () => value);
}
let inverse = after;
for (const [old, value] of replacements.toReversed()) {
    assert.equal(inverse.split(value).length, 2, 'Ambiguous pogo source inverse');
    inverse = inverse.replace(value, () => old);
}
assert.equal(inverse, before, 'Unrelated authentication source changed');
fs.writeFileSync(path, after);
fs.copyFileSync(`${owner}/src/pogo-runtime.hpp`, `${root}/auth/uke-pogo-auth/src/control-pogo-runtime.hpp`);
fs.copyFileSync(`${root}/desktop/src/state.hpp`, `${root}/auth/uke-pogo-auth/src/control-pogo-state.hpp`);
const unit = `${root}/desktop/packaging/pogo-attachment.conf`;
const old = fs.readFileSync(unit, 'utf8');
assert.equal(old.split('/usr/libexec/uke-pogo-desktop --bridge-attached').length, 2);
fs.writeFileSync(unit, old.replace('/usr/libexec/uke-pogo-desktop --bridge-attached',
    '/usr/libexec/uke-pogo-auth --bridge-attached'));
console.log('Pogo source inverse passed; protocol, TA and original desktop C++ remain exact.');
