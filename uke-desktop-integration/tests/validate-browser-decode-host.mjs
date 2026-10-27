import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {compare} from './validate-browser-decode.mjs';
const digest='b'.repeat(64),sourceSHA256='a'.repeat(64);
const text='#tb 0: 1/30\n#hash: SHA256\n'+Array.from({length:30},(_,n)=>
  ['0',n,n,1,115200,digest].join(', ')).join('\n')+'\n';
const reference={sourceSHA256,format:'NV12',width:320,height:240,
  framehashSHA256:createHash('sha256').update(text).digest('hex')};
const video={functionalPassed:true,sourceSHA256,frames:Array.from({length:30},(_,n)=>({
  format:'NV12',visibleRect:{width:320,height:240},timestamp:Math.round(n*1000000/30),
  packedByteLength:115200,sha256:digest}))};
assert.equal(compare(video,reference,text).pixelReferencePassed,true);
assert.equal(compare(video,reference,text).hardwareAccepted,false);
const uhdText='#tb 0: 1/1000\n#hash: SHA256\n'+Array.from({length:60},(_,n)=>
  ['0',Math.round(n*1000/60),Math.round(n*1000/60),17,12441600,digest].join(', ')).join('\n')+'\n';
const uhdReference={...reference,width:3840,height:2160,frameCount:60,
  framehashSHA256:createHash('sha256').update(uhdText).digest('hex')};
const uhdVideo={...video,frames:Array.from({length:60},(_,n)=>({format:'NV12',
  visibleRect:{width:3840,height:2160},timestamp:Math.round(n*1000/60)*1000,
  packedByteLength:12441600,sha256:digest}))};
assert.equal(compare(uhdVideo,uhdReference,uhdText).framesChecked,60);
assert.equal(compare(uhdVideo,uhdReference,uhdText).hardwareAccepted,false);
assert.throws(()=>compare(uhdVideo,{...uhdReference,frameCount:31},uhdText),/Unreviewed reference frame count/);
assert.throws(()=>compare({...uhdVideo,frames:uhdVideo.frames.slice(1)},uhdReference,uhdText),/browser frames/);
assert.throws(()=>compare({...video,sourceSHA256:'c'.repeat(64)},reference,text),/Different encoded input/);
const changed=structuredClone(video);changed.frames[13].sha256='d'.repeat(64);
assert.throws(()=>compare(changed,reference,text),/Visible pixels differ/);
const duplicate=structuredClone(video);duplicate.frames[1].timestamp=0;
assert.throws(()=>compare(duplicate,reference,text),/Repeated browser PTS/);
const repeatedText=text.replace(/^0, 1, 1, 1,/m,'0, 1, 0, 1,');
const repeatedReference={...reference,framehashSHA256:createHash('sha256').update(repeatedText).digest('hex')};
assert.throws(()=>compare(duplicate,repeatedReference,repeatedText),/repeated reference PTS/);
assert.throws(()=>compare(video,reference,text+'#modified\n'),/framehash file changed/);
console.log('Browser/reference pixel validation: passed (host fixtures only)');
