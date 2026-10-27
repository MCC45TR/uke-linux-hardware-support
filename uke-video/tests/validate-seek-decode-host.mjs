// Synthetic host fixtures; these are not codec or device results.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {compareSeek} from './validate-seek-decode.mjs';
const sha=x=>createHash('sha256').update(x).digest('hex');
const input=Buffer.from('explicit host fixture, not a compressed stream');
const header='index\tpts\tduration\twidth\theight\tformat\tdepth\tbytes\tsha256\tprimaries\ttransfer\tmatrix\trange';
function fixture(native,format,depth,count=30) {
  const bytes=320*240*3/2*(depth===10?2:1);
  const hashes=Array.from({length:count},(_,n)=>sha(native+':'+n));
  const text='#tb 0: 1/30\n#hash: SHA256\n#dimensions 0: 320x240\n'+hashes.map((h,n)=>`0, ${n}, ${n}, 1, ${bytes}, ${h}`).join('\n')+'\n';
  const reference={sourceSHA256:sha(input),format,width:320,height:240,frameCount:count,
    framehashSHA256:sha(text),color:[2,2,2,0]};
  const result={functionalPassed:true,sameAVCodecContext:true,heldFrameUnchanged:true,
    decoder:'rawvideo',softwareControl:true,sourceSHA256:sha(input),framesPerPass:count};
  const tsv=`# source_sha256 ${sha(input)}\n# decoder rawvideo\n# time_base 1/30\n${header}\n`+
    hashes.map((h,n)=>[n,n,1,320,240,native,depth,bytes,h,2,2,2,0].join('\t')).join('\n')+'\n';
  return {result,reference,text,tsv};
}
function change(tsv,row,column,value) {
  const lines=tsv.trimEnd().split('\n'),fields=lines[row+4].split('\t');
  fields[column]=String(value);lines[row+4]=fields.join('\t');return lines.join('\n')+'\n';
}
const f=fixture('nv12','NV12',8);
const check=(a=f.tsv,b=f.tsv,r=f.reference,t=f.text,result=f.result,data=input)=>
  compareSeek(data,result,a,b,r,t);
for(const [native,format,depth] of [['nv12','NV12',8],['p010le','P010',10],
  ['yuv420p','I420',8],['yuv420p10le','I420P10',10]]) {
  for(const count of [30,60]) {
    const g=fixture(native,format,depth,count);
    const pass=compareSeek(input,g.result,g.tsv,g.tsv,g.reference,g.text);
    assert.equal(pass.pixelReferencePassed,true);assert.equal(pass.hardwareAccepted,false);
    assert.equal(pass.lifecycleAccepted,false);assert.equal(pass.passesChecked,2);
    assert.equal(pass.framesPerPass,count);assert.equal(pass.colorReferenceChecked,true);
  }
}
assert.equal(check(undefined,undefined,{...f.reference,color:undefined}).colorReferenceChecked,false);
assert.equal(check(undefined,undefined,undefined,undefined,{...f.result,hardwareAccepted:true}).hardwareAccepted,false);
const repeatedText=f.text.replace(/^0, 1, 1, 1,/m,'0, 1, 0, 1,');
const repeatedTSV=change(f.tsv,1,1,0);
const repeatedRef={...f.reference,framehashSHA256:sha(repeatedText)};
assert.equal(check(repeatedTSV,repeatedTSV,repeatedRef,repeatedText).pixelReferencePassed,true);
assert.throws(()=>check(repeatedTSV,change(repeatedTSV,1,8,'e'.repeat(64)),repeatedRef,repeatedText),
  /Visible pixels differ/);
assert.throws(()=>check(undefined,change(f.tsv,7,8,'d'.repeat(64))),/Visible pixels differ/);
const wrong=change(f.tsv,7,8,'d'.repeat(64));
assert.throws(()=>check(wrong,wrong),/Visible pixels differ/);
assert.throws(()=>check(undefined,undefined,undefined,f.text+'# modified\n'),/framehash file changed/);
assert.throws(()=>check(undefined,undefined,undefined,undefined,undefined,Buffer.from('different input')),/input hash differs/);
assert.throws(()=>check(f.tsv.replace(sha(input),'a'.repeat(64))),/native source identity/);
assert.throws(()=>check(f.tsv.replace('# decoder rawvideo','# decoder h264_v4l2m2m')),/native decoder/);
assert.throws(()=>check(change(f.tsv,1,0,0)),/order\/index/);
assert.throws(()=>check(change(f.tsv,1,1,0)),/PTS differs/);
assert.throws(()=>check(undefined,change(f.tsv,2,2,2)),/duration differs/);
assert.throws(()=>check(undefined,change(f.tsv,2,2,0)),/Missing native frame duration/);
assert.throws(()=>check(change(f.tsv,1,4,241)),/visible height/);
assert.throws(()=>check(undefined,undefined,{...f.reference,width:240,height:320}),/Reference width differs/);
assert.throws(()=>check(change(f.tsv,1,7,1)),/visible plane size/);
assert.throws(()=>check(change(f.tsv,1,6,10)),/bit depth differs/);
assert.throws(()=>check(change(f.tsv,1,5,'cuda')),/Unreviewed native pixel format/);
assert.throws(()=>check(change(f.tsv,1,9,9)),/Independent color metadata differs/);
assert.throws(()=>check(undefined,change(f.tsv,1,9,9),{...f.reference,color:undefined}),/metadata/);
assert.throws(()=>check(f.tsv.replace('1/30','0/30')),/Invalid native time base/);
assert.throws(()=>check(f.tsv.replace(header,header+'\tunknown')),/columns/);
assert.throws(()=>check(f.tsv.split('\n').slice(0,-2).join('\n')),/frame count/);
assert.throws(()=>check(undefined,undefined,undefined,undefined,{...f.result,heldFrameUnchanged:false}),/did not complete/);
const unreviewed=fixture('nv12','NV12',8,31);
assert.throws(()=>compareSeek(input,unreviewed.result,unreviewed.tsv,unreviewed.tsv,unreviewed.reference,unreviewed.text),/Unreviewed reference frame count/);
let fractional=f.tsv.replace('# time_base 1/30','# time_base 1/30000000');
for(let n=0;n<30;n++) {fractional=change(fractional,n,1,n*1000000+1);fractional=change(fractional,n,2,1000000);}
assert.throws(()=>check(fractional,fractional),/Exact native\/reference PTS differs/);
console.log('PASS: both native seek passes bind to source, independent pixels, exact timing and optional color; synthetic host fixtures only.');
