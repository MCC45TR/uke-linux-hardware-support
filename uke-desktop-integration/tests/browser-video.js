// Scoped smoke workloads. API preference and RTC statistics never establish VPU acceptance.
'use strict';
(() => {
  // Chrome 156 raises the VA encode minimum to 321x241 for performance.
  const width=640, height=360, count=30;
  const encoders=new Set(['avc1.42E01E','avc1.4D001E','avc1.64001E','hvc1.1.6.L93.B0','hvc1.2.4.L93.B0']);
  let busy=false;
  const check=(condition,message)=>{if(!condition) throw new Error(message);};
  const base64=bytes=>{
    let value='';
    for(let i=0;i<bytes.length;i+=16384) value+=String.fromCharCode(...bytes.subarray(i,i+16384));
    return btoa(value);
  };
  const bytes=value=>Uint8Array.from(atob(value),c=>c.charCodeAt(0));
  async function bounded(promise) {
    let timer;
    try {return await Promise.race([promise,new Promise((_,reject)=>{
      timer=setTimeout(()=>reject(new Error('Workload timed out')),15000);
    })]);} finally {clearTimeout(timer);}
  }
  async function run(operation,work) {
    check(!busy,'Another workload is active');
    busy=true;
    const result={operation,startedUTC:new Date().toISOString(),hardwareAccepted:false,
      evidenceRequired:'Exact Chrome decoder/encoder plus Iris session/output trace; independent pixel/bitstream validation',
      functionalPassed:false};
    const start=performance.now();
    try {
      check(globalThis.isSecureContext,'A secure context is required');
      Object.assign(result,await work(result));
    } catch(error) {result.error=String(error);}
    finally {result.elapsedMs=performance.now()-start;busy=false;}
    globalThis.videoProbeResult=result;
    return result;
  }
  function serialConfig(config) {
    if(!config) return null;
    const out={...config};
    if(config.description) out.descriptionBase64=base64(new Uint8Array(config.description));
    delete out.description;
    return out;
  }
  function visibleBytes(format,w,h,layout,data) {
    let planes;
    if(format==='NV12') planes=[[w,h],[Math.ceil(w/2)*2,Math.ceil(h/2)]];
    else if(format==='I420' || format==='I420P10') {
      const bytes=format==='I420P10'?2:1;
      planes=[[w*bytes,h],[Math.ceil(w/2)*bytes,Math.ceil(h/2)],[Math.ceil(w/2)*bytes,Math.ceil(h/2)]];
    } else if(['RGBA','RGBX','BGRA','BGRX'].includes(format)) planes=[[w*4,h]];
    else throw new Error('No canonical pixel layout for '+format);
    check(layout.length===planes.length,'Unexpected copied plane count');
    const packed=new Uint8Array(planes.reduce((size,[row,rows])=>size+row*rows,0));
    let output=0;
    planes.forEach(([rowBytes,rows],index)=>{
      const {offset,stride}=layout[index];
      check(Number.isSafeInteger(offset) && offset>=0 && Number.isSafeInteger(stride) &&
        stride>=rowBytes && offset+(rows-1)*stride+rowBytes<=data.length,'Invalid copied plane layout');
      for(let row=0;row<rows;row++) {
        packed.set(data.subarray(offset+row*stride,offset+row*stride+rowBytes),output);output+=rowBytes;
      }
    });
    return packed;
  }
  async function videoInterop(frame) {
    const width=frame.visibleRect.width,height=frame.visibleRect.height;
    check(navigator.gpu,'WebGPU is unavailable');
    const adapter=await navigator.gpu.requestAdapter({powerPreference:'high-performance'});
    check(adapter && !adapter.info.isFallbackAdapter,'No non-fallback WebGPU adapter');
    const device=await adapter.requestDevice();
    let texture,readback;
    try {
      check(frame.visibleRect.width===width && frame.visibleRect.height===height,
        'Unexpected visible dimensions for interop');
      const reference=new Uint8Array(width*height*4);
      const layout=await bounded(frame.copyTo(reference,{format:'RGBA',colorSpace:'srgb'}));
      const shader=device.createShaderModule({code:[
        'struct Out { @builtin(position) pos: vec4f, @location(0) uv: vec2f };',
        '@vertex fn vs(@builtin(vertex_index) i:u32) -> Out {',
        'let p=array<vec2f,3>(vec2f(-1,-1),vec2f(3,-1),vec2f(-1,3));',
        'var o:Out; o.pos=vec4f(p[i],0,1); o.uv=p[i]*vec2f(0.5,-0.5)+vec2f(0.5); return o; }',
        '@group(0) @binding(0) var s:sampler;',
        '@group(0) @binding(1) var v:texture_external;',
        '@fragment fn fs(i:Out)->@location(0) vec4f { return textureSampleBaseClampToEdge(v,s,i.uv); }'
      ].join('\n')});
      device.pushErrorScope('validation');
      const pipeline=device.createRenderPipeline({layout:'auto',vertex:{module:shader,entryPoint:'vs'},
        fragment:{module:shader,entryPoint:'fs',targets:[{format:'rgba8unorm'}]}});
      texture=device.createTexture({size:[width,height],format:'rgba8unorm',
        usage:GPUTextureUsage.RENDER_ATTACHMENT|GPUTextureUsage.COPY_SRC});
      readback=device.createBuffer({size:width*height*4,usage:GPUBufferUsage.COPY_DST|GPUBufferUsage.MAP_READ});
      const binding=device.createBindGroup({layout:pipeline.getBindGroupLayout(0),entries:[
        {binding:0,resource:device.createSampler({magFilter:'nearest',minFilter:'nearest'})},
        {binding:1,resource:device.importExternalTexture({source:frame,colorSpace:'srgb'})}
      ]});
      const commands=device.createCommandEncoder();
      const pass=commands.beginRenderPass({colorAttachments:[{view:texture.createView(),
        clearValue:{r:0,g:0,b:0,a:1},loadOp:'clear',storeOp:'store'}]});
      pass.setPipeline(pipeline); pass.setBindGroup(0,binding); pass.draw(3); pass.end();
      commands.copyTextureToBuffer({texture},{buffer:readback,bytesPerRow:width*4},[width,height]);
      device.queue.submit([commands.finish()]);
      await bounded(readback.mapAsync(GPUMapMode.READ));
      const actual=new Uint8Array(readback.getMappedRange());
      const validation=await device.popErrorScope();
      check(!validation,validation?.message);
      let maxError=0,different=0;
      for(let row=0;row<height;row++) for(let column=0;column<width*4;column++) {
        const error=Math.abs(actual[row*width*4+column]-reference[layout[0].offset+row*layout[0].stride+column]);
        maxError=Math.max(maxError,error);
        if(error>2) different++;
      }
      const info=adapter.info;
      return {functionalPassed:different===0,adapter:{vendor:info.vendor,architecture:info.architecture,
        device:info.device,description:info.description,isFallbackAdapter:info.isFallbackAdapter},
        comparedBytes:width*height*4,maxChannelError:maxError,bytesOutsideTolerance:different,
        tolerance:2,hardwareAccepted:false,zeroCopyAccepted:false,
        note:'Decoded VideoFrame external-texture RGB comparison; no DMA-BUF/modifier/fence or HDR precision claim'};
    } finally {
      if(readback?.mapState==='mapped') readback.unmap();
      readback?.destroy(); texture?.destroy(); device.destroy();
    }
  }
  async function encode(codec) {
    return run('webcodecs-encode',async result=>{
      check(encoders.has(codec),'Codec is outside the admitted encode smoke set');
      const tenBit=codec==='hvc1.2.4.L93.B0',format=tenBit?'I420P10':'NV12';
      const config={codec,width,height,framerate:30,bitrate:tenBit?4000000:1000000,bitrateMode:'variable',
        latencyMode:'quality',hardwareAcceleration:'prefer-hardware'};
      const extension=codec.startsWith('avc1')?'avc':'hevc';
      config[extension]={format:'annexb'};
      const support=await VideoEncoder.isConfigSupported(config);
      check(support.supported,'Encoder configuration is unsupported');
      check(support.config[extension]?.format==='annexb','Annex-B option was not recognized');
      const chunks=[],errors=[],submitted=new Map();
      result.config=serialConfig(support.config);result.chunks=chunks;result.encoderErrors=errors;
      const start=performance.now();
      const encoder=new VideoEncoder({
        output:(chunk,metadata)=>{
          const data=new Uint8Array(chunk.byteLength); chunk.copyTo(data);
          chunks.push({type:chunk.type,timestamp:chunk.timestamp,duration:chunk.duration,
            outputLatencyMs:performance.now()-submitted.get(chunk.timestamp),
            dataBase64:base64(data),decoderConfig:serialConfig(metadata.decoderConfig)});
        },error:error=>errors.push(String(error))
      });
      try {
        encoder.configure(support.config);
        for(let n=0;n<count;n++) {
          const data=new Uint8Array(width*height*3/2*(tenBit?2:1));
          if(tenBit) {
            const words=new DataView(data.buffer);
            for(let i=0;i<data.length;i+=2) words.setUint16(i,512,true);
            for(let y=0;y<height;y++) for(let x=0;x<width;x++)
              words.setUint16((y*width+x)*2,64+((x+3*y+n*7)%877),true);
          } else {
            data.fill(128);
            for(let y=0;y<height;y++) for(let x=0;x<width;x++)
              data[y*width+x]=32+((x+y+n*7)%192);
          }
          const frame=new VideoFrame(data,{format,codedWidth:width,codedHeight:height,
            timestamp:Math.round(n*1000000/30),
            colorSpace:{primaries:'bt709',transfer:'bt709',matrix:'bt709',fullRange:false}});
          submitted.set(frame.timestamp,performance.now());
          try {encoder.encode(frame,{keyFrame:n===0});} finally {frame.close();}
        }
        await bounded(encoder.flush());
        check(!errors.length,errors.join('; '));
        const timestamps=new Set(chunks.map(chunk=>chunk.timestamp));
        const expected=new Set(Array.from({length:count},(_,n)=>Math.round(n*1000000/30)));
        check(chunks.length===count && timestamps.size===count &&
          chunks.every(chunk=>chunk.dataBase64.length>0 && expected.has(chunk.timestamp)) && chunks[0].type==='key',
          'Encoder did not produce every input frame exactly once');
        return {config:serialConfig(support.config),submittedFrames:count,chunks,
          workloadElapsedMs:performance.now()-start,
          functionalPassed:true,bitDepth:tenBit?10:8,sourceFormat:format,precisionAccepted:false,
          sourcePath:'CPU frame upload; not a zero-copy workload'};
      } finally {if(encoder.state!=='closed') encoder.close();}
    });
  }
  async function decode(input) {
    return run('webcodecs-decode',async result=>{
      check(input && /^(avc1\.(?:42|4[dD]|64)[0-9a-fA-F]{4}|(?:hvc1|hev1)\.[12]\.[A-Za-z0-9.]+|vp09\.(?:00\.\d{2}\.08|02\.\d{2}\.10)(?:\.\d{2})*)$/.test(input.codec),
        'Codec is outside the admitted decode smoke set');
      const hd=input.width===1920 && input.height===1080;
      const uhd=input.width===3840 && input.height===2160,large=hd || uhd,expectedFrames=uhd?60:30;
      check((large || (input.width===320 && input.height===240)) && input.expectedFrames===expectedFrames,
        'Use a reviewed SD/HD 30-frame or UHD 60-frame corpus');
      check(Array.isArray(input.chunks) && input.chunks.length>0 && input.chunks.length<=2*expectedFrames,
        'Invalid chunk count');
      let total=0;
      const chunks=input.chunks.map(chunk=>{
        check(['key','delta'].includes(chunk.type) && Number.isSafeInteger(chunk.timestamp) &&
          typeof chunk.dataBase64==='string' && chunk.dataBase64.length<=(large?6*1024*1024:1500000),'Invalid encoded chunk');
        const data=bytes(chunk.dataBase64); total+=data.length;
        check(data.length>0 && total<=(large?32:8)*1024*1024,'Encoded corpus exceeds the smoke limit');
        return new EncodedVideoChunk({type:chunk.type,timestamp:chunk.timestamp,data});
      });
      check(input.chunks[0].type==='key','The first chunk must be a key frame');
      const config={codec:input.codec,codedWidth:input.width,codedHeight:input.height,
        hardwareAcceleration:'prefer-hardware'};
      if(input.descriptionBase64) {
        check(typeof input.descriptionBase64==='string' && input.descriptionBase64.length<=1500000,
          'Decoder description exceeds the smoke limit');
        config.description=bytes(input.descriptionBase64);
      }
      const support=await VideoDecoder.isConfigSupported(config);
      check(support.supported,'Decoder configuration is unsupported');
      const frames=[],pending=[],errors=[];
      result.config=serialConfig(support.config);result.frames=frames;result.decoderErrors=errors;
      result.sourceSHA256=input.sourceSHA256??null;
      let interop=null;
      const start=performance.now(),submitted=new Map();
      const decoder=new VideoDecoder({
        output:frame=>{
          const index=pending.length;
          const outputLatencyMs=performance.now()-submitted.get(frame.timestamp);
          pending.push((async()=>{
            let packed;
            try {
              frames[index]={timestamp:frame.timestamp,duration:frame.duration,format:frame.format,
                outputLatencyMs,
                codedWidth:frame.codedWidth,codedHeight:frame.codedHeight,
                visibleRect:{x:frame.visibleRect.x,y:frame.visibleRect.y,
                  width:frame.visibleRect.width,height:frame.visibleRect.height},
                colorSpace:frame.colorSpace.toJSON()};
              check(frame.visibleRect.width===input.width && frame.visibleRect.height===input.height,
                'Decoder visible size changed');
              if(input.interop===true && index===0) {
                try {interop=await videoInterop(frame);}
                catch(error) {interop={functionalPassed:false,hardwareAccepted:false,error:String(error)};}
                result.interop=interop;
              }
              const size=frame.allocationSize();
              check(Number.isSafeInteger(size) && size>0 && size<=64*1024*1024,
                'Decoded frame allocation exceeds the smoke limit');
              const data=new Uint8Array(size);
              const layout=await bounded(frame.copyTo(data));
              packed=visibleBytes(frame.format,frame.visibleRect.width,frame.visibleRect.height,layout,data);
              Object.assign(frames[index],{layout,byteLength:data.length,
                packedByteLength:packed.length,hashScope:'Packed visible planes in the declared frame format'});
            } finally {frame.close();}
            // Release the decoder surface before CPU hashing the copied pixels.
            const digest=await crypto.subtle.digest('SHA-256',packed);
            frames[index].sha256=Array.from(new Uint8Array(digest),v=>v.toString(16).padStart(2,'0')).join('');
          })().catch(error=>errors.push(String(error))));
        },error:error=>errors.push(String(error))
      });
      try {
        decoder.configure(support.config);
        chunks.forEach(chunk=>{
          submitted.set(chunk.timestamp,performance.now());decoder.decode(chunk);
        });
        await bounded(decoder.flush());
        await bounded(Promise.all(pending));
        check(!errors.length,errors.join('; '));
        check(frames.length===expectedFrames && frames.every(Boolean) &&
          new Set(frames.map(frame=>frame.timestamp)).size===expectedFrames &&
          frames.every(frame=>chunks.some(chunk=>chunk.timestamp===frame.timestamp)),
          'Decoder did not produce every frame exactly once');
        return {config:serialConfig(support.config),frames,functionalPassed:true,interop,
          workloadElapsedMs:performance.now()-start,
          pixelReferenceAccepted:false,precisionAccepted:false,
          note:'Native format/layout hashes retained; independent same-format references are still required'};
      } finally {
        if(decoder.state!=='closed') decoder.close();
        await Promise.all(pending);
      }
    });
  }
  async function webRtc(mimeType='video/H264') {
    return run('webrtc-loopback',async()=>{
      check(['video/H264','video/H265'].includes(mimeType),'WebRTC smoke supports only H264/H265');
      const canvas=document.createElement('canvas'); canvas.width=width; canvas.height=height;
      const context=canvas.getContext('2d'), stream=canvas.captureStream(30);
      let sender,receiver,remote,timer,iceError=null;
      const pendingIce={sender:[],receiver:[]};
      try {
      sender=new RTCPeerConnection({iceServers:[]}); receiver=new RTCPeerConnection({iceServers:[]});
      remote=document.createElement('video'); remote.muted=true; remote.autoplay=true;
      sender.onicecandidate=event=>{if(event.candidate) {
        if(receiver.remoteDescription) receiver.addIceCandidate(event.candidate).catch(e=>{iceError=String(e);});
        else pendingIce.receiver.push(event.candidate);
      }};
      receiver.onicecandidate=event=>{if(event.candidate) {
        if(sender.remoteDescription) sender.addIceCandidate(event.candidate).catch(e=>{iceError=String(e);});
        else pendingIce.sender.push(event.candidate);
      }};
      receiver.ontrack=event=>{remote.srcObject=new MediaStream([event.track]);};
        const track=stream.getVideoTracks()[0];
        const transceiver=sender.addTransceiver(track,{direction:'sendonly',streams:[stream]});
        const codecs=RTCRtpSender.getCapabilities('video').codecs
          .filter(codec=>codec.mimeType.toLowerCase()===mimeType.toLowerCase());
        check(codecs.length>0,'No matching WebRTC capability');
        transceiver.setCodecPreferences(codecs);
        let frame=0;
        timer=setInterval(()=>{
          context.fillStyle='rgb('+((frame++*7)%192+32)+',96,160)';
          context.fillRect(0,0,width,height);
        },33);
        await bounded((async()=>{
        await sender.setLocalDescription(await sender.createOffer());
        await receiver.setRemoteDescription(sender.localDescription);
        await Promise.all(pendingIce.receiver.map(candidate=>receiver.addIceCandidate(candidate)));
        await receiver.setLocalDescription(await receiver.createAnswer());
        await sender.setRemoteDescription(receiver.localDescription);
        await Promise.all(pendingIce.sender.map(candidate=>sender.addIceCandidate(candidate)));
        })());
        const start=performance.now();
        let outbound,inbound,codec;
        do {
          await new Promise(resolve=>setTimeout(resolve,250));
          const sent=await sender.getStats(), received=await receiver.getStats();
          outbound=[...sent.values()].find(value=>value.type==='outbound-rtp' && value.kind==='video');
          inbound=[...received.values()].find(value=>value.type==='inbound-rtp' && value.kind==='video');
          codec=outbound && sent.get(outbound.codecId);
        } while((!outbound?.framesEncoded || !inbound?.framesDecoded) && performance.now()-start<10000);
        check(!iceError,iceError);
        check(codec?.mimeType.toLowerCase()===mimeType.toLowerCase() && outbound?.framesEncoded>0 &&
          inbound?.framesDecoded>0,'No selected-codec encode/decode loopback output');
        return {codec:{mimeType:codec.mimeType,sdpFmtpLine:codec.sdpFmtpLine},
          framesEncoded:outbound.framesEncoded,framesDecoded:inbound.framesDecoded,
          encoderImplementation:outbound.encoderImplementation??null,
          decoderImplementation:inbound.decoderImplementation??null,
          powerEfficientEncoder:outbound.powerEfficientEncoder??null,
          totalEncodeTime:outbound.totalEncodeTime??null,functionalPassed:true,
          note:'Canvas capture/local loopback; no microphone, camera or external signaling'};
      } finally {
        clearInterval(timer); sender?.close(); receiver?.close();
        stream.getTracks().forEach(track=>track.stop());
        if(remote?.srcObject) remote.srcObject.getTracks().forEach(track=>track.stop());
        if(remote) remote.srcObject=null;
      }
    });
  }
  globalThis.ukeVideoProbe={encode,decode,webRtc};
})();
