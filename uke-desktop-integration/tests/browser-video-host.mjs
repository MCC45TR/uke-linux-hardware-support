// Host-only API mocks validate the workload recorder, never a codec or device.
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
import {webcrypto} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

const controller=fileURLToPath(new URL('./capture-browser.mjs',import.meta.url));
for(const [args,message] of [
  [['0'],'Invalid forwarded port'],
  [['1','https://example.invalid'],'Use only a reviewed test page'],
  [['1','file:///var/tmp/uke-browser-video-20261025/browser-video.html'],'Provide a host JSON workload file'],
  [['1','file:///var/tmp/uke-browser-video-20261025/browser-gpu.html','unexpected'],'GPU baseline accepts no video workload']
]) {
  const child=spawnSync(process.execPath,[controller,...args],{encoding:'utf8',timeout:5000});
  assert.equal(child.status,1);
  assert(child.stderr.includes(message),child.stderr);
}

let drop=false,badCopy=false,badVisible=false,padded=false,encoderCreated=0,decoderCreated=0,closedInputs=0,closedOutputs=0;
class InputFrame {
  constructor(data,init) {
    this.timestamp=init.timestamp;
    assert.equal(init.codedWidth,640);assert.equal(init.codedHeight,360);
    assert.equal(data.length,640*360*3/2*(init.format==='I420P10'?2:1));
    if(init.format==='I420P10') {
      const view=new DataView(data.buffer);
      assert.equal(view.getUint16(0,true),64+Math.round(init.timestamp*30/1000000)*7);
      assert.equal(view.getUint16(2,true)%4,(view.getUint16(0,true)+1)%4);
    }
  }
  close() {closedInputs++;}
}
class Encoder {
  static async isConfigSupported(config) {return {supported:true,config:structuredClone(config)};}
  constructor(callbacks) {this.callbacks=callbacks;this.state='unconfigured';encoderCreated++;}
  configure(config) {this.config=config;this.state='configured';}
  encode(frame) {
    if(drop) return;
    this.callbacks.output({type:frame.timestamp===0?'key':'delta',timestamp:frame.timestamp,
      duration:33333,byteLength:4,copyTo:data=>data.set([0,0,1,9])},
      {decoderConfig:{codec:this.config.codec}});
  }
  async flush() {}
  close() {this.state='closed';}
}
class Decoder {
  static async isConfigSupported(config) {return {supported:true,config};}
  constructor(callbacks) {this.callbacks=callbacks;this.state='unconfigured';decoderCreated++;}
  configure(config) {this.config=config;this.state='configured';}
  decode(chunk) {
    if(drop) return;
    const width=this.config.codedWidth,height=this.config.codedHeight;
    let closed=false;
    this.callbacks.output({timestamp:chunk.timestamp,duration:33333,format:'NV12',
      codedWidth:width,codedHeight:Math.ceil(height/16)*16,
      visibleRect:{x:0,y:0,width:badVisible?width-2:width,height},
      colorSpace:{toJSON:()=>({primaries:'bt709'})},allocationSize:()=>padded?336*360+32:width*height*3/2,
      copyTo:async data=>{
        assert(!closed);if(badCopy) throw new Error('opaque frame readback unavailable');
        if(padded) {
          data.fill(0xa5);
          const uv=8+336*240+16;
          for(let row=0;row<240;row++) data.fill(17,8+row*336,8+row*336+320);
          for(let row=0;row<120;row++) data.fill(17,uv+row*336,uv+row*336+320);
          return [{offset:8,stride:336},{offset:uv,stride:336}];
        }
        data.fill(17);return [{offset:0,stride:width},{offset:width*height,stride:width}];
      },
      close:()=>{assert(!closed);closed=true;closedOutputs++;}});
  }
  async flush() {}
  close() {this.state='closed';}
}
const context=vm.createContext({console,setTimeout,clearTimeout,setInterval,clearInterval,performance,isSecureContext:true,
  VideoFrame:InputFrame,VideoEncoder:Encoder,VideoDecoder:Decoder,
  EncodedVideoChunk:class {constructor(init){Object.assign(this,init);}},
  crypto:webcrypto,btoa,atob,Uint8Array,structuredClone});
vm.runInContext(await readFile(new URL('./browser-video.js',import.meta.url),'utf8'),context);
const probe=context.ukeVideoProbe;
const encoded=await probe.encode('avc1.42E01E');
assert.equal(encoded.functionalPassed,true);
assert.equal(encoded.hardwareAccepted,false);
assert.equal(encoded.chunks.length,30);
assert.equal(closedInputs,30);
const input={codec:'avc1.42E01E',width:320,height:240,expectedFrames:30,chunks:encoded.chunks};
const decoded=await probe.decode(input);
assert.equal(decoded.functionalPassed,true);
assert.equal(decoded.hardwareAccepted,false);
assert.equal(decoded.pixelReferenceAccepted,false);
assert.equal(decoded.frames.length,30);
assert.equal(closedOutputs,30);
assert(decoded.frames.every(frame=>frame.sha256.length===64));
const hd=await probe.decode({...input,width:1920,height:1080});
assert.equal(hd.functionalPassed,true);assert.equal(hd.hardwareAccepted,false);
assert.equal(hd.config.codedWidth,1920);assert.equal(hd.config.codedHeight,1080);
assert(hd.frames.every(frame=>frame.codedHeight===1088 && frame.visibleRect.height===1080 &&
  frame.packedByteLength===1920*1080*3/2 && frame.sha256.length===64));
badVisible=true;
const wrongSize=await probe.decode({...input,width:1920,height:1080});
assert.equal(wrongSize.functionalPassed,false);assert.match(wrongSize.error,/visible size/);
badVisible=false;
padded=true;
const paddedFrames=await probe.decode(input);
assert.equal(paddedFrames.functionalPassed,true);
assert(paddedFrames.frames.every(frame=>frame.sha256===decoded.frames[0].sha256 &&
  frame.packedByteLength===320*240*3/2 && frame.byteLength>frame.packedByteLength));
padded=false;
const interop=await probe.decode({...input,interop:true});
assert.equal(interop.functionalPassed,true);
assert.equal(interop.interop.functionalPassed,false);
assert.equal(interop.interop.hardwareAccepted,false);
assert.match(interop.interop.error,/navigator/);
badCopy=true;
const failedReadback=await probe.decode(input);
assert.equal(failedReadback.functionalPassed,false);
assert.equal(failedReadback.hardwareAccepted,false);
assert.equal(failedReadback.frames.length,30);
assert.match(failedReadback.error,/opaque frame/);
const independentInterop=await probe.decode({...input,interop:true});
assert.equal(independentInterop.functionalPassed,false);
assert.match(independentInterop.interop.error,/navigator/);
badCopy=false;
const beforeEncoder=encoderCreated,beforeDecoder=decoderCreated;
assert.match((await probe.encode('av01.0.01M.08')).error,/outside the admitted/);
assert.match((await probe.decode({...input,width:8192})).error,/reviewed/);
assert.match((await probe.decode({...input,codec:'avc1.6e001e'})).error,/outside the admitted/);
assert.match((await probe.decode({...input,codec:'vp09.02.31.12'})).error,/outside the admitted/);
assert.match((await probe.decode({...input,chunks:[{type:'key',timestamp:0,dataBase64:'!'}]})).error,/Invalid/);
assert.equal(encoderCreated,beforeEncoder);
assert.equal(decoderCreated,beforeDecoder);
drop=true;
assert.equal((await probe.encode('hvc1.1.6.L93.B0')).functionalPassed,false);
assert.equal((await probe.decode(input)).functionalPassed,false);
drop=false;
const parallel=await Promise.allSettled([probe.encode('avc1.42E01E'),probe.encode('avc1.42E01E')]);
assert.equal(parallel[0].status,'fulfilled');
assert.equal(parallel[1].status,'rejected');
assert.match(String(parallel[1].reason),/Another workload/);
const main10=await probe.encode('hvc1.2.4.L93.B0');
assert.equal(main10.functionalPassed,true);
assert.equal(main10.bitDepth,10);
assert.equal(main10.sourceFormat,'I420P10');
assert.equal(main10.precisionAccepted,false);
let stoppedTracks=0,closedPeers=0,nextPeer=0,rtcCodec='video/H264';
context.document={createElement:tag=>tag==='canvas'?{
  getContext:()=>({fillRect(){}}),
  captureStream:()=>{
    const track={stop(){stoppedTracks++;}};
    return {getVideoTracks:()=>[track],getTracks:()=>[track]};
  }
}:{}};
context.RTCRtpSender={getCapabilities:()=>({codecs:[{mimeType:'video/H264'},{mimeType:'video/VP8'}]})};
context.RTCPeerConnection=class {
  constructor(options) {assert.equal(options.iceServers.length,0);this.sender=nextPeer++%2===0;}
  addTransceiver() {return {setCodecPreferences:codecs=>assert(codecs.every(c=>c.mimeType==='video/H264'))};}
  async createOffer() {return {};}
  async createAnswer() {return {};}
  async setLocalDescription(description) {this.localDescription=description;}
  async setRemoteDescription(description) {this.remoteDescription=description;}
  async getStats() {
    return this.sender?new Map([
      ['out',{type:'outbound-rtp',kind:'video',codecId:'codec',framesEncoded:4,
        encoderImplementation:'host mock',powerEfficientEncoder:true}],
      ['codec',{mimeType:rtcCodec}]
    ]):new Map([['in',{type:'inbound-rtp',kind:'video',framesDecoded:4}]]);
  }
  close() {closedPeers++;}
};
const rtc=await probe.webRtc();
assert.equal(rtc.functionalPassed,true);
assert.equal(rtc.hardwareAccepted,false);
assert.equal(stoppedTracks,1);
assert.equal(closedPeers,2);
rtcCodec='video/VP8';
assert.equal((await probe.webRtc()).functionalPassed,false);
assert.equal(closedPeers,4);
context.RTCPeerConnection=class {constructor(){throw new Error('mock RTC failure');}};
assert.match((await probe.webRtc()).error,/mock RTC failure/);
assert.equal(stoppedTracks,3);
console.log('Browser video recorder validation/cleanup: passed (host mocks only)');
