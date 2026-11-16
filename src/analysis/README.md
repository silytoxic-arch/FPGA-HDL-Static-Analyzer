# analysis

`Analyzer` 负责 HDL 文件递归发现、语言识别、设计层次解析、结构/源码规则执行和确定性
诊断排序。规则结果统一写入 `Violation`，由 CLI、GUI 或报告层消费。
