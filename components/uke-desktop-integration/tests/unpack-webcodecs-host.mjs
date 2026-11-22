import assert from 'node:assert/strict';
import {unpack} from './unpack-webcodecs-encode.mjs';
const video={operation:'webcodecs-encode',functionalPassed:true,submittedFrames:30,encoderErrors:[],
  config:{codec:'avc1.42E01E',width:640,height:360,framerate:30,avc:{format:'annexb'}},
  chunks:Array.from({length:30},(_,n)=>({type:n===0?'key':'delta',timestamp:Math.round(n*1000000/30),
    dataBase64:Buffer.from([0,0,0,1,9,n]).toString('base64')}))};
// Coded-order fixture: extraction must preserve bytes rather than sort by presentation time.
[video.chunks[1],video.chunks[2]]=[video.chunks[2],video.chunks[1]];
const {bitstream,receipt}=unpack(video);
assert.equal(bitstream.length,180);
assert.equal(bitstream[11],2);
assert.equal(receipt.packets[1].timestamp,66667);
assert.equal(receipt.hardwareAccepted,false);
assert.equal(receipt.independentDecodePassed,false);
assert.equal(receipt.bitstreamSHA256.length,64);
const duplicate=structuredClone(video);duplicate.chunks[1].timestamp=0;
assert.throws(()=>unpack(duplicate),/repeated encoder PTS/);
const truncated=structuredClone(video);truncated.chunks.pop();
assert.throws(()=>unpack(truncated),/thirty coded/);
const lengthPrefixed=structuredClone(video);lengthPrefixed.config.avc.format='avc';
assert.throws(()=>unpack(lengthPrefixed),/Length-prefixed/);
const badBase64=structuredClone(video);badBase64.chunks[0].dataBase64+='!';
assert.throws(()=>unpack(badBase64),/Invalid base64/);
const noStartCode=structuredClone(video);noStartCode.chunks[0].dataBase64=Buffer.from([1,2,3,4]).toString('base64');
assert.throws(()=>unpack(noStartCode),/Missing Annex-B/);
const tenBit=structuredClone(video);
tenBit.config={...tenBit.config,codec:'hvc1.2.4.L93.B0',hevc:{format:'annexb'}};
assert.equal(unpack(tenBit).receipt.bitDepth,10);
console.log('WebCodecs Annex-B extraction: passed (host byte fixtures, no real encoder)');
