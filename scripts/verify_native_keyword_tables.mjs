// Drift check against the separately retained initializer/file-byte observations.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const observations = JSON.parse(fs.readFileSync(path.join(root, 'docs/evidence/verilog_keyword_table_observations.json')));
const source = fs.readFileSync(path.join(root, 'src/analysis/verilog_keyword_names.h'), 'utf8');
const names = ['Vhdl','Verilog','SystemVerilog','SdfEdif','Psl','Windows'];
if (observations.sha256 !== '60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0')
    throw new Error('Observation target mismatch');
let count = 0;
for (let i = 0; i < names.length; ++i) {
    const match = new RegExp('std::array<std::string_view, ([0-9]+)> ' + names[i] + '\\{([\\s\\S]*?)\\n\\};').exec(source);
    if (!match) throw new Error('Missing compiled table ' + names[i]);
    const keys = [...match[2].matchAll(/"(?:[^"\\]|\\.)*"/g)].map(m => JSON.parse(m[0]));
    const expected = observations.tables[i].entries.map(e => e.text);
    if (Number(match[1]) !== expected.length || JSON.stringify(keys) !== JSON.stringify(expected))
        throw new Error('Initializer drift in ' + names[i]);
    count += keys.length;
}
console.log(`${count} compiled initializer entries match retained observations; no claim of vendor runtime equivalence.`);
