// SPDX-License-Identifier: MIT
#include <FidelityFX/host/ffx_fsr3upscaler.h>
#include <FidelityFX/host/ffx_frameinterpolation.h>
#include <FidelityFX/host/ffx_opticalflow.h>
#include <cassert>
#include <cstdio>

int main()
{
    static_assert(sizeof(wchar_t) == 4);
    assert(ffxFsr3UpscalerGetEffectVersion() == FFX_SDK_MAKE_VERSION(
        FFX_FSR3UPSCALER_VERSION_MAJOR, FFX_FSR3UPSCALER_VERSION_MINOR, FFX_FSR3UPSCALER_VERSION_PATCH));
    assert(ffxFrameInterpolationGetEffectVersion() == FFX_SDK_MAKE_VERSION(
        FFX_FRAMEINTERPOLATION_VERSION_MAJOR, FFX_FRAMEINTERPOLATION_VERSION_MINOR, FFX_FRAMEINTERPOLATION_VERSION_PATCH));
    assert(ffxOpticalflowGetEffectVersion() == FFX_SDK_MAKE_VERSION(
        FFX_OPTICALFLOW_VERSION_MAJOR, FFX_OPTICALFLOW_VERSION_MINOR, FFX_OPTICALFLOW_VERSION_PATCH));
    std::puts("PASS: SDK library load and CPU effect-version checks; no GPU dispatch");
}
