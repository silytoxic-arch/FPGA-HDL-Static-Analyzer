# A-1-4 每行字符数原生恢复记录

公开规则编号严格采用官方三段式 `A-1-4`。内部条款键 `GJB10157.5.1.4` 仅用于源码、附件和逆向证据关联，不作为界面规则编号。

## 恢复对象

- 附件版本：T-Linter V3.1.4.251205
- 目标文件：`T-Linter.dll`
- SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`
- 构造函数：`FUN_180020d00`
- 活动入口：`FUN_180528fc0`
- 字符转换函数：`FUN_1804df420`
- 配置写入函数：`CheckRule::setMaxStatesNum`（`0x180628d30`）

## 恢复判据

1. 构造函数安装 `Yosys::GJB10157_VLOG_5_1_4::vftable`；活动虚表槽为 `FUN_180528fc0`，节点过滤槽只接纳 `AST_MODULE(2)`。
2. 规则对象 `+0x140` 的有符号整数为长度阈值；值为 0 时立即返回。
3. 活动入口遍历 `+0x68` 模块名映射，只处理向量中恰有一个模块根的键。重复同名模块使向量长度不为 1，该键整体跳过；没有模块根的源码不会被扫描。
4. 每个被接纳的模块键独立打开其根节点的源码路径并扫描整个文件。因此一个文件内有两个不同且唯一的模块名时，同一超长物理行会输出两条完全相同的诊断。
5. 每行不进行裁剪；注释、缩进和尾随空格均进入长度计算，换行符本身不进入。
6. `FUN_1804df420` 先以 `CP_UTF8` 调用 `MultiByteToWideChar`，再以活动的 Windows `CP_ACP` 调用 `WideCharToMultiByte`；比较结果为 Windows ANSI 字节数，而非 UTF-8 字节数或 Unicode 字符数。
7. 当转换长度大于或等于阈值时输出小写 `error`。诊断使用源码路径、物理行号，不携带已恢复列号；产品统一落在第 1 列。
8. 正文由“建议每行字符数小于”+十进制阈值+“个”拼接。标准阈值 80 对应精确正文“建议每行字符数小于80个”。

## 配置生产链

- DLL 导出函数 `CheckRule::setMaxStatesNum` 根据规则字符串定位对象并把整数写入 `+0x140`。
- `runCheck.exe` 对配置键 `GJB10157_VLOG_5_1_4_NUM` 执行十进制 `QString::toInt`，随后调用上述导出函数。
- 已恢复的安装元数据块没有展示数值默认值；80 来自官方标准和随包示例，产品固定采用 80。

## 产品接入

- 按解析得到的模块名构建映射，并只接纳恰有一个定义的模块键。
- 对每个接纳键重新扫描整份源码，保留原生重复诊断，不在最终去重阶段合并 `A-1-4`。
- ASCII 在所有平台精确计数；Windows 对非 ASCII 行复现 UTF-8 → UTF-16 → `CP_ACP` 的转换长度。
- 非 Windows 无活动 Windows ANSI 代码页，非 ASCII 行保持未知并停止对该行作出判断，不以 UTF-8 字节数冒充原生结果。
- 报告层把内部条款键映射为公开编号 `A-1-4`。

## 未解决边界

- 安装配置之外的默认阈值来源及用户自定义配置在产品界面中的开放方式。
- 活动 Windows ANSI 代码页带来的环境差异，以及非法 UTF-8 的专有前端恢复行为。
- 宏、include、虚拟文件、生成模块和错误恢复节点的完整源码归属。
- 与其他规则共同运行时的全局映射遍历和诊断排序。

因此本规则记为“证据支持的核心判据；集成源码适配器”，但安静结果仍为 `ManualReviewRequired`，不声明完整原生等价。

## 原始证据及哈希

| 证据 | SHA-256 |
| --- | --- |
| `reverse-tools/evidence/a1_4/native_constructor.txt` | `1ea8b05329e8f0b6a4c190643aab87958ae72454ceb8e9ebfeb5067169abd659` |
| `reverse-tools/evidence/a1_4/native_vtable.txt` | `5067ad33c49e562024c41321e4949c5b88ee08783645bd8af77e534f0454142b` |
| `reverse-tools/evidence/a1_4/native_core_functions.txt` | `5d370c021f0d1e4a7347369c82227cf7b7389caf6e231d72a340b02c7fee7fa7` |
| `reverse-tools/evidence/a1_4/native_config_setter.txt` | `bb61e19bc18045d8be7aa9aec5c9ffa59cba7f25db72ec572b1fbae5f8c11eef` |
| `reverse-tools/evidence/a1_4/native_line_conversion.txt` | `1165c6d990db4de4398d4ff0b1e5174558df7625bc0692e3208b18c2515cefca` |
| `reverse-tools/evidence/a1_4/runcheck_config_block.txt` | `e3082fe062c0e53b5d86f5579206620646a552327d095671cfb2913e0fccf795` |
| `reverse-tools/evidence/a1_4/runcheck_config_dispatch.txt` | `cf212add5b88c0c40739c4f8c7b0374bccfcfd9106b88b5c77be311218383afc` |
| `reverse-tools/evidence/a1_4/runcheck_metadata_block.txt` | `ba42bd7f2f10f0fde07cc738601aa66a8230eae8db9e4598a80cca18287bb9c5` |
