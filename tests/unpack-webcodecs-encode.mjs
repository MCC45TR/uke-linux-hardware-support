// Host-only extraction; an independent decoder must qualify the saved bitstream.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {readFile,writeFile} from 'node:fs/promises';
import {pathToFileURL} from 'node:url';

const codecs=new Set(['avc1.42E01E','avc1.4D001E','avc1.64001E','hvc1.1.6.L93.B0','hvc1.2.4.L93.B0']);
export function unpack(video) {
  assert(video?.operation==='webcodecs-encode' && video.functionalPassed,'Encoder workload did not pass');
  const config=video.config;
  assert(codecs.has(config?.codec),'Unreviewed encoder codec');
  assert(config.width===640 && config.height===360 && config.framerate===30,'Unreviewed encoder dimensions/rate');
  const extension=config.codec.startsWith('avc1')?'avc':'hevc';
  assert.equal(config[extension]?.format,'annexb','Length-prefixed output is not an Annex-B stream');
  assert.equal(video.submittedFrames,30,'Expected thirty inputs');
  assert(Array.isArray(video.encoderErrors) && video.encoderErrors.length===0,'Encoder reported errors');
  assert.equal(video.chunks?.length,30,'Expected thirty coded outputs');
  const expected=new Set(Array.from({length:30},(_,n)=>Math.round(n*1000000/30)));
  const seen=new Set(),buffers=[],packets=[];
  let size=0;
  for(const chunk of video.chunks) {
    assert(['key','delta'].includes(chunk.type),'Invalid chunk type');
    assert(Number.isSafeInteger(chunk.timestamp) && expected.has(chunk.timestamp) && !seen.has(chunk.timestamp),
      'Invalid/repeated encoder PTS');
    seen.add(chunk.timestamp);
    assert(typeof chunk.dataBase64==='string' && chunk.dataBase64.length>0 && chunk.dataBase64.length<=1500000,
      'Invalid chunk size');
    const data=Buffer.from(chunk.dataBase64,'base64');
    assert.equal(data.toString('base64'),chunk.dataBase64,'Invalid base64');
    assert(data.length>=4 && data[0]===0 && data[1]===0 &&
      (data[2]===1 || (data[2]===0 && data[3]===1)),'Missing Annex-B start code');
    size+=data.length;
    assert(size<=8*1024*1024,'Encoded output exceeds the smoke limit');
    packets.push({type:chunk.type,timestamp:chunk.timestamp,offset:size-data.length,byteLength:data.length});
    buffers.push(data);
  }
  assert(video.chunks[0].type==='key' && video.chunks[0].timestamp===0,'Missing first key frame');
  const bitstream=Buffer.concat(buffers);
  return {bitstream,receipt:{extractionPassed:true,codec:config.codec,demuxer:extension==='avc'?'h264':'hevc',
    width:640,height:360,bitDepth:config.codec==='hvc1.2.4.L93.B0'?10:8,
    frames:30,byteLength:bitstream.length,bitstreamSHA256:createHash('sha256').update(bitstream).digest('hex'),
    packets,hardwareAccepted:false,independentDecodePassed:false,precisionAccepted:false,
    note:'Chunk order retained, including reordered PTS. Decode/inspect independently; extraction is not codec or VPU acceptance.'}};
}
if(process.argv[1] && import.meta.url===pathToFileURL(process.argv[1]).href) {
  try {
    assert(process.argv.length===4,'Usage: unpack-webcodecs-encode.mjs CAPTURE.json NEW_OUTPUT');
    const capture=JSON.parse(await readFile(process.argv[2],'utf8'));
    const {bitstream,receipt}=unpack(capture.video);
    await writeFile(process.argv[3],bitstream,{flag:'wx',mode:0o600});
    console.log(JSON.stringify(receipt,null,2));
  } catch(error) {
    console.log(JSON.stringify({extractionPassed:false,hardwareAccepted:false,error:String(error)},null,2));
    process.exitCode=1;
  }
}
