// Host-only conversion of FFprobe packet JSON; no demuxer or runtime is shipped to Uke.
import assert from 'node:assert/strict';
import {readFile,stat} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';

function unhex(dump) {
  assert.equal(typeof dump,'string','Missing FFprobe hex data');
  const parts=[];
  let offset=0;
  for(const line of dump.split('\n')) {
    if(!line.trim()) continue;
    const match=line.match(/^([a-fA-F0-9]{8}):\s+(.*)$/);
    assert(match,'Invalid FFprobe hex line');
    assert.equal(parseInt(match[1],16),offset,'Non-contiguous FFprobe hex data');
    const hex=match[2].split(/ {2,}/)[0].replaceAll(' ','');
    assert(/^(?:[0-9a-fA-F]{2})+$/.test(hex),'Invalid FFprobe hex bytes');
    const part=Buffer.from(hex,'hex'); parts.push(part); offset+=part.length;
  }
  return Buffer.concat(parts);
}
export function makeJob(probe,sourceSHA256) {
  assert(/^[a-f0-9]{64}$/.test(sourceSHA256),'Supply the original stream SHA-256');
  assert.equal(probe.streams?.length,1,'Select exactly one video stream');
  const stream=probe.streams[0];
  const hd=stream.width===1920 && stream.height===1080;
  assert(hd || (stream.width===320 && stream.height===240),'Use a reviewed decode size');
  assert.equal(Number(stream.nb_read_frames),30,'Expected thirty software-reference frames');
  assert.equal(probe.packets?.length,30,'Expected thirty coded packets');
  let codec,description;
  if(stream.codec_name==='h264') {
    description=unhex(stream.extradata);
    assert(description.length>=7 && description[0]===1,'Missing AVC configuration record');
    assert([66,77,100].includes(description[1]) && ['yuv420p','yuvj420p'].includes(stream.pix_fmt),
      'Unsupported H264 profile/depth');
    codec='avc1.'+description.subarray(1,4).toString('hex');
  } else if(stream.codec_name==='hevc') {
    description=unhex(stream.extradata);
    assert(description.length>=23 && description[0]===1,'Missing HEVC configuration record');
    const profile=description[1]&31,tier=description[1]&32?'H':'L';
    assert([1,2].includes(profile) && description[1]>>6===0,'Unsupported HEVC profile space');
    assert((profile===1 && stream.pix_fmt==='yuv420p') ||
      (profile===2 && stream.pix_fmt==='yuv420p10le'),'Use true 8/10-bit HEVC reference inputs');
    let flags=BigInt(description.readUInt32BE(2)),reversed=0n;
    for(let bit=0;bit<32;bit++) {reversed=(reversed<<1n)|(flags&1n);flags>>=1n;}
    const constraints=Array.from(description.subarray(6,12));
    while(constraints.at(-1)===0) constraints.pop();
    const tag=stream.codec_tag_string==='hev1'?'hev1':'hvc1';
    codec=tag+'.'+profile+'.'+reversed.toString(16)+'.'+tier+description[12]+
      (constraints.length?'.'+constraints.map(v=>v.toString(16).toUpperCase()).join('.'):'');
  } else {
    assert.equal(stream.codec_name,'vp9','Unsupported smoke codec');
    assert(['yuv420p','yuv420p10le'].includes(stream.pix_fmt),'Unsupported VP9 pixel depth');
    const profile=Number(String(stream.profile).replace(/^Profile\s*/,'')),depth=
      /10/.test(stream.pix_fmt)?10:8;
    assert((profile===0 && depth===8)||(profile===2 && depth===10),'Unsupported VP9 profile/depth');
    codec='vp09.0'+profile+'.'+(hd?'51':'31')+'.'+(depth===8?'08':'10');
  }
  let total=0;
  const chunks=probe.packets.map(packet=>{
    assert.equal(packet.stream_index,stream.index,'Packet belongs to another stream');
    const data=unhex(packet.data);
    assert(data.length>0 && data.length===Number(packet.size),'Truncated packet data');
    total+=data.length; assert(total<=(hd?32:8)*1024*1024,'Smoke corpus too large');
    const timestamp=Math.round(Number(packet.pts_time)*1000000);
    assert(Number.isSafeInteger(timestamp),'Missing or invalid packet PTS');
    return {type:packet.flags?.includes('K')?'key':'delta',timestamp,dataBase64:data.toString('base64')};
  });
  assert.equal(chunks[0].type,'key','Corpus must begin with a key packet');
  assert.equal(new Set(chunks.map(chunk=>chunk.timestamp)).size,30,'Repeated packet PTS');
  const input={codec,width:stream.width,height:stream.height,expectedFrames:30,sourceSHA256,chunks,
    sourceProfile:stream.profile??null,sourceLevel:stream.level??null,sourcePixelFormat:stream.pix_fmt??null,
    sourceColor:{primaries:stream.color_primaries??null,transfer:stream.color_transfer??null,
      matrix:stream.color_space??null,range:stream.color_range??null}};
  if(stream.codec_name==='vp9') {
    input.requestedVp9Level=hd?51:31;
    input.vp9LevelConformanceAccepted=false;
  }
  if(description) input.descriptionBase64=description.toString('base64');
  return {operation:'decode',input};
}
if(process.argv[1] && import.meta.url===pathToFileURL(process.argv[1]).href) {
  assert((await stat(process.argv[2])).size<=256*1024*1024,'Packet JSON too large');
  console.log(JSON.stringify(makeJob(JSON.parse(await readFile(process.argv[2],'utf8')),process.argv[3]),null,2));
}
