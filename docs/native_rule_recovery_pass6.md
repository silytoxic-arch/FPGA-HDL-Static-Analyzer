# 第六轮：命名与状态机判据接入

目标 DLL SHA-256：`60e6b52875f4d7c3fbdf494d869b3a4bcc3fdc8ee63d8620ea9614da5d6599b0`。
静态伪代码和字节观察不等于原始源码或实际执行结果；本轮未执行厂商软件。

## 命名

5.2.1 run `0x18052c420`：`ev_8ed10f75820818d3d20d08887706e8cd7659c8e91158a8f12d34d1be83f68c3b`。
目标类型为 MODULE/WIRE/PARAMETER/LOCALPARAM/CELL/TASK/FUNCTION，非空名称首字节数字或 `_` 报错。
5.2.2 run `0x180535370`：`ev_32bd8b91ae9488d76d830d21919304bee359a91224d19bde6409118acd1fab75`。
类型集合不含 FUNCTION，本体名不执行连续/末尾下划线检查。
连续下划线常量字节：`ev_440621dd15465fbff20fba21f55c4de203e5e109759ef80d4edf7fcab5abdb86`。
5.2.12 ctor/run：`ev_72031d097e80e28af266b1d999a091e98e505f631f13006bbb16c39c3b51e52b`、
`ev_18d1ea59101ccfacbaf11002bc798a90887dab8fc02865c01c8d2c786def1449`，默认2..32闭区间。
源码适配器仅保守绑定可靠声明；未知前端归一化和配置覆盖未恢复。

## FSM

显式转换源状态键数 >40：`ev_a676773f6e4eb5ee266b847f12395fef2bd86765be6969ce538488088be98c2e`。
编码字段 !=0 / !=1：`ev_55a076e093a07e39b21fe2f2fa169dc037a5ba2864f0b5534b0a87ac85c1c55e`、
`ev_ff5656816acfb070f1c6db099db6b3248d2d262ef0d64bcccd626e212594d08e`。
模型写入：`ev_62e99d995d32c416ee4a9a23432091d0f2496edf47e2c8c76ff29a4b7a1aaafb`；
默认/顺序0、独热1、格雷2，safe 是独立标志，未改变默认编码。
源码绑定和面积/速度目标配置是产品独立适配，不能称为原生完全恢复。
局部转换图是 may-transition，不能据此认定不可达、死锁或复位性质；4.11.a/d仍未接入。

## 文件头配置入口

PE export 观察定位 `setHeaderComment@CheckRule@Yosys`。
setter `ev_c2a2f9228d4b8abb364922693e1df5a80dac61114d0247578e399a2feb80b310` 查规则名并传入map。
装载 helper `ev_7b37ea997846cac845b54b7f69002f632c1e915ee64d9657c2046369c88ed821`
清旧tree，重置size，然后复制输入到Rule +0x148/+0x150。
复制链见 pass3/098..101；最终pair构造
`ev_6518abafe2526f0e6d88e15c4685588f7b0ad7c50e75905cf81c6525c2766168`
将value bool原样写入，未观察到翻转。
真实外部调用传入的默认map仍未知；该入口不是默认Author/History模板的证据。

完整新证据留存于 `reverse-tools/evidence/t_linter_pass3/session-evidence-checkpoint5.json`
（1250条记录），每条规则恢复/实现/测试和限制见随包台账。
验证见 [V1.2.8检查点](v1_2_8_release.md)，附件复测见 [审计](t_linter_native_pass6_audit.md)。
