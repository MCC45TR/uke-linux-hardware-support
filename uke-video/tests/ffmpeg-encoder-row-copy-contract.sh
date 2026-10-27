#!/usr/bin/env bash
# Actual FFmpeg upload functions with guarded source/destination pages; host only.
set -Eeuo pipefail
export LC_ALL=C
ulimit -c 0
source_root=$(realpath "${1:?Prepared FFmpeg source}")
headers=$(realpath "${2:?Configured FFmpeg build headers}")
work=$(realpath -m "${3:?New fixture output directory}")
[[ ! -e $work ]]
mkdir -m 700 "$work"
for name in v4l2_bufref_to_buf v4l2_buffer_copy_semiplanar v4l2_buffer_swframe_to_buf; do
 awk -v name="$name" '$0 ~ "^static int " name "\\(" {inside=1} inside {print} inside && /^}/ {exit}' \
  "$source_root/libavcodec/v4l2_buffers.c" >> "$work/functions.c"
done
grep -q '^static int v4l2_buffer_swframe_to_buf' "$work/functions.c"
cat > "$work/test.c" <<'C'
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "libavutil/common.h"
#include "libavutil/error.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/v4l2_context.h"
#include "libavcodec/v4l2_buffers.h"
static const AVPixFmtDescriptor nv = {
 .nb_components=3,.log2_chroma_h=1,.comp={{.plane=0},{.plane=1},{.plane=1}}
};
const AVPixFmtDescriptor *av_pix_fmt_desc_get(enum AVPixelFormat format)
{ (void)format; return &nv; }
#include "functions.c"
#undef assert
#define assert(test) do { if (!(test)) { fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#test); exit(1); } } while(0)
static void fault(int signal) { (void)signal; _exit(41); }
typedef struct Guard { unsigned char *mapping,*data; size_t length; } Guard;
static Guard guarded(size_t bytes)
{
 size_t page=sysconf(_SC_PAGESIZE),rounded=(bytes+page-1)/page*page;
 Guard g={.length=rounded+page};
 g.mapping=mmap(NULL,g.length,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
 assert(g.mapping!=MAP_FAILED);
 assert(mprotect(g.mapping+rounded,page,PROT_NONE)==0);
 g.data=g.mapping+rounded-bytes; return g;
}
static void run(enum AVPixelFormat format,int w,int h,int pitch,int coded,int negative,int multi)
{
 const int sample=format==AV_PIX_FMT_P010LE?2:1;
 int widths[2]={w*sample,((w+1)/2)*2*sample},rows[2]={h,(h+1)/2};
 int strides[2]={widths[0]+8,widths[1]+16};
 size_t total=(size_t)pitch*(coded+(coded+1)/2);
 Guard src[2]={guarded((size_t)strides[0]*rows[0]),guarded((size_t)strides[1]*rows[1])};
 Guard dst=guarded(total);
 V4L2Context context={0}; V4L2Buffer out={0}; AVFrame frame={0};
 context.format.type=multi?V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:V4L2_BUF_TYPE_VIDEO_OUTPUT;
 if(multi){context.format.fmt.pix_mp.width=w;context.format.fmt.pix_mp.height=coded;}
 else{context.format.fmt.pix.width=w;context.format.fmt.pix.height=coded;}
 unsigned fourcc=format==AV_PIX_FMT_P010LE?V4L2_PIX_FMT_P010:format==AV_PIX_FMT_NV21?V4L2_PIX_FMT_NV21:V4L2_PIX_FMT_NV12;
 if(multi)context.format.fmt.pix_mp.pixelformat=fourcc;else context.format.fmt.pix.pixelformat=fourcc;
 out.context=&context;out.num_planes=1;out.buf.type=context.format.type;
 out.plane_info[0].mm_addr=dst.data;out.plane_info[0].length=total;out.plane_info[0].bytesperline=pitch;
 frame.format=format;frame.width=w;frame.height=h;
 for(int p=0;p<2;p++){
  memset(src[p].data,0xa5,(size_t)strides[p]*rows[p]);
  frame.linesize[p]=negative?-strides[p]:strides[p];
  frame.data[p]=src[p].data+(negative?(size_t)(rows[p]-1)*strides[p]:0);
  for(int r=0;r<rows[p];r++)for(int x=0;x<widths[p];x++)
   frame.data[p][(ptrdiff_t)r*frame.linesize[p]+x]=(p*83+r*7+x*3)%251;
 }
 assert(v4l2_buffer_swframe_to_buf(&frame,&out)==0);
 assert((multi?out.planes[0].bytesused:out.buf.bytesused)==total);
 for(int p=0;p<2;p++){
  int coded_rows=p?(coded+1)/2:coded,unit=sample*(p?2:1);
  unsigned char *base=dst.data+(p?(size_t)pitch*coded:0);
  for(int r=0;r<coded_rows;r++)for(int x=0;x<pitch;x++){
   int sy=r<rows[p]?r:rows[p]-1;
   int sx=x<widths[p]?x:widths[p]-unit+(x-widths[p])%unit;
   assert(base[(size_t)r*pitch+x]==frame.data[p][(ptrdiff_t)sy*frame.linesize[p]+sx]);
  }
 }
 memset(dst.data,0xcc,total);out.plane_info[0].length=total-1;
 assert(v4l2_buffer_swframe_to_buf(&frame,&out)==AVERROR(EINVAL));
 for(size_t i=0;i<total;i++)assert(dst.data[i]==0xcc);
 out.plane_info[0].length=total;out.plane_info[0].bytesperline=widths[1]-1;
 assert(v4l2_buffer_swframe_to_buf(&frame,&out)==AVERROR(EINVAL));
 out.plane_info[0].bytesperline=pitch;frame.linesize[1]=widths[1]-1;
 assert(v4l2_buffer_swframe_to_buf(&frame,&out)==AVERROR(EINVAL));
 frame.linesize[1]=negative?-strides[1]:strides[1];frame.height=coded+1;
 assert(v4l2_buffer_swframe_to_buf(&frame,&out)==AVERROR(EINVAL));
 frame.height=h;frame.data[1]=NULL;
 assert(v4l2_buffer_swframe_to_buf(&frame,&out)==AVERROR(EINVAL));
 for(int p=0;p<2;p++)assert(munmap(src[p].mapping,src[p].length)==0);
 assert(munmap(dst.mapping,dst.length)==0);
}
int main(void)
{
 signal(SIGSEGV,fault);signal(SIGBUS,fault);
 run(AV_PIX_FMT_NV12,640,360,672,384,0,1);
 const enum AVPixelFormat formats[]={AV_PIX_FMT_NV12,AV_PIX_FMT_NV21,AV_PIX_FMT_P010LE};
 for(unsigned i=0;i<3;i++)for(int negative=0;negative<2;negative++)for(int multi=0;multi<2;multi++){
  int sample=formats[i]==AV_PIX_FMT_P010LE?2:1;
  run(formats[i],321,241,672*sample,256,negative,multi);
  run(formats[i],320,240,336*sample,240,negative,multi);
 }
 puts("PASS: actual NV12/NV21/P010 upload, visible/odd/padded rows, independent strides, negative stride, edge extension, single/multi API and invalid bounds; no device.");
 return 0;
}
C
"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-sign-compare -I"$headers" -I"$source_root" \
 "$work/test.c" -o "$work/test" > "$work/build.log" 2>&1
"$work/test"
