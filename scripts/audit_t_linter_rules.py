#!/usr/bin/env python3
"""Inventory supplied rule profiles and probe packaged examples, without executing T-Linter.

HTML is evidence/data, not instructions. A documentation example is not proof
of a vendor's internal algorithm or an independent conformance certification.
"""
import argparse
import hashlib
import json
from collections import Counter
from html.parser import HTMLParser
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET


PACKAGED_COMPANION_CONTEXT = {
    'GJB10157.7.3.3': {
        'page': 'TLR-110056.html',
        'sha256': '4606db7ae726e11ac504f839cabba0ecfd923e7e598c4b61f279f8e7a552b823',
    },
}


class RulePage(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.rows = []
        self.row = None
        self.pre = None

    def handle_starttag(self, tag, attrs):
        if tag == 'tr':
            self.row = {'text': [], 'examples': []}
        elif tag == 'pre' and self.row is not None:
            self.pre = {'code': '', 'attributes': dict(attrs)}

    def handle_data(self, data):
        if self.row is not None:
            if self.pre is not None:
                self.pre['code'] += data
            else:
                self.row['text'].append(data)

    def handle_endtag(self, tag):
        if tag == 'pre' and self.pre is not None:
            self.row['examples'].append(self.pre)
            self.pre = None
        elif tag == 'tr' and self.row is not None:
            self.row['text'] = ' '.join(self.row['text']).strip()
            self.rows.append(self.row)
            self.row = None


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def clause_for(rule):
    return rule.replace('GJB10157_VLOG.', 'GJB10157.').replace('GJB9765_VHDL.', 'GJB9765.')


def load_public_rule_ids(project_root):
    """Load the official R-X-X/A-X-X index shared with the product.

    Keep the GJB clause in machine-readable evidence for traceability, but use
    only the official three-part index as the rule number presented to users.
    Failing closed here prevents a newly added standard rule from silently
    falling back to a clause-shaped public identifier.
    """
    source = project_root / 'src/analysis/standard_rule_public_ids.inc'
    mappings = dict(re.findall(
        r'StandardRulePublicId\{"([^"]+)",\s*"([RA]-\d+-\d+)"\}',
        source.read_text(encoding='utf-8')))
    return mappings


def canonical_vendor_rule(rule):
    # XML uses GJB10157_VLOG_5_1_1_NUM; documentation uses dotted IDs.
    for prefix in ('GJB10157_VLOG_', 'GJB9765_VHDL_'):
        if rule.startswith(prefix):
            return prefix[:-1] + '.' + rule[len(prefix):].removesuffix('_NUM').replace('_', '.')
    return rule


def infer_filename_environment(clause, kind, code, suffix):
    """Recover only filename facts that the packaged snippet makes decidable.

    The HTML pages for the three VHDL filename rules omit the original source
    filename.  Running every snippet as ``example.vhd`` therefore manufactures
    false positives for compliant examples.  Synthesize the rule-prescribed
    filename when the target design unit is present, and explicitly defer a
    USE-only/declaration-only snippet whose referenced file is absent.
    """
    expected = kind == 'violating'
    if clause == 'GJB9765.4.5.f.5':
        match = re.search(r'\bentity\s+([A-Za-z][A-Za-z0-9_]*)\s+is\b', code,
                          re.IGNORECASE)
        if match:
            name = match[1]
            return ({'file_name': ('mismatched_' + name + suffix) if expected else name + '.vhd',
                     'basis': 'synthesized-from-entity-and-example-class'}, None)
    elif clause == 'GJB9765.4.5.f.9':
        match = re.search(r'\bpackage\s+(?!body\b)([A-Za-z][A-Za-z0-9_]*)\s+is\b', code,
                          re.IGNORECASE)
        if match:
            name = match[1]
            return ({'file_name': ('mismatched_' + name + suffix) if expected else name + '.vhd',
                     'basis': 'synthesized-from-package-declaration-and-example-class'}, None)
        if re.search(r'\buse\s+work\s*\.', code, re.IGNORECASE):
            return (None, '包声明源文件未随 USE-only 示例提供，无法验证其文件基名')
    elif clause == 'GJB9765.4.5.f.10':
        match = re.search(r'\bpackage\s+body\s+([A-Za-z][A-Za-z0-9_]*)\s+is\b', code,
                          re.IGNORECASE)
        if match:
            name = match[1]
            return ({'file_name': name + ('.vhd' if expected else '_body.vhd'),
                     'basis': 'synthesized-from-package-body-and-example-class'}, None)
        if re.search(r'\b(?:package|use\s+work\s*\.)', code, re.IGNORECASE):
            return (None, '包体源文件未随声明/USE-only 示例提供，无法验证其文件名')
    return ({'file_name': 'example' + suffix, 'basis': 'generic-fallback'}, None)


def infer_packaged_example_context(clause, code):
    """Return a reason when a snippet omits semantics required by its rule.

    GJB10157.7.3.3 compares the direction of a parent INOUT actual with the
    resolved child formal.  The shipped page contains only the parent module,
    so treating either example as a detector match/mismatch would manufacture
    knowledge about the missing child interface.
    """
    if clause != 'GJB10157.7.3.3':
        return None
    lexical = re.sub(r'/\*.*?\*/', ' ', code, flags=re.DOTALL)
    lexical = re.sub(r'//[^\n]*', ' ', lexical)
    definitions = {match[1].lower() for match in re.finditer(
        r'\bmodule\s+([A-Za-z_][A-Za-z0-9_$]*)\b', lexical, re.IGNORECASE)}
    targets = {match[1].lower() for match in re.finditer(
        r'\b([A-Za-z_][A-Za-z0-9_$]*)\s+(?:#\s*\([^;]*?\)\s*)?'
        r'[A-Za-z_][A-Za-z0-9_$]*\s*\(', lexical, re.IGNORECASE | re.DOTALL)
        if match[1].lower() not in {
            'module', 'function', 'task', 'if', 'for', 'while', 'repeat',
            'case', 'casex', 'casez', 'always', 'initial', 'assign'}}
    unresolved = sorted(targets - definitions)
    if unresolved:
        return ('子模块定义未随示例提供，无法解析形式端口方向：' +
                '、'.join(unresolved))
    return None


def unresolved_verilog_modules(code):
    """Return instantiated module names whose definitions are absent from code."""
    lexical = re.sub(r'/\*.*?\*/', ' ', code, flags=re.DOTALL)
    lexical = re.sub(r'//[^\n]*', ' ', lexical)
    definitions = {match[1].lower() for match in re.finditer(
        r'\bmodule\s+([A-Za-z_][A-Za-z0-9_$]*)\b', lexical, re.IGNORECASE)}
    targets = {match[1].lower() for match in re.finditer(
        r'\b([A-Za-z_][A-Za-z0-9_$]*)\s+(?:#\s*\([^;]*?\)\s*)?'
        r'[A-Za-z_][A-Za-z0-9_$]*\s*\(', lexical, re.IGNORECASE | re.DOTALL)
        if match[1].lower() not in {
            'module', 'function', 'task', 'if', 'for', 'while', 'repeat',
            'case', 'casex', 'casez', 'always', 'initial', 'assign'}}
    return sorted(targets - definitions)


def extract_verilog_module(code, module_name):
    """Extract one complete module definition while preserving packaged text."""
    match = re.search(
        r'(?is)(?:^|\n)[ \t]*module\s+' + re.escape(module_name) +
        r'\b.*?\bendmodule\b[^\r\n]*', code)
    return match[0].strip() + '\n' if match else None


def recover_modules_from_companion_page(page_path, expected_sha256, module_names):
    """Recover exact module definitions from one identity-bound packaged page."""
    if not page_path.is_file():
        return [], f'附件上下文页不存在：{page_path.name}'
    actual_sha256 = digest(page_path)
    if actual_sha256 != expected_sha256:
        return [], (f'附件上下文页哈希不匹配：{page_path.name}，'
                    f'期望 {expected_sha256}，实际 {actual_sha256}')
    page = RulePage()
    page.feed(page_path.read_text(encoding='utf-8-sig'))
    recovered = []
    for module_name in module_names:
        definition = None
        for row in page.rows:
            for example in row['examples']:
                definition = extract_verilog_module(example['code'], module_name)
                if definition:
                    break
            if definition:
                break
        if not definition:
            return [], (f'附件上下文页 {page_path.name} 未包含模块定义：'
                        f'{module_name}')
        recovered.append({
            'module': module_name,
            'code': definition,
            'source': str(page_path),
            'sha256': actual_sha256,
        })
    return recovered, None


def recover_packaged_example_context(examples_root, clause, code):
    """Recover only explicitly mapped, identity-bound cross-page context."""
    specification = PACKAGED_COMPANION_CONTEXT.get(clause)
    unresolved = unresolved_verilog_modules(code) if specification else []
    if not specification or not unresolved:
        return [], None
    return recover_modules_from_companion_page(
        examples_root / specification['page'], specification['sha256'], unresolved)


def documented_native_conflict(clause, kind, code, detected):
    """Identify only evidence-backed conflicts between a page label and DLL defaults.

    This does not change the expected page result or suppress diagnostics.  It
    prevents a recovered binary/documentation contradiction from being counted
    as an unexplained product mismatch.
    """
    if (clause == 'GJB10157.5.2.15' and kind == 'compliant' and detected and
            re.search(r'\bparameter\s+Data\b', code)):
        return ('附件把 parameter Data 标为正确，但 DLL 构造函数固定启用 parameter/localparam '
                'ASCII 小写拒绝，且专用标志不存在配置覆盖；见 '
                'native_verilog_naming_case_5_2_15_configuration_findings.md')
    if (clause == 'GJB9765.4.5.f.1' and kind == 'compliant' and detected and
            re.search(r'\b(?:entity|architecture)\s+utest_GJB_4_5_f_1_ccode',
                      code, re.IGNORECASE)):
        return ('附件把超过 20 字符的实体/结构体名称标为正确，但 DLL 的递归候选集明确包含 '
                'AST_ENTITY 与 AST_ARCHITECTURE，且统一采用 20 字节上限；见 '
                'native_vhdl_signal_name_length_4_5_f_1_findings.md')
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--attachment', required=True, type=Path)
    parser.add_argument('--cli', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--markdown', type=Path)
    parser.add_argument('--timeout', type=float, default=5)
    args = parser.parse_args()
    project_root = Path(__file__).resolve().parents[1]
    public_rule_ids = load_public_rule_ids(project_root)
    roots = list((args.attachment / 'install').glob('*/examples_html'))
    if len(roots) != 1:
        parser.error('Expected exactly one packaged examples_html directory')
    profiles = {}
    evidence = []
    for name in ('T-Linter.dll', 'T-Linter-Console.exe', 'LinterDB.db'):
        path = roots[0].parent / name
        if path.is_file():
            evidence.append({'path': str(path), 'sha256': digest(path),
                             'kind': ('identity-only for this example audit; not executed here; '
                                      'native static recovery is tracked separately')})
    for path in sorted(args.attachment.glob('*.lprp')):
        evidence.append({'path': str(path), 'sha256': digest(path), 'kind': 'rule-selection-xml'})
        for group in ET.parse(path).getroot():
            for rule in group:
                profiles.setdefault(canonical_vendor_rule(rule.tag), []).append({
                    'profile': path.name, 'group': group.tag,
                    'enabled': rule.findtext('Enabled') == 'true',
                    'severity': rule.findtext('Severity')})
    details_path = project_root / 'src/analysis/standard_rule_details.inc'
    modes = dict(re.findall(r'StandardRuleDetail\{"([^"]+)".*?StandardAssessmentMode::(\w+)',
                            details_path.read_text(encoding='utf-8')))
    records = []
    for path in sorted(roots[0].glob('*.html')):
        page = RulePage()
        page.feed(path.read_text(encoding='utf-8-sig'))
        examples = []
        summary = ''
        for row in page.rows:
            if row['text'].startswith('规则概述'):
                summary = row['text'][len('规则概述'):].strip()
            kind = 'violating' if '错误代码' in row['text'] else (
                'compliant' if '正确代码' in row['text'] else 'unclassified')
            for example in row['examples']:
                examples.append({'kind': kind, **example})
        clause = clause_for(path.stem)
        records.append({'rule_id': public_rule_ids.get(clause),
                        'vendor_rule': path.stem, 'clause': clause,
                        'summary': summary, 'source': str(path), 'sha256': digest(path),
                        'profiles': profiles.get(path.stem, []), 'examples': examples,
                        'detector_mode': modes.get(clause, 'not-in-product'),
                        'algorithm_status': 'documented-criterion-only; native-recovery-tracked-separately'})
    # Include selected rules for which this package has no example page.
    seen = {record['vendor_rule'] for record in records}
    for rule in sorted(profiles.keys() - seen):
        clause = clause_for(rule)
        records.append({'rule_id': public_rule_ids.get(clause),
                        'vendor_rule': rule, 'clause': clause, 'summary': '',
                        'profiles': profiles[rule], 'examples': [],
                        'detector_mode': modes.get(clause, 'not-in-product'),
                        'algorithm_status': 'no-packaged-example; native-recovery-tracked-separately'})
    missing_public_ids = sorted(record['clause'] for record in records
                                if record['clause'].startswith('GJB') and
                                not record.get('rule_id'))
    if missing_public_ids:
        parser.error('Official three-part rule IDs are missing for: ' +
                     ', '.join(missing_public_ids))
    prior = {}
    if args.baseline:
        for record in json.loads(args.baseline.read_text(encoding='utf-8'))['rules']:
            for index, example in enumerate(record['examples']):
                if 'probe' in example:
                    prior[(record['vendor_rule'], index)] = example['probe']
    total = 0
    if args.cli:
        cli = args.cli.resolve()
        with tempfile.TemporaryDirectory(prefix='fpga-rule-oracles-') as temp:
            for record in records:
                if not record['clause'].startswith('GJB'):
                    continue
                standard = 'gjb10157' if record['clause'].startswith('GJB10157.') else 'gjb9765'
                suffix = '.v' if standard == 'gjb10157' else '.vhd'
                for index, example in enumerate(record['examples']):
                    if example['kind'] == 'unclassified' or not example['code'].strip():
                        continue
                    declared_name = re.search(r'File\s*Name\s*:\s*([^\s()]+\.(?:vhd|vhdl|sv|v))\b',
                                              example['code'], re.IGNORECASE)
                    filename_basis = 'embedded-file-name'
                    recovered_context, recovery_reason = recover_packaged_example_context(
                        roots[0], record['clause'], example['code'])
                    combined_code = example['code'] + ''.join(
                        '\n' + context['code'] for context in recovered_context)
                    context_reason = recovery_reason or infer_packaged_example_context(
                        record['clause'], combined_code)
                    if declared_name:
                        file_name = Path(declared_name[1].replace('\\', '/')).name
                    else:
                        filename_environment, filename_context_reason = infer_filename_environment(
                            record['clause'], example['kind'], example['code'], suffix)
                        context_reason = context_reason or filename_context_reason
                        if filename_environment:
                            file_name = filename_environment['file_name']
                            filename_basis = filename_environment['basis']
                        else:
                            file_name = None
                    declared_file_name = file_name if declared_name else None
                    # Some packaged GJB10157 Verilog examples incorrectly label
                    # themselves .vhd. Preserve the basename but probe using the
                    # language of the selected rule, recording the adjustment.
                    if declared_name and standard == 'gjb10157' and Path(file_name).suffix.lower() in ('.vhd', '.vhdl'):
                        file_name = Path(file_name).stem + '.v'
                    elif declared_name and standard == 'gjb9765' and Path(file_name).suffix.lower() in ('.v', '.sv'):
                        file_name = Path(file_name).stem + '.vhd'
                    if not declared_name and record['clause'] == 'GJB10157.5.2.11':
                        # This rule's packaged examples identify their intended
                        # source basename in a `(parser)` comment, not a File Name
                        # field. Probing both as example.v invents a violation.
                        parser_name = re.search(r'//\s*([A-Za-z][A-Za-z0-9_]*)\s+\(parser\)',
                                                example['code'])
                        if parser_name:
                            file_name = parser_name[1] + suffix
                            filename_basis = 'parser-comment'
                    if context_reason:
                        example['probe'] = {
                            'context_required': context_reason,
                            'expected_detection': example['kind'] == 'violating',
                            'declared_file_name': None,
                            'probe_file_name': None,
                            'filename_basis': 'unavailable-from-packaged-snippet'}
                        if (record['vendor_rule'], index) in prior:
                            example['baseline_probe'] = prior[(record['vendor_rule'], index)]
                        total += 1
                        continue
                    # Do not retain earlier examples in the same analysis directory.
                    source = Path(temp) / file_name
                    source.write_text(example['code'], encoding='utf-8')
                    context_paths = []
                    for context in recovered_context:
                        context_path = Path(temp) / ('companion_' + context['module'] + suffix)
                        context_path.write_text(context['code'], encoding='utf-8')
                        context_paths.append(context_path)
                    try:
                        process = subprocess.run(
                            [str(cli), '--standard', standard, '--format', 'json',
                             str(source), *(str(path) for path in context_paths)],
                            capture_output=True, text=True, timeout=args.timeout)
                        result = json.loads(process.stdout)
                        # Public reports intentionally expose only the official
                        # three-part R/A index. The GJB clause remains an
                        # internal audit join key and is translated through the
                        # same checked mapping used to build each record.
                        flags = [v for v in result.get('violations', [])
                                 if v.get('ruleId') == record['rule_id']]
                        expected = example['kind'] == 'violating'
                        example['probe'] = {'detected': bool(flags), 'matches_documented_example': bool(flags) == expected,
                                            'diagnostics': flags, 'analysis_errors': result.get('errors', []),
                                            'exit_code': process.returncode,
                                            'declared_file_name': declared_file_name,
                                            'probe_file_name': file_name,
                                            'filename_basis': filename_basis}
                        if recovered_context:
                            example['probe']['recovered_context'] = [{
                                'module': context['module'],
                                'source': context['source'],
                                'sha256': context['sha256'],
                                'probe_file_name': context_path.name,
                            } for context, context_path in zip(recovered_context, context_paths)]
                        conflict = documented_native_conflict(
                            record['clause'], example['kind'], example['code'], bool(flags))
                        if conflict and not example['probe']['matches_documented_example']:
                            example['probe']['native_documentation_conflict'] = conflict
                    except (subprocess.TimeoutExpired, json.JSONDecodeError) as error:
                        example['probe'] = {'unresolved': type(error).__name__}
                    if (record['vendor_rule'], index) in prior:
                        example['baseline_probe'] = prior[(record['vendor_rule'], index)]
                    total += 1
                    if total % 40 == 0:
                        print(f'Probed {total} documented GJB examples', flush=True)
    matched = Counter()
    for record in records:
        for example in record['examples']:
            probe = example.get('probe', {})
            if 'native_documentation_conflict' in probe:
                matched['native-documentation-conflict'] += 1
            elif 'matches_documented_example' in probe:
                matched[record['detector_mode'] + (':matched' if probe['matches_documented_example'] else ':mismatch')] += 1
            elif 'unresolved' in probe:
                matched['unresolved'] += 1
            elif 'context_required' in probe:
                matched['context-required'] += 1
    implementation = None
    if args.cli:
        version = subprocess.run([str(args.cli.resolve()), '--version'], capture_output=True,
                                 text=True, timeout=args.timeout)
        implementation = {'cli_path': str(args.cli.resolve()), 'sha256': digest(args.cli.resolve()),
                          'version': version.stdout.strip() if version.returncode == 0 else None}
    report = {'boundary': 'Packaged documentation and local independent implementation; no vendor execution. Native recovery is tracked separately in native_rule_implementation_ledger.json, not established by example matching.',
              'implementation': implementation,
              'attachment': str(args.attachment), 'evidence': evidence,
              'summary': {'rules': len(records), 'packaged_pages': len(seen),
                          'standard_rules': sum(r['clause'].startswith('GJB') for r in records),
                          'probed_examples': total, 'probe_results': dict(matched)}, 'rules': records}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    if args.markdown:
        lines = ['# 附件逐规则证据与验证台账', '',
                 '本表覆盖附件规则配置和随包 HTML 的并集。文档判据不等于厂商内部算法；原生逆向证据和接入状态另见 native_rule_implementation_ledger.json。',
                 '“吻合”仅表示本软件在该文档示例上有/无目标诊断，不表示整个规则算法已完整实现。',
                 'ManualReview / ProjectContext 未检测到反例不是自动合规；保留待复核。', '',
                 '| 规则 | 附件判据 | 软件检测方式 | 文档示例吻合/已测试 | 未解决 |',
                 '|---|---|---|---|---|']
        for record in records:
            probes = [e['probe'] for e in record['examples'] if 'probe' in e]
            good = sum(p.get('matches_documented_example') is True for p in probes)
            unresolved = '内部算法未恢复'
            if not record.get('source'):
                unresolved += '；无随包说明'
            elif not probes:
                unresolved += '；未运行示例'
            elif any('native_documentation_conflict' in p for p in probes):
                unresolved += '；附件标注与 DLL 固定默认谓词冲突'
            elif any(p.get('matches_documented_example') is False for p in probes):
                unresolved += '；示例存在差异'
            elif any('context_required' in p for p in probes):
                unresolved += '；示例缺少被引用源文件上下文'
            elif any('unresolved' in p for p in probes):
                unresolved += '；示例执行未完成'
            summary = record['summary'].replace('|', '\\|').replace('\n', ' ')
            lines.append(f"| {record['rule_id']} | {summary or '无可验证的随包说明'} | "
                         f"{record['detector_mode']} | {good}/{len(probes)} | {unresolved} |")
        lines += ['', '表格规则编号采用官方三段式 R-X-X / A-X-X。内部 GJB 条款、完整文件路径、逐文件 SHA-256、正反例源码和源定位诊断见同目录 JSON 证据文件。', '']
        args.markdown.write_text('\n'.join(lines), encoding='utf-8')
    print(json.dumps(report['summary'], ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
