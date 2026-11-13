// SPDX-License-Identifier: MIT
import fs from 'node:fs';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const prefix = 'uke-control/';
const assets = [];
for (const folder of ['src', 'tests']) {
    assets.push(...fs.readdirSync(prefix+folder).filter(name => name.startsWith('motion-')).map(name => prefix+folder+'/'+name));
}
assets.push(...['motion.mk', 'motion-assets.mjs', 'uke-control-motion.service', 'org.senemos.Uke.Motion1.conf',
    'org.senemos.Uke.Motion1.service', 'org.senemos.uke.motion.policy', 'selinux/uke_control_motion.te',
    'selinux/uke_control_motion.fc'].map(name => prefix+'packaging/'+name));
const entries = assets.sort().map(path => ({path, sha256: crypto.createHash('sha256').update(fs.readFileSync(path)).digest('hex')}));
const path = prefix+'packaging/motion-integration.json';
const manifest = JSON.parse(fs.readFileSync(path));
if (process.argv[2] === '--refresh') {
    manifest.source_assets = entries;
    fs.writeFileSync(path, JSON.stringify(manifest, null, 2)+'\n');
} else {
    assert.equal(process.argv[2], '--check');
    assert.deepEqual(manifest.source_assets, entries);
}
console.log('Motion input manifest:', entries.length, 'exact source assets.');
