# A-5-31 (`GJB9765.4.5.f.30`) client configuration boundary

## Artifact identity

- `T-Linter.dll` SHA-256: `60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- `runCheck.exe` SHA-256: `0ee30b930fe018acca0998ef6bfcf7949c081c2b13eeb494b5b83dc6238d8d27`
- Rule-object constructor: `FUN_1805f4f60` (`0x1805f4f60`)
- Base constructor: `FUN_18062a230` (`0x18062a230`)
- Exported configuration entry: `Yosys::CheckRule::setRuleNameConvention` (`0x180629290`)
- Four-string writer: `FUN_180628e70` (`0x180628e70`)
- Rule entry: `FUN_1805f5080` (`0x1805f5080`)

## Direct observations

1. `FUN_18062a230` initializes the four small-string fields at rule-object byte offsets `+0xc0`, `+0xe0`, `+0x100`, and `+0x120` as empty strings. `FUN_1805f4f60` replaces the vtable and initializes rule-specific containers, but does not write `+0xc0`.
2. `FUN_180628e70` copies four caller-supplied strings into exactly those four fields. The only recovered code caller inside `T-Linter.dll` is the exported `setRuleNameConvention` path.
3. `FUN_1805f5080` reads `+0xc0` as the comparison pattern. It uses mode `0` for reset-related prefix checks and mode `1` for non-reset substring checks.
4. `runCheck.exe` maps internal code `VLR-200055` to `GJB9765_VHDL.4.5.f.30`, but `VLR-200055` does not occur in the configuration dispatcher `FUN_140042500`. That dispatcher contains four calls to `setRuleNameConvention` for other explicitly compared rule-code families.
5. Every supplied `.lprp` occurrence of `GJB9765_VHDL_4_5_f_30_NUM` contains only enablement and severity state; no naming-convention value is present.
6. Among the supplied PE files, only `runCheck.exe` and `T-Linter.dll` contain the `setRuleNameConvention` symbol text. The GUI and console executables do not expose another observed binding path.

## Evidence-backed conclusion

The setter mechanism is recovered, but the supplied client path does not bind a value for A-5-31. Therefore the value `rst` must not be described as a recovered native object default. It is an independently documented policy value established by the packaged rule text and examples. The product may implement that documented policy, but it must keep the native configuration value and exact shipped-client runtime behavior unresolved.

This is a static observation. It does not prove that an unexamined external client, injected configuration, or runtime memory mutation cannot set the field.

