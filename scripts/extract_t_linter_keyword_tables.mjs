// Static observations, never target execution. Reconstruct the six contiguous
// string initializer ranges from retained run evidence and immutable PE bytes.
import fs from 'node:fs';
import crypto from 'node:crypto';
const [binaryPath, evidencePath, outputPath] = process.argv.slice(2);
if (!outputPath) throw new Error('Usage: node extract_keyword_tables.mjs DLL run.json output.json');
const bytes = fs.readFileSync(binaryPath);
const sha256 = crypto.createHash('sha256').update(bytes).digest('hex');
if (sha256 !== '60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0')
  throw new Error('Target identity mismatch');
const pe = bytes.readUInt32LE(0x3c);
if (bytes.readUInt32LE(pe) !== 0x4550 || bytes.readUInt16LE(pe + 24) !== 0x20b)
  throw new Error('Expected PE32+');
const imageBase = bytes.readBigUInt64LE(pe + 24 + 24);
const sections = [];
for (let i = 0; i < bytes.readUInt16LE(pe + 6); ++i) {
  const offset = pe + 24 + bytes.readUInt16LE(pe + 20) + i * 40;
  sections.push({name: bytes.subarray(offset, offset + 8).toString().replace(/\0.*$/, ''),
    rva: bytes.readUInt32LE(offset + 12), size: bytes.readUInt32LE(offset + 16),
    fileOffset: bytes.readUInt32LE(offset + 20)});
}
const retained = JSON.parse(fs.readFileSync(evidencePath, 'utf8'));
const evidence = retained.structuredContent ?? JSON.parse(retained.content[0].text);
const code = evidence.normalized_result ?? evidence.raw_result;
if (evidence.subject.digest.sha256 !== sha256 || typeof code !== 'string')
  throw new Error('Evidence identity/type mismatch');
const source = (value, length) => {
  if (value.startsWith('"')) {
    let text = JSON.parse(value);
    if (length !== undefined) text = text.slice(0, length);
    return {text, provenance: 'retained-pseudocode-literal', evidenceId: evidence.evidence_id};
  }
  const match = /^&DAT_([0-9a-f]+)$/.exec(value);
  if (!match) throw new Error('Unsupported string initializer ' + value);
  const va = BigInt('0x' + match[1]);
  const rva = Number(va - imageBase);
  const section = sections.find(s => rva >= s.rva && rva < s.rva + s.size);
  if (!section) throw new Error('Unmapped source address');
  const offset = section.fileOffset + rva - section.rva;
  let end = length === undefined ? bytes.indexOf(0, offset) : offset + length;
  if (end < offset || end > section.fileOffset + section.size) throw new Error('Unbounded string');
  const raw = bytes.subarray(offset, end);
  if (raw.some(c => c === 0 || c >= 128)) throw new Error('Unsupported keyword bytes');
  return {text: raw.toString('ascii'), provenance: 'immutable-PE-file-bytes',
    va: '0x' + va.toString(16), rva, fileOffset: offset, section: section.name, hex: raw.toString('hex')};
};
const categories = ['VHDL', 'Verilog', 'SystemVerilog', 'SDF/EDIF', 'PSL', 'Windows'];
const tables = [];
let slots = new Map();
// All admitted constructor operands are literal strings or direct DATA refs.
// Pair every builder with its following vector-destruction range and count.
const events = /FUN_180(040680|04a000|0d9010)\(local_([0-9a-f]+),\s*("(?:[^"\\]|\\.)*"|&DAT_[0-9a-f]+)(?:,\s*(0x[0-9a-f]+|[0-9]+))?\s*\)|FUN_180535cb0\(local_(31[1-6]0),&local_31e8\);\s*_eh_vector_destructor_iterator_\(local_([0-9a-f]+),0x20,(0x[0-9a-f]+|[0-9]+),FUN_18003efe0\)/g;
for (const event of code.matchAll(events)) {
  if (event[1]) {
    const length = event[4] === undefined ? undefined : Number(event[4]);
    if (event[1] === '040680' && length === undefined) throw new Error('Missing explicit length');
    slots.set(Number.parseInt(event[2], 16), source(event[3], length));
  } else {
    const count = Number(event[7]);
    const start = Number.parseInt(event[6], 16);
    const entries = Array.from({length: count}, (_, i) => {
      const slot = start - i * 32;
      const value = slots.get(slot);
      if (!value) throw new Error('Missing slot ' + slot.toString(16));
      return {stackSlot: 'local_' + slot.toString(16), ...value};
    });
    const category = categories[tables.length];
    if (event[5] !== '31' + (tables.length + 1) + '0') throw new Error('Unexpected builder order');
    tables.push({category, initializerCount: count, uniqueCount: new Set(entries.map(e => e.text)).size, entries});
    slots = new Map();
  }
}
if (tables.length !== 6 || tables.map(t => t.initializerCount).join(',') !== '115,126,117,1,67,22')
  throw new Error('Incomplete or unexpected six table ranges');
const output = {target: binaryPath, sha256, runEvidenceId: evidence.evidence_id,
  boundary: 'Static reconstruction from retained initializer calls plus SHA-bound file bytes; not runtime observation. Inline literals retain pseudocode provenance. No default category configuration inferred.',
  tables};
fs.writeFileSync(outputPath, JSON.stringify(output, null, 2) + '\n');
console.log(tables.map(t => ({category: t.category, initializers: t.initializerCount, unique: t.uniqueCount,
  dataRefs: t.entries.filter(e => e.va).length})));
