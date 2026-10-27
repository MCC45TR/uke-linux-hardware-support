// Host-only native seek comparison; never deploy Node or this script to Uke.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {readFile,stat} from 'node:fs/promises';
import {basename,dirname,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {compare} from '../../uke-desktop-integration/tests/validate-browser-decode.mjs';

const formats={nv12:['NV12',8],p010le:['P010',10],yuv420p:['I420',8],yuv420p10le:['I420P10',10]};
const decoders=['rawvideo','h264_v4l2m2m','hevc_v4l2m2m','vp9_v4l2m2m'];
const columns='index\tpts\tduration\twidth\theight\tformat\tdepth\tbytes\tsha256\tprimaries\ttransfer\tmatrix\trange';
const digest=data=>createHash('sha256').update(data).digest('hex');
function integer(text) {
  assert(/^-?(0|[1-9]\d*)$/.test(text),'Invalid native integer');
  const value=BigInt(text);
  assert(value>=-(2n**63n) && value<2n**63n,'Native integer exceeds int64');
  return value;
}
function timebase(text) {
  const match=text.match(/^([1-9]\d*)\/([1-9]\d*)$/);
  assert(match,'Invalid native time base');
  const result=match.slice(1).map(integer);
  assert(result.every(n=>n<=2147483647n),'Time base exceeds AVRational');
  return result;
}
function parse(text,result,reference) {
  assert(text.length<=65536,'Native TSV exceeds bound');
  const lines=text.trimEnd().split(/\r?\n/);
  assert.equal(lines.shift(),'# source_sha256 '+result.sourceSHA256,'Different native source identity');
  assert.equal(lines.shift(),'# decoder '+result.decoder,'Different native decoder');
  const header=lines.shift();
  assert(header?.startsWith('# time_base '),'Missing native time base');
  const tb=timebase(header.slice(12));
  assert.equal(lines.shift(),columns,'Unexpected native columns');
  assert.equal(lines.length,reference.frameCount??30,'Unexpected native frame count');
  const frames=lines.map((line,index)=>{
    const f=line.split('\t');assert.equal(f.length,13,'Unexpected native row');
    assert.equal(integer(f[0]),BigInt(index),'Native frame order/index differs');
    const [format,depth]=formats[f[5]]??[];
    assert(format,'Unreviewed native pixel format');
    assert.equal(Number(integer(f[6])),depth,'Native bit depth differs');
    assert(/^[a-f0-9]{64}$/.test(f[8]),'Invalid native frame digest');
    const pts=integer(f[1]),duration=integer(f[2]);
    assert(duration>0n,'Missing native frame duration');
    const width=Number(integer(f[3])),height=Number(integer(f[4])),size=Number(integer(f[7]));
    assert(width>0 && width<=4096 && height>0 && height<=4096 && size>0 && size<=64*1024*1024,
      'Invalid visible frame bounds');
    const color=f.slice(9).map(x=>Number(integer(x)));
    assert(color.every(x=>x>=0 && x<=255),'Invalid native color field');
    if(reference.color) assert.deepEqual(color,reference.color,'Independent color metadata differs');
    const timestamp=Math.round(Number(pts)*Number(tb[0])/Number(tb[1])*1000000);
    assert(Number.isSafeInteger(timestamp),'Unsafe native timestamp');
    return {pts,duration,color,format,visibleRect:{width,height},timestamp,packedByteLength:size,sha256:f[8]};
  });
  return {tb,frames};
}

export function compareSeek(input,result,first,second,reference,framehash) {
  assert(input.length>0 && input.length<=64*1024*1024,'Invalid encoded input size');
  assert.equal(digest(input),reference.sourceSHA256,'Encoded input hash differs');
  assert.equal(result.sourceSHA256,reference.sourceSHA256,'Result source differs');
  assert(result.functionalPassed && result.sameAVCodecContext && result.heldFrameUnchanged,
    'Same-context seek workload did not complete');
  assert(decoders.includes(result.decoder),'Unreviewed native decoder');
  assert.equal(result.softwareControl,result.decoder==='rawvideo','Decoder/control mismatch');
  assert.equal(result.framesPerPass,reference.frameCount??30,'Result frame count differs');
  const passes=[parse(first,result,reference),parse(second,result,reference)];
  assert.equal(digest(framehash),reference.framehashSHA256,'Reference framehash file changed');
  const dimensions=framehash.match(/^#dimensions 0:\s*(\d+)x(\d+)$/m);
  assert(dimensions,'Missing reference dimensions');
  assert.equal(Number(dimensions[1]),reference.width,'Reference width differs from framehash');
  assert.equal(Number(dimensions[2]),reference.height,'Reference height differs from framehash');
  const refTB=framehash.match(/^#tb 0:\s*([1-9]\d*\/[1-9]\d*)$/m);
  assert(refTB,'Missing reference time base');
  const rt=timebase(refTB[1]);
  const rows=framehash.split('\n').filter(l=>l.trim() && !l.startsWith('#')).map(l=>l.split(',').map(x=>x.trim()));
  assert.equal(rows.length,reference.frameCount??30,'Unexpected reference rows');
  for(const pass of passes) {
    for(let index=0;index<pass.frames.length;index++) {
      const frame=pass.frames[index],ref=rows[index];
      assert.equal(ref.length,6,'Unexpected reference row');
      assert.equal(frame.pts*pass.tb[0]*rt[1],integer(ref[2])*rt[0]*pass.tb[1],
        'Exact native/reference PTS differs');
      assert.equal(frame.duration*pass.tb[0]*rt[1],integer(ref[3])*rt[0]*pass.tb[1],
        'Exact native/reference duration differs');
    }
    compare({functionalPassed:true,sourceSHA256:result.sourceSHA256,frames:pass.frames},reference,framehash);
  }
  assert.deepEqual(passes[0].frames,passes[1].frames,'Seek changed native frame metadata');
  return {pixelReferencePassed:true,passesChecked:2,framesPerPass:passes[0].frames.length,
    colorReferenceChecked:Boolean(reference.color),softwareControl:result.softwareControl,
    sourceSHA256:reference.sourceSHA256,hardwareAccepted:false,lifecycleAccepted:false,
    note:'Independent visible-plane/timing comparison; actual backend, teardown and display evidence remain required'};
}
async function bounded(path,limit) {
  const info=await stat(path);assert(info.isFile() && info.size<=limit,'Invalid or oversized input file');
  const data=await readFile(path);assert(data.length<=limit,'Input grew beyond bound');return data;
}
if(process.argv[1] && import.meta.url===pathToFileURL(process.argv[1]).href) {
  try {
    assert.equal(process.argv.length,5,'Usage: INPUT SEEK_RESULT_DIRECTORY REFERENCE_JSON');
    const [inputFile,directory,refFile]=process.argv.slice(2);
    const reference=JSON.parse(await bounded(refFile,32768));
    assert.equal(basename(reference.framehashFile),reference.framehashFile,'Invalid reference path');
    const [input,result,first,second,framehash]=await Promise.all([
      bounded(inputFile,64*1024*1024),bounded(resolve(directory,'result.json'),32768),
      bounded(resolve(directory,'first.tsv'),65536),bounded(resolve(directory,'second.tsv'),65536),
      bounded(resolve(dirname(refFile),reference.framehashFile),65536)]);
    console.log(JSON.stringify(compareSeek(input,JSON.parse(result),first.toString(),second.toString(),reference,framehash.toString()),null,2));
  } catch(error) {
    console.log(JSON.stringify({pixelReferencePassed:false,hardwareAccepted:false,lifecycleAccepted:false,error:String(error)},null,2));
    process.exitCode=1;
  }
}
