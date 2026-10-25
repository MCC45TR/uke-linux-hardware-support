// Host-only CDP capture/workload controller; browser startup/shutdown belong to the trial supervisor.
import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {readFile,stat} from 'node:fs/promises';

const port=Number(process.argv[2]);
const probeURL=process.argv[3];
assert(Number.isInteger(port) && port>0 && port<=65535,'Invalid forwarded port');
const gpuURL='file:///var/tmp/uke-browser-video-20261025/browser-gpu.html';
const videoURL='file:///var/tmp/uke-browser-video-20261025/browser-video.html';
assert([gpuURL,videoURL].includes(probeURL),'Use only a reviewed test page');
let job=null;
if(probeURL===videoURL) {
  assert(process.argv[4],'Provide a host JSON workload file');
  assert((await stat(process.argv[4])).size<=12*1024*1024,'Workload file too large');
  job=JSON.parse(await readFile(process.argv[4],'utf8'));
  assert(['encode','decode','webRtc'].includes(job.operation),'Invalid video workload');
} else assert(!process.argv[4],'GPU baseline accepts no video workload');
const version=await (await fetch(`http://127.0.0.1:${port}/json/version`,
                               {signal:AbortSignal.timeout(10000)})).json();
assert.equal(version.Browser,'Chrome/156.0.8078.47','Unexpected browser version');
const endpoint=new URL(version.webSocketDebuggerUrl);
assert.equal(endpoint.protocol,'ws:');
assert(['localhost','127.0.0.1'].includes(endpoint.hostname),'Non-loopback CDP endpoint');
endpoint.hostname='127.0.0.1'; endpoint.port=String(port);
const socket=new WebSocket(endpoint);
await new Promise((resolve,reject)=>{
  const timer=setTimeout(()=>{socket.close();reject(new Error('CDP connection timed out'));},10000);
  socket.onopen=()=>{clearTimeout(timer);resolve();};
  socket.onerror=error=>{clearTimeout(timer);reject(error);};
});
let nextId=0;
const pending=new Map(), created=[],mediaEvents=[];
socket.onmessage=event=>{
  const value=JSON.parse(event.data), entry=pending.get(value.id);
  if(value.method?.startsWith('Media.')) mediaEvents.push(value);
  if(!entry) return;
  clearTimeout(entry.timer); pending.delete(value.id);
  if(value.error) entry.reject(new Error(JSON.stringify(value.error)));
  else entry.resolve(value.result);
};
socket.onclose=()=>{
  for(const entry of pending.values()) {
    clearTimeout(entry.timer); entry.reject(new Error('CDP connection closed'));
  }
  pending.clear();
};
function call(method,params={},sessionId) {
  const id=++nextId;
  return new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>{pending.delete(id);reject(new Error(`${method} timed out`));},
      job && method==='Runtime.evaluate'?45000:15000);
    pending.set(id,{resolve,reject,timer});
    socket.send(JSON.stringify({id,method,params,...(sessionId?{sessionId}:{})}));
  });
}
async function target(url) {
  const value=await call('Target.createTarget',{url});
  assert(value.targetId && !created.includes(value.targetId));
  created.push(value.targetId);
  return (await call('Target.attachToTarget',{targetId:value.targetId,flatten:true})).sessionId;
}
async function gpuReport() {
  const session=await target('chrome://gpu');
  await call('Accessibility.enable',{},session);
  let text='';
  for(let attempt=0;attempt<40;++attempt) {
    const tree=await call('Accessibility.getFullAXTree',{},session);
    text=tree.nodes.filter(n=>n.role?.value==='StaticText').map(n=>n.name?.value??'').join('\n');
    if(text.includes('Video Acceleration Information') && text.includes('Version Information'))
      return {text,complete:true};
    await delay(250);
  }
  return {text,complete:false};
}
const result={scope:job?'Isolated video API workload; hardware, independent reference and physical display acceptance remain separate':
              'Isolated GPU initialization and GL/WebGPU smoke; no codec, HDR or buffer-interop acceptance',
              capturedUTC:new Date().toISOString(),probeURL};
try {
  result.browser=await call('Browser.getVersion');
  assert.equal(result.browser.revision.replace(/^@/,''),
               '1d17a2e2ee147bb7d6ae2ab6abdb3a2510c17775','Unexpected source revision');
  result.before=await call('SystemInfo.getInfo');
  const beforeReport=await gpuReport();
  result.gpuReportText=beforeReport.text;
  result.gpuReportComplete=beforeReport.complete;
  const probeSession=await target(probeURL);
  if(job) {
    let ready=false;
    for(let attempt=0;attempt<40;++attempt) {
      const state=await call('Runtime.evaluate',{expression:
        'typeof ukeVideoProbe === "object" && isSecureContext',returnByValue:true},probeSession);
      if(state.result.value) {ready=true;break;}
      await delay(250);
    }
    assert(ready,'Video page unavailable or not a secure context');
    try {await call('Media.enable',{},probeSession);}
    catch(error) {result.mediaObserverError=String(error);}
    const argument=job.operation==='decode'?job.input:job.codec;
    const state=await call('Runtime.evaluate',{expression:
      'ukeVideoProbe.'+job.operation+'('+(argument===undefined?'':JSON.stringify(argument))+')',
      awaitPromise:true,returnByValue:true},probeSession);
    assert(!state.exceptionDetails,'Video workload evaluation failed');
    result.video=state.result.value;
    result.mediaEvents=mediaEvents.filter(event=>event.sessionId===probeSession);
    if(!result.video?.functionalPassed) process.exitCode=1;
  } else {
  for(let attempt=0;attempt<200;++attempt) {
    const state=await call('Runtime.evaluate',{expression:
      'window.gpuProbeDone ? JSON.stringify({secureContext:isSecureContext,result:window.gpuProbeResult}) : null',
      returnByValue:true},probeSession);
    if(state.exceptionDetails) throw new Error('Probe evaluation exception');
    if(state.result.value) {result.probe=JSON.parse(state.result.value);break;}
    await delay(100);
  }
  assert(result.probe,'No completed probe result');
  const probe=result.probe.result;
  result.glPixelsAndRestorePassed=Boolean(probe?.canvas?.passed && probe?.webgl?.passed &&
                                         probe?.webglContextRestore?.passed);
  if(!result.glPixelsAndRestorePassed) process.exitCode=1;
  }
  // Preserve each result: an unavailable WebGPU path is not a GL pass or failure.
  result.after=await call('SystemInfo.getInfo');
  const afterReport=await gpuReport();
  result.gpuReportTextAfter=afterReport.text;
  result.gpuReportCompleteAfter=afterReport.complete;
  if(!result.gpuReportComplete || !result.gpuReportCompleteAfter) process.exitCode=1;
  result.processes=await call('SystemInfo.getProcessInfo');
} catch(error) {
  result.error=String(error);
  process.exitCode=1;
} finally {
  console.log(JSON.stringify(result,null,2));
  // Never close the browser or any target that this observer did not create.
  for(const targetId of created) {
    try {await call('Target.closeTarget',{targetId});} catch {}
  }
  socket.close();
}
