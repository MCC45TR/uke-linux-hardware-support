// SPDX-License-Identifier: MIT
#include "../src/display-device.hpp"
#include <cassert>
#include <iostream>
static drmModeConnector panel{},external{},second{};
static drmModeEncoder panel_encoder{},external_encoder{},second_encoder{};
static drmModeCrtc panel_crtc{},external_crtc{},second_crtc{};
static bool missing_encoder=false;
extern "C" drmModeConnectorPtr drmModeGetConnectorCurrent(int,uint32_t id) {
    return id==1?&panel:id==2?&external:id==3?&second:nullptr;
}
extern "C" void drmModeFreeConnector(drmModeConnectorPtr) {}
extern "C" drmModeEncoderPtr drmModeGetEncoder(int,uint32_t id) {
    return missing_encoder?nullptr:id==11?&panel_encoder:id==12?&external_encoder:id==13?&second_encoder:nullptr;
}
extern "C" void drmModeFreeEncoder(drmModeEncoderPtr) {}
extern "C" drmModeCrtcPtr drmModeGetCrtc(int,uint32_t id) {
    return id==84?&panel_crtc:id==85?&external_crtc:id==86?&second_crtc:nullptr;
}
extern "C" void drmModeFreeCrtc(drmModeCrtcPtr) {}
int main() {
    uint32_t connectors[]{2,1,3},crtcs[]{85,84,86};
    drmModeRes resources{};resources.connectors=connectors;resources.count_connectors=2;
    resources.crtcs=crtcs;resources.count_crtcs=3;
    panel.connector_type=DRM_MODE_CONNECTOR_DSI;panel.connection=DRM_MODE_CONNECTED;panel.encoder_id=11;
    external.connector_type=DRM_MODE_CONNECTOR_DisplayPort;external.connection=DRM_MODE_CONNECTED;external.encoder_id=12;
    panel_encoder.crtc_id=84;external_encoder.crtc_id=85;second_encoder.crtc_id=86;
    panel_crtc.crtc_id=84;panel_crtc.mode_valid=1;panel_crtc.mode.hdisplay=3200;panel_crtc.mode.vdisplay=2136;
    external_crtc=panel_crtc;external_crtc.crtc_id=85;
    auto selected=uke::test::internal_crtc(0,resources);
    assert(selected && selected->id==84 && selected->index==1);
    assert((uke::test::vblank_type(selected->index)&DRM_VBLANK_HIGH_CRTC_MASK)==(1<<DRM_VBLANK_HIGH_CRTC_SHIFT));
    panel.connection=DRM_MODE_DISCONNECTED;assert(!uke::test::internal_crtc(0,resources));
    panel.connection=DRM_MODE_CONNECTED;panel_crtc.mode_valid=0;assert(!uke::test::internal_crtc(0,resources));
    panel_crtc.mode_valid=1;panel_crtc.mode.hdisplay=1920;assert(!uke::test::internal_crtc(0,resources));
    panel_crtc.mode.hdisplay=3200;missing_encoder=true;assert(!uke::test::internal_crtc(0,resources));missing_encoder=false;
    panel_encoder.crtc_id=99;assert(!uke::test::internal_crtc(0,resources));panel_encoder.crtc_id=84;
    second=panel;second.encoder_id=13;second_crtc=panel_crtc;second_crtc.crtc_id=86;resources.count_connectors=3;
    bool refused=false;try {(void)uke::test::internal_crtc(0,resources);}catch(const std::runtime_error &) {refused=true;}
    assert(refused);
    refused=false;try {(void)uke::test::vblank_type(UINT32_MAX);}catch(const std::runtime_error &) {refused=true;}
    assert(refused);
    std::cout<<"Internal DSI selection, multiple CRTCs, vblank index and ambiguity refusals pass\n";
}
