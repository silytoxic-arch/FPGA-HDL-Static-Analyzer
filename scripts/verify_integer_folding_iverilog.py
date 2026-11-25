#!/usr/bin/env python3
"""Independent two-state HDL oracle; never runs the proprietary target."""
import argparse
import json
import pathlib
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--test-binary", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    rows = subprocess.run([args.test_binary, "--dump"], check=True, capture_output=True, text=True)
    cases = [line.split("\t") for line in rows.stdout.splitlines()]
    if not cases or any(len(row) != 6 for row in cases):
        raise RuntimeError("Unexpected model output")
    source = []
    for index, (decl, expr, _, _, _, _) in enumerate(cases):
        # Display arguments evaluate naturally. Casting here would introduce a
        # context width and change inner arithmetic before comparison.
        source.append(f'module probe_{index}; {decl} initial begin '
                      f'$display("{index}\\t%0d\\t%h", '
                      f'$bits({expr}), ({expr})); end endmodule')
    with tempfile.TemporaryDirectory(prefix="fpga-integer-oracle-") as folder:
        sv = pathlib.Path(folder) / "probe.sv"
        executable = pathlib.Path(folder) / "probe.vvp"
        sv.write_text("\n".join(source), encoding="utf-8")
        subprocess.run(["iverilog", "-g2012", "-gstrict-expr-width", "-o", str(executable), str(sv)], check=True)
        result = subprocess.run(["vvp", str(executable)], check=True, capture_output=True, text=True)
    actual = {}
    for line in result.stdout.splitlines():
        index, width, bits = line.split("\t")
        actual[int(index)] = (int(width), int(bits, 16))
    checks = []
    for index, (decl, expr, width, bits, cache, sign) in enumerate(cases):
        expected = (int(width), int(bits))
        # Native appends a zero sign bit to positive wide unsized decimals.
        # Icarus strict mode instead uses 32 bits; preserve this known boundary.
        decimal_boundaries = {"2147483648+0": ((33, 2147483648), (32, 2147483648)),
                              "4294967295+1": ((33, 4294967296), (32, 0))}
        native_decimal_boundary = not decl and decimal_boundaries.get(expr) == (expected, actual.get(index))
        checks.append({"declaration": decl, "expression": expr, "model": expected,
                       "iverilog": actual.get(index), "matched": actual.get(index) == expected,
                       "knownNativeDecimalBoundary": native_decimal_boundary})
    version = subprocess.run(["iverilog", "-V"], capture_output=True, text=True).stdout.splitlines()[0]
    report = {"scope": "Independent HDL semantics, not proprietary runtime equivalence",
              "oracle": version, "flags": ["-g2012", "-gstrict-expr-width"],
              "scopeNote": "Checks natural width and bits; signed cache has separate C++ assertions. Two native wide-decimal boundaries are recorded separately.",
              "total": len(checks),
              "matched": sum(item["matched"] for item in checks), "checks": checks}
    pathlib.Path(args.output).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f'{report["matched"]}/{report["total"]} integer folding oracle checks matched')
    failures = [item for item in checks if not item["matched"] and not item["knownNativeDecimalBoundary"]]
    if failures:
        for item in checks:
            if item in failures:
                print(json.dumps(item))
        raise SystemExit(1)


if __name__ == "__main__":
    main()
