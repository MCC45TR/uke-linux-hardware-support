// Host-only comparison. Matching pixels do not establish which hardware produced them.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {readFile} from 'node:fs/promises';
import {basename,dirname,resolve} from 'node:path';
import {pathToFileURL} from 'node:url';

export function compare(video,reference,text) {
  assert(video?.functionalPassed,'Browser workload did not complete successfully');
  assert(/^[a-f0-9]{64}$/.test(reference.sourceSHA256),'Invalid reference source identity');
  assert.equal(video.sourceSHA256,reference.sourceSHA256,'Different encoded input');
  assert.equal(createHash('sha256').update(text).digest('hex'),reference.framehashSHA256,
    'Reference framehash file changed');
  const tb=text.match(/^#tb 0:\s*(\d+)\/(\d+)/m);
  assert(tb && Number(tb[2])>0,'Missing reference time base');
  assert(/^#hash:\s*SHA256$/mi.test(text),'Reference must use SHA-256');
  const expected=new Map();
  for(const line of text.split('\n').filter(line=>line.trim() && !line.startsWith('#'))) {
    const fields=line.split(',').map(value=>value.trim());
    assert.equal(fields.length,6,'Unexpected reference frame row');
    assert.equal(fields[0],'0','Unexpected reference stream');
    const timestamp=Math.round(Number(fields[2])*Number(tb[1])/Number(tb[2])*1000000);
    assert(Number.isSafeInteger(timestamp) && !expected.has(timestamp),'Invalid/repeated reference PTS');
    assert(/^[a-f0-9]{64}$/i.test(fields[5]),'Invalid reference digest');
    expected.set(timestamp,{size:Number(fields[4]),sha256:fields[5].toLowerCase()});
  }
  assert.equal(expected.size,30,'Expected thirty reference frames');
  assert.equal(video.frames?.length,30,'Expected thirty browser frames');
  const seen=new Set();
  for(const frame of video.frames) {
    assert.equal(frame.format,reference.format,'Different pixel representation');
    assert.equal(frame.visibleRect.width,reference.width,'Different visible width');
    assert.equal(frame.visibleRect.height,reference.height,'Different visible height');
    assert(!seen.has(frame.timestamp),'Repeated browser PTS');seen.add(frame.timestamp);
    const wanted=expected.get(frame.timestamp);
    assert(wanted,'Browser PTS is absent from reference');
    assert.equal(frame.packedByteLength,wanted.size,'Different visible plane size');
    assert.equal(frame.sha256,wanted.sha256,'Visible pixels differ at PTS '+frame.timestamp);
  }
  return {pixelReferencePassed:true,framesChecked:30,format:reference.format,
    sourceSHA256:reference.sourceSHA256,hardwareAccepted:false,
    note:'Independent same-input visible-plane comparison only; actual Iris/backend evidence remains required'};
}
if(process.argv[1] && import.meta.url===pathToFileURL(process.argv[1]).href) {
  try {
    const capture=JSON.parse(await readFile(process.argv[2],'utf8'));
    const reference=JSON.parse(await readFile(process.argv[3],'utf8'));
    assert.equal(basename(reference.framehashFile),reference.framehashFile,'Invalid reference path');
    const text=await readFile(resolve(dirname(process.argv[3]),reference.framehashFile),'utf8');
    console.log(JSON.stringify(compare(capture.video,reference,text),null,2));
  } catch(error) {
    console.log(JSON.stringify({pixelReferencePassed:false,hardwareAccepted:false,error:String(error)},null,2));
    process.exitCode=1;
  }
}
