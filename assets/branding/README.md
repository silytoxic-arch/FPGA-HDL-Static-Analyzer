# 应用图标 V2 · 高级简约与功能表达

[Figma 可编辑设计](https://www.figma.com/design/QGY5w6p4GZ4ibG5Ek1oFFT?node-id=20-77)

芯片线框与引脚代表 FPGA；内部代码括号及源码行条代表 RTL；右下放大镜与检查标记
代表问题定位、静态分析和编码规则检查。图标无文字，采用石墨灰
`#252724`、温白 `#F6F6F3`、雾绿 `#A9C4B5`。图形与软件现有低饱和界面保持一致。

- `hdl-analyzer-icon.svg`：512 × 512 矢量源，圆角底板外侧透明。
- `hdl-analyzer-icon-1024.png`：Figma 导出的 1024 × 1024 透明 PNG。
- `icon-preview-v2.png`：512 px 主图标与 128 / 64 / 32 px 实际缩放预览。
- Figma V2 主组件：`20:76`；展示板：`20:77`。七处颜色绑定复用品牌语义变量，可继续编辑。
- `figma-icon-v2-state.json`：V2 节点、变量复用和验证记录。
- 旧版保留为 `hdl-analyzer-icon-old.svg` / `hdl-analyzer-icon-old-1024.png` / `hdl-analyzer-old.icns` / `icon-preview-old.png`；Figma 旧组件 `18:58` 和展示板 `18:59` 不变。

建议桌面应用图标使用 32 px 及以上，避免压缩内部线框。不要拉伸、叠加文字或新增外圈。
Qt 窗口资源继续引用 `hdl-analyzer-icon-1024.png`；此文件现已替换为 V2 的 Figma 导出。
macOS 发布前需从新版 PNG 重新生成 `hdl-analyzer.icns`；iconset 包含普通与 Retina 尺寸。
