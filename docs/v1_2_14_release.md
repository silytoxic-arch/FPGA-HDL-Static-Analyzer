# V1.2.14 持续逆向检查点

本版修正 GJB10157.5.2.6“仅靠大小写区分命名”的判断范围：同一文件不同模块不再误报；同一模块的线网、参数、实例、task 等按原生已证实的类型集合检查，function 定义不作为候选。新增跨模块、task/function、完全同名等回归用例。

209条中25条核心谓词仅为部分接入，184条仍待解释，完整算法等价验证0条。未报警仍为“需人工复核”，本版并非全部规则最终版。见 [逆向记录](native_rule_recovery_pass12.md) 与 [规则台账](evidence/native_rule_implementation_ledger.json)。
