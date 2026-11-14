// SPDX-License-Identifier: MIT
// Inspect checkpolicy's decompiled, linked policy; never load the host policy.
import fs from 'node:fs';
import assert from 'node:assert/strict';

function policy(text) {
    const types = new Set(), attributes = new Map(), allows = [];
    let depth = 0, start = 0, quoted = false, escaped = false, comment = false;
    for (let i = 0; i < text.length; ++i) {
        const c = text[i];
        if (comment) { if (c === '\n') comment = false; continue; }
        if (quoted) {
            if (escaped) escaped = false;
            else if (c === '\\') escaped = true;
            else if (c === '"') quoted = false;
            continue;
        }
        if (c === ';') { comment = true; continue; }
        if (c === '"') { quoted = true; continue; }
        if (c === '(') { if (depth++ === 0) start = i; }
        else if (c === ')') {
            assert(depth > 0);
            if (--depth !== 0) continue;
            const form = text.slice(start, i + 1);
            if (!/^\((?:type|typeattribute|typeattributeset|allow)\s/.test(form)) continue;
            const tokens = form.match(/[()]|[^()\s]+/g), stack = [];
            let root;
            for (const token of tokens) {
                if (token === '(') {
                    const node = [];
                    if (stack.length) stack.at(-1).push(node); else root = node;
                    stack.push(node);
                } else if (token === ')') { assert(stack.length); stack.pop(); }
                else { assert(stack.length); stack.at(-1).push(token); }
            }
            assert.equal(stack.length, 0);
            if (root[0] === 'type') types.add(root[1]);
            else if (root[0] === 'typeattributeset') attributes.set(root[1], root[2]);
            else if (root[0] === 'allow') {
                assert(Array.isArray(root[3]) && Array.isArray(root[3][1]), 'Expected decompiled class permission list');
                allows.push(root);
            }
        }
    }
    assert.equal(depth, 0); assert(!quoted);
    function contains(expr, type, visiting = new Set()) {
        if (typeof expr === 'string') {
            if (expr === type) return true;
            if (!attributes.has(expr) || visiting.has(expr)) return false;
            return contains(attributes.get(expr), type, new Set([...visiting, expr]));
        }
        if (expr[0] === 'all') return types.has(type);
        if (expr[0] === 'not') return !contains(expr[1], type, visiting);
        if (expr[0] === 'and') return expr.slice(1).every(x => contains(x, type, visiting));
        if (expr[0] === 'or') return expr.slice(1).some(x => contains(x, type, visiting));
        if (expr[0] === 'xor') return expr.slice(1).filter(x => contains(x, type, visiting)).length === 1;
        return expr.some(x => contains(x, type, visiting));
    }
    function allowed(source, target, klass, permission) {
        if (!types.has(source) || !types.has(target)) return false;
        return allows.some(row => contains(row[1], source)
            && (row[2] === 'self' ? source === target : contains(row[2], target))
            && row[3][0] === klass && row[3][1].includes(permission));
    }
    return {types, allowed};
}

const [mode, path] = process.argv.slice(2);
if (mode === '--self-test') {
    const sample = policy(`(type init_t) (type target_t) (type entry_t) (type other_t)
      (typeattribute entry) (typeattributeset entry (entry_t))
      (typeattribute users) (typeattributeset users (and (all) (not (init_t))))
      (allow init_t entry (file (execute open read getattr)))
      (allow target_t self (process (signal)))
      (allow users other_t (file (read)))`);
    assert(sample.allowed('init_t', 'entry_t', 'file', 'execute'));
    assert(!sample.allowed('init_t', 'other_t', 'file', 'execute'));
    assert(!sample.allowed('init_t', 'entry_t', 'file', 'write'));
    assert(sample.allowed('target_t', 'target_t', 'process', 'signal'));
    assert(!sample.allowed('target_t', 'init_t', 'process', 'signal'));
    assert(sample.allowed('target_t', 'other_t', 'file', 'read'));
    assert(!sample.allowed('init_t', 'other_t', 'file', 'read'));
    console.log('Linked-policy query: attributes, Boolean sets, self and absent permissions pass.');
    process.exit(0);
}
assert(['enabled', 'disabled-owned', 'disabled-sensor'].includes(mode) && path);
const {types, allowed} = policy(fs.readFileSync(path, 'utf8'));
const sensor = 'iiosensorproxy_t', executable = 'uke_motion_exec_t', runtime = 'uke_motion_runtime_t';
if (mode === 'enabled') {
    for (const permission of ['execute', 'getattr', 'open', 'read']) {
        assert(allowed('init_t', executable, 'file', permission),
            `Motion policy missing init_t -> ${executable}:file ${permission}`);
    }
    assert(allowed('init_t', sensor, 'process', 'transition'));
    assert(allowed('init_t', sensor, 'process2', 'nnp_transition'));
    for (const permission of ['execute', 'entrypoint', 'getattr', 'open', 'read', 'map']) {
        assert(allowed(sensor, executable, 'file', permission), 'Motion target entry point: '+permission);
    }
    for (const permission of ['search', 'write', 'add_name', 'remove_name']) {
        assert(allowed(sensor, runtime, 'dir', permission), 'Motion private directory: '+permission);
    }
    for (const permission of ['create', 'open', 'read', 'write', 'getattr', 'setattr', 'rename', 'unlink']) {
        assert(allowed(sensor, runtime, 'file', permission), 'Motion private journal: '+permission);
    }
} else if (mode === 'disabled-owned') {
    assert(!types.has(executable) && !types.has(runtime));
    assert(!allowed(sensor, 'bin_t', 'file', 'entrypoint'), 'Disabled module must not admit a generic executable');
} else {
    assert(!types.has(sensor));
    assert(types.has(executable) && types.has(runtime));
    assert(!allowed('init_t', executable, 'file', 'execute'), 'Disabled original sensor must not admit the new entry point');
}
console.log('Actual linked Fedora policy permissions: '+mode+' passes.');
