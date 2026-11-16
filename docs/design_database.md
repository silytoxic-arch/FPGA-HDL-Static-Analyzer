# Core Data Model 与 Design Database

## 范围

阶段 2 定义静态分析后续阶段共用的数据模型和只读查询接口。阶段 5 已能独立读取 HDL
文件并执行快速源码规则，但尚未通过完整语法树把解析结果写入 DesignDatabase，也尚未
把分析器接入 GUI。

命名空间按职责划分：

- `fpga::core`：`Project`、`SourceFile`、`SourceLocation` 和 HDL 语言类型。
- `fpga::design`：`Module`、`Port`、`Signal`、`Parameter`、`Instance`、`Assignment`、`AlwaysBlock`、`Clock`、`Reset`、`Driver`、`Load` 与 `DesignDatabase`。
- `fpga::analysis`：`Severity` 与 `Violation`。这些只是结果数据，不包含 Rule Engine。

## 所有权和引用

```text
Project
└── vector<SourceFile>

DesignDatabase
└── unordered_map<module name, unique_ptr<Module>>
    └── value collections
        ├── Port / Signal / Parameter / Instance
        ├── Assignment / AlwaysBlock
        └── Clock / Reset
            Signal ──owns──> Driver / Load
```

数据库独占模块。`unique_ptr<Module>` 使已插入模块的地址在哈希表扩容后保持稳定。模块以值拥有内部对象，模块、信号和实例之间的交叉关系保存稳定的名称标识，而不是裸指针。当前没有删除或修改已入库模块的接口，因此查询所得只读引用在 `DesignDatabase` 生命周期内有效。

`Project` 以值拥有 `SourceFile`。源码路径使用 `std::filesystem::path`，方便 Windows Unicode 路径边界处理；可选内存内容使用 `std::string` 自行持有，模型不会保存调用者传入的 `string_view`。

`SourceLocation` 的行号和列号从 1 开始，并单独记录从 0 开始的字节偏移。这样以后可区分 UTF-8 字节位置和编辑器显示列。

## 模型关系

`Signal` 包含名称、位宽、有符号属性、类型、所属模块、源码位置、驱动、负载、可选时钟域和可选复位域。`Driver` 与 `Load` 保存关系类别、目标或表达式摘要及来源位置。

`Module` 包含名称、定义文件和来源位置，并拥有端口、信号、参数、实例、连续赋值、always 块、时钟和复位集合。`Instance::moduleType` 表示被例化的模块名，`parentModule` 表示所属模块。阶段 2 不解析或自动校验这些名称关系。

`Violation` 包含规则 ID、四级严重度、消息、说明、建议、文件、行列，以及可选模块和信号上下文。它可由未来的 Rule Engine 生成，也可直接被 CLI、GUI 和报告层消费。

## DesignDatabase API

| 接口 | 行为 |
|---|---|
| `addModule(Module)` | 转移模块所有权；拒绝空名称和重复名称，返回 `false` |
| `findModule(name)` | 返回可选的只读模块引用 |
| `findSignal(module, signal)` | 在指定模块中查找信号，避免跨模块同名歧义 |
| `getModules()` | 返回按模块名排序的只读引用集合 |
| `getSignals()` | 按模块名顺序汇总信号，保留模块内声明顺序 |
| `getInstances()` | 按模块名顺序汇总实例，保留模块内声明顺序 |
| `getDrivers()` | 从所有信号汇总驱动记录 |
| `getLoads()` | 从所有信号汇总负载记录 |

返回值使用 `std::reference_wrapper<const T>`，避免复制大型模型，也不向调用者暴露可空裸指针。单项查询使用 `std::optional` 表达未找到。批量查询构造轻量引用视图；调用者不拥有返回对象。

## 当前约束

- 模块名称在数据库内唯一，空名称不可插入。
- `findSignal` 要求模块名和信号名，阶段 2 不提供模糊或全局同名解析。
- 模型暂不执行 HDL 语义验证，例如零位宽、重复信号、未解析实例、时钟域名称一致性。这些验证应由后续构建器、Parser 适配层或分析阶段报告，而不应藏在数据容器中。
- 数据库目前面向构建完成后的单线程只读访问。并发构建、增量更新、快照和序列化留给后续阶段。

## 测试

`tests/design_database_tests.cpp` 覆盖项目源码所有权与路径去重、所有核心模型字段、模块插入与重复拒绝、查找成功和失败、聚合查询、确定性模块顺序，以及插入新模块后已有模块引用的稳定性。
