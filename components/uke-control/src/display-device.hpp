// SPDX-License-Identifier: MIT
#pragma once
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <memory>
#include <optional>
#include <stdexcept>

namespace uke::test {
struct InternalCrtc { uint32_t id, index; };
inline std::optional<InternalCrtc> internal_crtc(int fd, const drmModeRes &resources) {
    std::optional<InternalCrtc> selected;
    for (int i=0;i<resources.count_connectors;i++) {
        std::unique_ptr<drmModeConnector,decltype(&drmModeFreeConnector)> connector(
            drmModeGetConnectorCurrent(fd,resources.connectors[i]),drmModeFreeConnector);
        if (!connector || connector->connector_type!=DRM_MODE_CONNECTOR_DSI ||
            connector->connection!=DRM_MODE_CONNECTED || !connector->encoder_id) continue;
        std::unique_ptr<drmModeEncoder,decltype(&drmModeFreeEncoder)> encoder(
            drmModeGetEncoder(fd,connector->encoder_id),drmModeFreeEncoder);
        if (!encoder || !encoder->crtc_id) continue;
        for (int index=0;index<resources.count_crtcs;index++) {
            if (resources.crtcs[index]!=encoder->crtc_id) continue;
            std::unique_ptr<drmModeCrtc,decltype(&drmModeFreeCrtc)> crtc(
                drmModeGetCrtc(fd,encoder->crtc_id),drmModeFreeCrtc);
            if (!crtc || !crtc->mode_valid || crtc->mode.hdisplay!=3200 ||
                crtc->mode.vdisplay!=2136) continue;
            if (selected && selected->id!=crtc->crtc_id)
                throw std::runtime_error("Ambiguous active internal Uke CRTCs");
            selected=InternalCrtc{crtc->crtc_id,uint32_t(index)};
        }
    }
    return selected;
}
inline drmVBlankSeqType vblank_type(uint32_t index) {
    if (index>uint32_t(DRM_VBLANK_HIGH_CRTC_MASK>>DRM_VBLANK_HIGH_CRTC_SHIFT))
        throw std::runtime_error("DRM CRTC index exceeds vblank ABI");
    return drmVBlankSeqType(DRM_VBLANK_RELATIVE|(index<<DRM_VBLANK_HIGH_CRTC_SHIFT));
}
}
