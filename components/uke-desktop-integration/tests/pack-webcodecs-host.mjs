import assert from 'node:assert/strict';
import {makeJob} from './pack-webcodecs-corpus.mjs';
const dump=buffer=>Array.from({length:Math.ceil(buffer.length/16)},(_,index)=>{
  const part=buffer.subarray(index*16,index*16+16);
  const hex=part.toString('hex').match(/.{1,4}/g).join(' ');
  return (index*16).toString(16).padStart(8,'0')+': '+hex+'  ASCII';
}).join('\n');
const stream={index:0,width:320,height:240,nb_read_frames:'30',codec_name:'h264',
  profile:'Constrained Baseline',pix_fmt:'yuv420p',extradata:dump(Buffer.from('0142e01effe10000','hex'))};
const packets=Array.from({length:30},(_,n)=>({stream_index:0,size:'4',pts_time:String(n/30),
  flags:n===0?'K__':'___',data:dump(Buffer.from([0,0,1,9]))}));
const sha='a'.repeat(64);
const job=makeJob({streams:[stream],packets},sha);
assert.deepEqual(JSON.parse(JSON.stringify(job)),job);
assert.equal(job.input.codec,'avc1.42e01e');
assert.equal(job.input.chunks.length,30);
assert.equal(job.input.chunks[29].timestamp,966667);
const hvcc=Buffer.alloc(23); hvcc[0]=1;hvcc[1]=2;hvcc[2]=0x20;hvcc[6]=0xb0;hvcc[12]=93;
assert.equal(makeJob({streams:[{...stream,codec_name:'hevc',pix_fmt:'yuv420p10le',extradata:dump(hvcc)}],packets},sha).input.codec,
  'hvc1.2.4.L93.B0');
assert.equal(makeJob({streams:[{...stream,codec_name:'hevc',pix_fmt:'yuv420p10le',codec_tag_string:'hev1',extradata:dump(hvcc)}],packets},sha).input.codec,
  'hev1.2.4.L93.B0');
assert.equal(makeJob({streams:[{...stream,codec_name:'vp9',profile:'Profile 2',pix_fmt:'yuv420p10le'}],packets},sha)
  .input.codec,'vp09.02.31.10');
assert.throws(()=>makeJob({streams:[stream],packets:packets.slice(1)},sha),/thirty coded/);
assert.throws(()=>makeJob({streams:[{...stream,codec_name:'vp9',profile:'Profile 2',pix_fmt:'yuv420p12le'}],packets},sha),/Unsupported VP9/);
assert.throws(()=>makeJob({streams:[stream],packets:[{...packets[0],size:'5'},...packets.slice(1)]},sha),/Truncated/);
assert.throws(()=>makeJob({streams:[stream],packets:[{...packets[0],data:'00000004: 0000 0109  ASCII'},...packets.slice(1)]},sha),/Non-contiguous/);
console.log('FFprobe/WebCodecs packet conversion: passed (host fixtures only)');
