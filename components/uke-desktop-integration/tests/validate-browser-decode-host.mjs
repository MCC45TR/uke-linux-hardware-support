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
assert.throws(()=>compare({...video,sourceSHA256:'c'.repeat(64)},reference,text),/Different encoded input/);
const changed=structuredClone(video);changed.frames[13].sha256='d'.repeat(64);
assert.throws(()=>compare(changed,reference,text),/Visible pixels differ/);
const duplicate=structuredClone(video);duplicate.frames[1].timestamp=0;
assert.throws(()=>compare(duplicate,reference,text),/Repeated browser PTS/);
assert.throws(()=>compare(video,reference,text+'#modified\n'),/framehash file changed/);
console.log('Browser/reference pixel validation: passed (host fixtures only)');
