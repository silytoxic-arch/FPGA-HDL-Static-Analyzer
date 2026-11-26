#!/usr/bin/env python3
"""Boundary tests for packaged-example environment recovery."""

import importlib.util
import hashlib
from pathlib import Path
import re
import tempfile


SCRIPT = Path(__file__).resolve().parents[1] / 'scripts' / 'audit_t_linter_rules.py'
SPEC = importlib.util.spec_from_file_location('audit_t_linter_rules', SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def infer(clause, kind, code):
    return MODULE.infer_filename_environment(clause, kind, code, '.vhd')


def main():
    public_ids = MODULE.load_public_rule_ids(Path(__file__).resolve().parents[1])
    assert len(public_ids) == 209
    assert public_ids['GJB10157.5.2.1'] == 'R-2-1'
    assert public_ids['GJB9765.4.8.e'] == 'A-8-1'
    assert all(re.fullmatch(r'[RA]-\d+-\d+', value) for value in public_ids.values())

    environment, reason = infer('GJB9765.4.5.f.5', 'compliant',
                                'entity Core_ent is end entity;')
    assert reason is None and environment['file_name'] == 'Core_ent.vhd'
    environment, reason = infer('GJB9765.4.5.f.5', 'violating',
                                'entity Core_ent is end entity;')
    assert reason is None and environment['file_name'] != 'Core_ent.vhd'

    environment, reason = infer('GJB9765.4.5.f.9', 'compliant',
                                'package Types_pack is end package;')
    assert reason is None and environment['file_name'] == 'Types_pack.vhd'
    environment, reason = infer('GJB9765.4.5.f.9', 'violating',
                                'library work; use work.Types_pack.all;')
    assert environment is None and '未随' in reason

    environment, reason = infer('GJB9765.4.5.f.10', 'compliant',
                                'package body Types_pack is end package body;')
    assert reason is None and environment['file_name'] == 'Types_pack_body.vhd'
    environment, reason = infer('GJB9765.4.5.f.10', 'violating',
                                'package Types_pack is end package;')
    assert environment is None and '包体源文件' in reason

    environment, reason = infer('GJB9765.4.5.f.1', 'compliant',
                                'entity Core_ent is end entity;')
    assert reason is None and environment['file_name'] == 'example.vhd'

    missing = MODULE.infer_packaged_example_context(
        'GJB10157.7.3.3',
        'module parent(inout IO); child u0(.IN1(IO)); endmodule')
    assert missing is not None and 'child' in missing
    complete = MODULE.infer_packaged_example_context(
        'GJB10157.7.3.3',
        'module child(input IN1); endmodule\n'
        'module parent(inout IO); child u0(.IN1(IO)); endmodule')
    assert complete is None
    unrelated = MODULE.infer_packaged_example_context(
        'GJB10157.6.1.7',
        'module parent; unknown u0(); endmodule')
    assert unrelated is None

    companion_html = '''<html><table><tr><td>正确代码</td><td><pre>
module parent(inout IO); child u0(.IN1(IO)); endmodule
module child(input IN1); endmodule
</pre></td></tr></table></html>'''
    with tempfile.TemporaryDirectory(prefix='fpga-audit-context-') as temp:
        companion = Path(temp) / 'TLR-test.html'
        companion.write_text(companion_html, encoding='utf-8')
        expected_hash = hashlib.sha256(companion.read_bytes()).hexdigest()
        recovered, reason = MODULE.recover_modules_from_companion_page(
            companion, expected_hash, ['child'])
        assert reason is None and len(recovered) == 1
        assert recovered[0]['module'] == 'child'
        assert recovered[0]['code'].startswith('module child')
        assert recovered[0]['code'].rstrip().endswith('endmodule')
        rejected, reason = MODULE.recover_modules_from_companion_page(
            companion, '0' * 64, ['child'])
        assert not rejected and '哈希不匹配' in reason
        missing_definition, reason = MODULE.recover_modules_from_companion_page(
            companion, expected_hash, ['missing'])
        assert not missing_definition and '未包含模块定义' in reason

    target_sha256 = '60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0'
    conflict = MODULE.documented_native_conflict(
        'GJB10157.5.2.15', 'compliant', 'parameter Data = 1;', True, target_sha256)
    assert conflict is not None and 'DLL' in conflict
    assert MODULE.documented_native_conflict(
        'GJB10157.5.2.15', 'violating', 'parameter data = 1;', True, target_sha256) is None
    assert MODULE.documented_native_conflict(
        'GJB10157.5.2.15', 'compliant', 'parameter Data = 1;', False, target_sha256) is None
    constant_example = (Path(__file__).resolve().parent / 'fixtures' /
                        'vhdl_a10_4_documentation_violation.vhd').read_text(encoding='utf-8')
    assert hashlib.sha256(constant_example.encode('utf-8')).hexdigest() == (
        '192b5b175afb84bf969b23512b510942a1bb6367244b80cb87088af20969cb18')
    assert MODULE.documented_native_conflict(
        'GJB9765.4.10.h', 'violating', constant_example, False, target_sha256) is not None
    assert MODULE.documented_native_conflict(
        'GJB9765.4.10.h', 'violating', constant_example + '\n-- changed', False,
        target_sha256) is None
    assert MODULE.documented_native_conflict(
        'GJB9765.4.10.h', 'violating', constant_example, True, target_sha256) is None
    assert MODULE.documented_native_conflict(
        'GJB9765.4.10.h', 'compliant', constant_example, False, target_sha256) is None
    for unknown_target in (None, '0' * 64):
        assert MODULE.documented_native_conflict(
            'GJB10157.5.2.15', 'compliant', 'parameter Data = 1;', True,
            unknown_target) is None
        assert MODULE.documented_native_conflict(
            'GJB9765.4.10.h', 'violating', constant_example, False, unknown_target) is None
        assert MODULE.documented_native_conflict(
            'GJB9765.4.5.f.1', 'compliant',
            'entity utest_GJB_4_5_f_1_ccode is end entity;', True,
            unknown_target) is None
    assert MODULE.documented_native_conflict(
        'GJB9765.4.10.h', 'violating',
        "process(clk) begin if rising_edge(clk) then dout<='0'; end if; end process;",
        False, target_sha256) is None  # Unrelated source cannot inherit the exact-example exception.
    print('audit_t_linter_rules environment-boundary tests passed')


if __name__ == '__main__':
    main()
