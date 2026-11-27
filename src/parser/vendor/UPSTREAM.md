# Yosys preprocessor integration

The preprocessor sources retain their ISC notices and originate from Yosys 0.17,
commit [6f9602b4cfedc6441f55272f6c025d64620cd24f](https://github.com/YosysHQ/yosys/commit/6f9602b4cfedc6441f55272f6c025d64620cd24f).
`preproc.cc` SHA-256: `0c61dbdd6c788805b8e24b4987aed896f4496497e76170b53af40c61360fa3df`.

The supplied target's version string names this prefix. Static macro argument,
default/conditional and file-stack paths corroborate this lineage. This is not
proof that every vendor customization or compilation option matches upstream.

Host adaptation:
- Move the namespace and remove kernel/RTLIL dependencies; provide standalone
  formatting, streams and exception-based diagnostics.
- Guard complete engine calls with a mutex and clear static queues on all exits.
- Use UTF-8-aware filesystem paths, check short include filenames before indexing,
  and reject filenames the native file-stack protocol cannot represent.
- Bound input/output bytes, input operations, include depth and macro-argument restoration depth;
  malformed, recursive or resource-limited input remains unproved.
- Track lexer `file_push`, `file_pop`, and `line` counters separately from expanded
  offsets. Physical-line rules retain their original inputs. Native declaration
  reports with no column use column 1; exact physical macro columns/offsets are
  not inferred.

A frontend session models public parserVerilogCode calls sharing the design's
macro cache until clear(). Each GJB10157 Analyzer invocation creates a private
session and processes roots in supplied order. Configured definitions are
reapplied on each root; source define/undef/resetall update the shared cache.
Public clear and the save/scratch/restore rule-preparation boundary are retained
in pass 134 evidence. Independent analyzer calls never share caches.

Default read_hdl defines YOSYS and SYNTHESIS; alternate formal/nosynthesis modes
are not exposed. Naming clauses 5.2.1/.2/.3/.4/.6/.12/.15 consume expanded input.
Generic physical-position projection receives the same initial defined-name
snapshot and directive validation. Its within-file include effects, remaining
structural/semantic models, full AST/lowering, optional frontends and vendor
changes outside recovered paths remain unresolved.

See `docs/native_rule_recovery_pass134.md` and `THIRD_PARTY_NOTICES.md`.
