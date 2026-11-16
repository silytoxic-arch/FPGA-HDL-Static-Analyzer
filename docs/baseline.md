# 诊断基线与增量 CI

诊断基线用于已有 RTL 项目的渐进式接入：历史问题保持可计数，但 CLI 退出阈值只评估未被
基线匹配的新问题。

首次生成：

```sh
fpga_analyzer_cli --write-baseline .fpga-lint-baseline --format json \
  --output initial-analysis.json ./rtl
```

成功写入时命令返回 0，即使当前存在 warning/error。将基线文件纳入版本控制，然后在
`.fpga-lint.conf` 中启用：

```text
baseline .fpga-lint-baseline
fail-on warning
```

也可以仅在 CI 命令中使用：

```sh
fpga_analyzer_cli --baseline .fpga-lint-baseline --format sarif \
  --output fpga-lint.sarif ./rtl
```

## 匹配与更新规则

- 指纹包含规则 ID、规范化文件路径、消息、设计单元和信号上下文。
- 严重级不参与指纹；调整 severity policy 不会使已有基线失配。
- 行号和列号不参与指纹，因此源码前方插入或删除行不会使历史诊断失配。
- 重复指纹按基线条目数量逐个消费，不会用一个条目隐藏任意数量的新问题。
- 基线中的相对路径以基线文件目录解析；位于项目中的基线可随项目目录一起移动。
- 消息、路径和上下文字段采用反斜线转义的制表符分隔格式，首行为版本标识
  `fpga-lint-baseline<TAB>1`。无效版本、字段或转义会使 CLI 返回 2。

修复历史问题后，应重新运行 `--write-baseline` 更新文件；不要同时传入 `--baseline` 和
`--write-baseline`。报告的 `baselinedViolations` 与 suppression 分开显示，便于跟踪历史
问题是否持续减少。
