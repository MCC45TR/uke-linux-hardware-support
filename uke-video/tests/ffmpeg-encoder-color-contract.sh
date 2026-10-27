#!/usr/bin/env bash
# Actual upstream setter, modeled ioctl; host only, no device node.
set -Eeuo pipefail
export LC_ALL=C
source_root=$(realpath "${1:?Prepared FFmpeg source}")
headers=$(realpath "${2:?Configured FFmpeg build headers}")
work=$(realpath -m "${3:?New fixture output directory}")
[[ ! -e $work ]]
mkdir -m 700 "$work"
input=$source_root/libavcodec/v4l2_context.c
for function_name in ctx_to_m2mctx ff_v4l2_context_set_format; do
  awk -v function_name="$function_name" '
    $0 ~ "^[a-zA-Z_].*" function_name "\\(" {inside=1}
    inside{print}
    inside && /^}/ {exit}
  ' "$input" >> "$work/functions.c"
done
grep -q '^int ff_v4l2_context_set_format' "$work/functions.c"
cat > "$work/test.c" <<'C'
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <linux/videodev2.h>
#include "libavcodec/avcodec.h"
#include "libavcodec/v4l2_m2m.h"
#include "libavutil/error.h"
#ifndef container_of
#define container_of(p,t,m) ((t *)((char *)(p)-offsetof(t,m)))
#endif
static AVCodec encoder, decoder;
int av_codec_is_encoder(const AVCodec *codec) { return codec == &encoder; }
void av_log(void *context, int level, const char *format, ...) { (void)context; (void)level; (void)format; }
static struct v4l2_format sent;
static int calls, mode, failures;
static int model_ioctl(int fd, unsigned long request, ...) {
    va_list args;
    struct v4l2_format *format;
    if (fd != 77 || request != VIDIOC_S_FMT) return -1;
    va_start(args, request); format = va_arg(args, struct v4l2_format *); va_end(args);
    sent = *format; calls++;
    if (mode == 1) { errno = EBUSY; return -1; }
    if (mode == 2) {
        if (V4L2_TYPE_IS_MULTIPLANAR(format->type)) format->fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_DEFAULT;
        else format->fmt.pix.xfer_func = V4L2_XFER_FUNC_DEFAULT;
    }
    if (mode == 3) {
        if (V4L2_TYPE_IS_MULTIPLANAR(format->type)) format->fmt.pix_mp.quantization = V4L2_QUANTIZATION_DEFAULT;
        else format->fmt.pix.quantization = V4L2_QUANTIZATION_DEFAULT;
    }
    if (mode == 4) {
        if (V4L2_TYPE_IS_MULTIPLANAR(format->type)) format->fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
        else format->fmt.pix.colorspace = V4L2_COLORSPACE_DEFAULT;
    }
    return 0;
}
#define ioctl model_ioctl
#include "functions.c"
static void expect(int passed, const char *name) {
    if (!passed) { fprintf(stderr, "FAIL: %s\n", name); failures++; }
}
static void reset(V4L2m2mContext *s, AVCodecContext *a, int type) {
    memset(s, 0, sizeof(*s)); memset(a, 0, sizeof(*a)); memset(&sent, 0, sizeof(sent));
    calls = mode = 0; s->fd = 77; s->avctx = a; a->codec = &encoder;
    a->color_primaries = AVCOL_PRI_BT709; a->color_trc = AVCOL_TRC_BT709;
    a->colorspace = AVCOL_SPC_BT709; a->color_range = AVCOL_RANGE_MPEG;
    s->output.type = type; s->output.format.type = type;
    s->output.av_codec_id = AV_CODEC_ID_RAWVIDEO;
    s->capture.type = type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ?
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->capture.format.type = s->capture.type;
}
static int fields(int cs, int xf, int yc, int qt) {
    if (V4L2_TYPE_IS_MULTIPLANAR(sent.type))
        return sent.fmt.pix_mp.colorspace == (unsigned)cs && sent.fmt.pix_mp.xfer_func == xf &&
               sent.fmt.pix_mp.ycbcr_enc == yc && sent.fmt.pix_mp.quantization == qt;
    return sent.fmt.pix.colorspace == (unsigned)cs && sent.fmt.pix.xfer_func == (unsigned)xf &&
           sent.fmt.pix.ycbcr_enc == (unsigned)yc && sent.fmt.pix.quantization == (unsigned)qt;
}
int main(void) {
    V4L2m2mContext s;
    AVCodecContext a;
    int types[] = { V4L2_BUF_TYPE_VIDEO_OUTPUT, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE };
    for (unsigned i=0;i<2;i++) {
        int type=types[i];
        reset(&s,&a,type);
        expect(ff_v4l2_context_set_format(&s.output)==0 && calls==1 &&
               fields(V4L2_COLORSPACE_REC709,V4L2_XFER_FUNC_709,V4L2_YCBCR_ENC_709,V4L2_QUANTIZATION_LIM_RANGE), "709 limited sent");
        if (!V4L2_TYPE_IS_MULTIPLANAR(type)) expect(sent.fmt.pix.priv==V4L2_PIX_FMT_PRIV_MAGIC,"single-plane extended fields enabled");
        reset(&s,&a,type); a.color_primaries=AVCOL_PRI_BT2020; a.color_trc=AVCOL_TRC_SMPTE2084; a.colorspace=AVCOL_SPC_BT2020_NCL;
        expect(ff_v4l2_context_set_format(&s.output)==0 &&
               fields(V4L2_COLORSPACE_BT2020,V4L2_XFER_FUNC_SMPTE2084,V4L2_YCBCR_ENC_BT2020,V4L2_QUANTIZATION_LIM_RANGE), "2020 PQ mapped, not hardware-qualified");
        reset(&s,&a,type); a.color_range=AVCOL_RANGE_JPEG;
        expect(ff_v4l2_context_set_format(&s.output)==0 && fields(V4L2_COLORSPACE_REC709,V4L2_XFER_FUNC_709,V4L2_YCBCR_ENC_709,V4L2_QUANTIZATION_FULL_RANGE),"direct API range mapping, not advertised support");
        reset(&s,&a,type); a.color_primaries=AVCOL_PRI_UNSPECIFIED; a.color_trc=AVCOL_TRC_UNSPECIFIED; a.colorspace=AVCOL_SPC_UNSPECIFIED; a.color_range=AVCOL_RANGE_UNSPECIFIED;
        expect(ff_v4l2_context_set_format(&s.output)==0 && fields(0,0,0,0),"unspecified defaults retained");
        reset(&s,&a,type); a.color_primaries=AVCOL_PRI_UNSPECIFIED; a.color_trc=AVCOL_TRC_UNSPECIFIED; a.colorspace=AVCOL_SPC_UNSPECIFIED;
        expect(ff_v4l2_context_set_format(&s.output)==0 && fields(0,0,0,V4L2_QUANTIZATION_LIM_RANGE),"range-only request retained");
        reset(&s,&a,type); a.codec=&decoder;
        expect(ff_v4l2_context_set_format(&s.output)==0 && fields(0,0,0,0),"decoder path unchanged");
        reset(&s,&a,type);
        expect(ff_v4l2_context_set_format(&s.capture)==0 && fields(0,0,0,0),"capture path unchanged");
        reset(&s,&a,type); a.color_trc=AVCOL_TRC_ARIB_STD_B67;
        expect(ff_v4l2_context_set_format(&s.output)==AVERROR(EINVAL) && !calls,"unrepresentable HLG refused before ioctl");
        reset(&s,&a,type); a.color_primaries=AVCOL_PRI_UNSPECIFIED;
        expect(ff_v4l2_context_set_format(&s.output)==AVERROR(EINVAL) && !calls,"incomplete tuple not invented");
        reset(&s,&a,type); a.color_range=(enum AVColorRange)99;
        expect(ff_v4l2_context_set_format(&s.output)==AVERROR(EINVAL) && !calls,"unknown range refused");
        reset(&s,&a,type); mode=2;
        expect(ff_v4l2_context_set_format(&s.output)==AVERROR(EINVAL) && calls==1,"driver transfer coercion refused");
        reset(&s,&a,type); mode=3;
        expect(ff_v4l2_context_set_format(&s.output)==AVERROR(EINVAL) && calls==1,"driver range coercion refused");
        reset(&s,&a,type); mode=4; a.color_primaries=AVCOL_PRI_BT2020; a.color_trc=AVCOL_TRC_SMPTE2084; a.colorspace=AVCOL_SPC_BT2020_NCL;
        expect(ff_v4l2_context_set_format(&s.output)==AVERROR(EINVAL) && calls==1,"driver 2020 colorspace coercion refused");
        reset(&s,&a,type); mode=1;
        expect(ff_v4l2_context_set_format(&s.output)==-1 && calls==1,"ioctl error retained");
    }
    if (failures) return 1;
    puts("PASS: actual encoder S_FMT setter maps explicit color/range, rejects loss and preserves other paths; 28 modeled cases, no hardware.");
    return 0;
}
C
"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror -I"$headers" -I"$source_root" \
    "$work/test.c" -o "$work/test" > "$work/build.log" 2>&1
"$work/test"
