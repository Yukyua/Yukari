# 原版指令实现与验证范围

2026-10-31。逆向目标仅为原版 `game.exe`（SHA-256 `08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267`）。357 个分发表项对应 325 个独立处理地址，全部已导出反编译正文；没有分析汉化 EXE、CNP 或补丁 DLL。

实际 C++ 资源解析器扫描原版五个 YKC：571 个 YKS、824,603 处静态调用、157 种使用中的函数。静态调用包含不可达分支，不能作为运行次数。

## 入口覆盖

| 类别 | 原版入口 | 本作使用的函数 | 静态调用 |
|---|---:|---:|---:|
| 明确实现或服务委托 | 332 | 149 | 824,128 |
| Case 声明及选择调度 | 25 | 8 | 475 |
| 仅登记 | 0 | 0 | 0 |
| 无处理路径 | 0 | 0 | 0 |

审计对全部入口调用实际服务 API，检查 runtime 的委托顺序、VM 指令和选择声明。参数或 provider 错误只能证明入口存在，不能证明其行为与原版一致。完整逐项来源见 [指令契约](instruction-contracts.md)，机器报告为 `analysis/compatibility/dispatch-coverage.json`。

## 行为验证

实现覆盖 VM 调用/选择/脚本状态、文字窗口与逐字/Ruby/图像字体、选择及事件调度、图层颜色/滤镜/运动/粒子、转场/特效、PCM 与流音频、影片、MIDI、CD 接口、平台服务、原版槽和系统数据、已读记录与历史。

原版声音使用分组、循环/静音状态及原始音量曲线；影片使用 FFmpeg 解码。SDL GPU 合成在真实 Metal renderer 上验证；调色、遮罩、画布、GraphicCopy、重叠自复制、逐字文字合成与转场遮罩生成已接入 Metal compute。63 组 CPU/GPU 差分验证整数操作逐字节一致，褐色浮点滤镜容差 1 色阶，另通过 Metal API Validation；解码与字形源数据准备仍在 CPU。详见 [GPU 管线](gpu-rendering.md)。CD 模块提供无设备语义、TOC/AIFF 解码及独立音源生命周期；真实物理设备未验证。

本轮运行验证：标题测试通过；连续推进 2001 页剧情进入 `common/com01` 并保存；对应原版格式槽再次导入后停在相同的脚本 word=28520。影片已完整解码。本轮纯模块、VM、渲染、音频、存档与合成脚本测试均独立执行；具体数量随新增测试更新，以 CTest 输出为准。它们不是全路线通关证明。

## 仍需保留的限制

- `tools/compatibility_contracts.json` 严格区分入口与契约：44 项已明确审查的局部契约标为 complete，其余 313 项默认 partial，unsupported 为 0。不能据此推导“引擎完成百分比”。
- 原版软件 DIB 字形后处理已实现：4 倍采样向下取整、定向矩形扩张、奇数厚度原点截断及组合阴影非对称偏移。缩小核心已与原版机器码对照 2,108 组、2,163,635 个输出像素一致；这不等于所有字形已与 Windows GDI 对齐。支持实际 GDI 彩色/透明度双平面缓存；当前样本来自 Wine 和替代字体，未采样字形仍用 FreeType。真实微软字体、原生 Windows 基准、DirectDraw 灰度描边与 RGB 子像素 mask 合成仍未完成验证。见 [字形管线](native-glyph-pipeline.md)。窗口消息映射和平台控件存在跨系统差异。
- 原版存档已通过字段/结构往返与 Yukari 恢复；Windows 原引擎读取 Yukari 输出尚未实测。系统配置保留模板中的未知字段，不把保留字段等同于完整行为还原。
- 本作未使用的指令有独立契约与合成验证，但未覆盖所有参数组合、所有媒体格式和所有硬件。
- 历史审计文档保留当时缺口供证据追踪；其中“未实现”名单不能代替此处的当前审计。

## 复现

```sh
cmake --build build -j
ctest --test-dir build --output-on-failure
python3 tools/audit_compatibility.py --game-dir game
python3 tools/audit_instruction_contracts.py
python3 tests/title.py --game-dir game --exe build/yukari
```

审计只读游戏数据；服务探针的临时文件与隐藏 SDL 窗口隔离在临时目录。审计报告包含源码哈希，新增接线后应重新生成。
