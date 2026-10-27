// SPDX-License-Identifier: MIT
// Bounded owned-image and DRM export diagnostic; no presentation/codec claim.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <string>
extern "C" {
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libavutil/log.h>
}

static void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
static void checked(int result, const char *operation) {
    if (result >= 0) return;
    char error[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(result, error, sizeof(error));
    throw std::runtime_error(std::string(operation)+": "+error);
}
using Frame = std::unique_ptr<AVFrame, void (*)(AVFrame *)>;
using Buffer = std::unique_ptr<AVBufferRef, void (*)(AVBufferRef *)>;
static Frame frame() {
    Frame f(av_frame_alloc(), [](AVFrame *p) { av_frame_free(&p); });
    require(bool(f), "frame allocation"); return f;
}
static Buffer buffer(AVBufferRef *p) {
    require(p, "buffer allocation");
    return {p, [](AVBufferRef *v) { av_buffer_unref(&v); }};
}
static Frame software(AVPixelFormat format) {
    auto f=frame(); f->format=format; f->width=130; f->height=98;
    checked(av_frame_get_buffer(f.get(),64), "software frame"); return f;
}
static void fill(AVFrame *f, unsigned iteration) {
    const unsigned bytes=f->format==AV_PIX_FMT_P010LE ? 2 : 1;
    for(unsigned p=0;p<2;++p) for(int y=0;y<f->height/(p?2:1);++y)
        for(int x=0;x<f->width;++x) {
            unsigned value=(x*17+y*7+p*199+iteration*13)%(bytes==2?1024:256);
            auto *out=f->data[p]+y*f->linesize[p]+x*bytes;
            if(bytes==2) {value<<=6;out[0]=value&255;out[1]=value>>8;} else out[0]=value;
        }
}
static bool equal(const AVFrame *a, const AVFrame *b) {
    if(a->format!=b->format || a->width!=b->width || a->height!=b->height) return false;
    const size_t row=a->width*(a->format==AV_PIX_FMT_P010LE ? 2 : 1);
    for(unsigned p=0;p<2;++p) for(int y=0;y<a->height/(p?2:1);++y)
        if(std::memcmp(a->data[p]+y*a->linesize[p],b->data[p]+y*b->linesize[p],row)) return false;
    return true;
}
static void run(AVBufferRef *device, AVBufferRef *drm_device, AVPixelFormat format, bool exported) {
    uint64_t modifier=0;
    VkImageDrmFormatModifierListCreateInfoEXT mods{};
    mods.sType=VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT;
    mods.drmFormatModifierCount=1;mods.pDrmFormatModifiers=&modifier;
    auto pool=buffer(av_hwframe_ctx_alloc(device));
    auto *fc=reinterpret_cast<AVHWFramesContext *>(pool->data);
    fc->format=AV_PIX_FMT_VULKAN;fc->sw_format=format;fc->width=130;fc->height=98;
    auto *vk=reinterpret_cast<AVVulkanFramesContext *>(fc->hwctx);
    if(exported) {
        vk->tiling=VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
        vk->create_pnext=&mods;
        vk->usage=VkImageUsageFlagBits(VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        vk->flags=AV_VK_FRAME_FLAG_DISABLE_MULTIPLANE;
    }
    checked(av_hwframe_ctx_init(pool.get()), "Vulkan frame pool");
    std::printf("pool format=%s exported=%u usage=%#x tiling=%u\n",
                format==AV_PIX_FMT_NV12?"NV12":"P010",unsigned(exported),vk->usage,unsigned(vk->tiling));
    Buffer drm_pool(nullptr, [](AVBufferRef *p) { av_buffer_unref(&p); });
    if(exported) {
        AVBufferRef *derived=nullptr;
        checked(av_hwframe_ctx_create_derived(&derived,AV_PIX_FMT_DRM_PRIME,drm_device,pool.get(),0),"DRM frame pool");
        drm_pool=buffer(derived);
    }
    for(unsigned iteration=0;iteration<8;++iteration) {
        auto input=software(format); fill(input.get(),iteration);
        auto gpu=frame(); checked(av_hwframe_get_buffer(pool.get(),gpu.get(),0),"Vulkan allocation");
        checked(av_hwframe_transfer_data(gpu.get(),input.get(),0),"upload");
        auto mapped=frame(); auto roundtrip=frame();
        AVFrame *download=gpu.get();
        if(exported) {
            mapped->format=AV_PIX_FMT_DRM_PRIME;mapped->hw_frames_ctx=av_buffer_ref(drm_pool.get());
            require(mapped->hw_frames_ctx,"DRM pool reference");
            checked(av_hwframe_map(mapped.get(),gpu.get(),AV_HWFRAME_MAP_READ|AV_HWFRAME_MAP_DIRECT),"Vulkan DRM export");
            require(mapped->hw_frames_ctx && mapped->hw_frames_ctx->data==drm_pool->data,
                    "exporter overwrote the requested DRM frames context");
            const auto *desc=reinterpret_cast<const AVDRMFrameDescriptor *>(mapped->data[0]);
            require(desc && desc->nb_objects>0 && desc->nb_objects<=4 && desc->nb_layers>0 && desc->nb_layers<=4,"DRM layout");
            for(int i=0;i<desc->nb_objects;++i) {
                require(desc->objects[i].fd>=0 && desc->objects[i].format_modifier==modifier,"DRM object/modifier");
                if(!iteration)std::printf("DRM object=%d bytes=%zu modifier=%#llx\n",i,desc->objects[i].size,static_cast<unsigned long long>(desc->objects[i].format_modifier));
            }
            roundtrip->format=AV_PIX_FMT_VULKAN;roundtrip->hw_frames_ctx=av_buffer_ref(pool.get());
            require(roundtrip->hw_frames_ctx,"Vulkan pool reference");
            checked(av_hwframe_map(roundtrip.get(),mapped.get(),AV_HWFRAME_MAP_READ|AV_HWFRAME_MAP_DIRECT),"DRM mapped roundtrip");
            download=roundtrip.get();
        }
        auto output=frame(); output->format=format;
        checked(av_hwframe_transfer_data(output.get(),download,0),"download");
        require(equal(input.get(),output.get()),"GPU roundtrip pixels");
    }
    std::printf("PASS:8 exact owned-image %s %s roundtrips\n",format==AV_PIX_FMT_NV12?"NV12":"P010",exported?"DRM export/map":"host transfer");
}
int main(int argc, char **argv) {
    try {
        std::setvbuf(stdout,nullptr,_IONBF,0);
        if(argc==2 && !std::strcmp(argv[1],"--self-check")) {
            for(auto format:{AV_PIX_FMT_NV12,AV_PIX_FMT_P010LE}) {
                auto a=software(format),b=software(format);fill(a.get(),3);fill(b.get(),3);
                require(equal(a.get(),b.get()),"equal padded pattern");
                b->data[1][0]^=1;require(!equal(a.get(),b.get()),"changed chroma rejected");
            }
            std::puts("PASS:CPU layout/equality self-check only; no GPU.");return 0;
        }
        require(argc==2 && !std::strcmp(argv[1],"/dev/dri/by-path/platform-3d00000.qcom_kgsl-3d0-render"),"Pass the verified Uke render alias or --self-check");
        av_log_set_level(AV_LOG_DEBUG);
        AVBufferRef *raw=nullptr;
        checked(av_hwdevice_ctx_create(&raw,AV_HWDEVICE_TYPE_DRM,argv[1],nullptr,0),"DRM device");
        auto drm=buffer(raw);raw=nullptr;
        checked(av_hwdevice_ctx_create_derived(&raw,AV_HWDEVICE_TYPE_VULKAN,drm.get(),0),"derived Vulkan device");
        auto device=buffer(raw);
        auto *dc=reinterpret_cast<AVHWDeviceContext *>(device->data);
        auto *vk=reinterpret_cast<AVVulkanDeviceContext *>(dc->hwctx);
        VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(vk->phys_dev,&props);
        require(props.deviceType==VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && std::strstr(props.deviceName,"732"),"physical Adreno732 required");
        std::printf("device=%s\n",props.deviceName);
        for(bool exported:{false,true}) for(auto format:{AV_PIX_FMT_NV12,AV_PIX_FMT_P010LE})run(device.get(),drm.get(),format,exported);
        return 0;
    } catch(const std::exception &e) {std::fprintf(stderr,"FAIL:%s\n",e.what());return 1;}
}
