# 格式证据与实施边界

日期：2026-10-31。

本文记录首版工具的公开格式依据；后续已取得 参考样本 原始样本，见 [原始资源审计](original-resources.md) 和 [原版引擎逆向](original-engine.md)。下文“没有提供游戏”的记录属于首次建仓时的历史状态。

## 本轮原版反汇编

通过 REA 6.4.0 / Ghidra 12.1.4 对原版 PE32 x86 分发表的 325 个独立 handler 完成正文导出，覆盖全部 357 个入口；进一步追踪了文字、选择、图层、声音、保存与配置 helper。所有分析使用临时 Ghidra 工程，源 EXE 只读。两次会话均已导出证据并关闭：

- `analysis/original-engine/instructions/session-evidence.json`：449 条记录。
- `analysis/original-engine/instructions2/session-evidence.json`：113 条记录，1,310,101 字节。

正文、地址、Evidence ID 和当前实现路由见 [完整指令契约](instruction-contracts.md)。入口与正文数量不等于行为已逐项实机验收；验证范围见 [当前审计](compatibility-audit.md)。

## 实物与环境

- 用户给出了引擎名称、语言和平台目标；没有提供游戏路径或二进制。
- `/Users/zhang/Work/YukaSystem` 初始为空，没有可复用的代码或 AGENTS.md。
- 当前没有连接 REA MCP，PATH 中没有 rea CLI。未安装工具、修改注册或打开任何目标分析会话。没有目标 EXE 时安装反编译器不能解决语义验证问题。
- 本地已存在 AppleClang 17、CMake、SDL2 2.32.72（sdl2-compat）与 libpng 1.6.59。

## 公开源码证据

主要参考 AtomCrafty/YukaTool2，固定提交
`fd8f445dfe685f5d0b724cfecbcbc91963a87376`：

- [YKS 头部](https://github.com/AtomCrafty/yukatool2/blob/fd8f445dfe685f5d0b724cfecbcbc91963a87376/src/Yuka.Core/IO/Formats/Yks.cs)
- [反汇编结构](https://github.com/AtomCrafty/yukatool2/blob/fd8f445dfe685f5d0b724cfecbcbc91963a87376/src/Yuka.Core/Script/Binary/Disassembler.cs)
- [索引元素类型与字段](https://github.com/AtomCrafty/yukatool2/blob/fd8f445dfe685f5d0b724cfecbcbc91963a87376/src/Yuka.Core/Script/Data/DataElement.cs)
- [数据区解码](https://github.com/AtomCrafty/yukatool2/blob/fd8f445dfe685f5d0b724cfecbcbc91963a87376/src/Yuka.Core/Script/Data/DataSector.cs)
- [YKG 三个区段](https://github.com/AtomCrafty/yukatool2/blob/fd8f445dfe685f5d0b724cfecbcbc91963a87376/src/Yuka.Core/IO/Formats/Ykg.cs)
- [动画记录](https://github.com/AtomCrafty/yukatool2/blob/fd8f445dfe685f5d0b724cfecbcbc91963a87376/src/Yuka.Core/Graphics/Animation.cs)

交叉参考 GARbro 的 [YKC](https://github.com/morkt/GARbro/blob/master/ArcFormats/Yuka/ArcYKC.cs) 与 [YKG](https://github.com/morkt/GARbro/blob/master/ArcFormats/Yuka/ImageYKG.cs)。GARbro 使用 master 链接，此处没有声称它是固定版本或真实游戏运行证据。

以上是公开源码的格式证据，不是本项目对游戏主程序的逆向结果；没有 REA Evidence ID。

## 初始格式工具阶段（历史记录）

所有数值均按小端读取，偏移与长度使用减法检查边界，乘法在 64 位执行。

| 格式 | 公开源码观察 | 本项目处理与限制 |
|---|---|---|
| YKC001/002 | 0x10/0x14 为目录偏移/字节长度；每项 20 字节，含名称偏移/长度、资源偏移/长度、保留字段 | 严格匹配签名，目录按需加载；001 CP932，002 UTF-8；不假设跨包覆盖行为 |
| YKS001 | 0x10/0x14 为代码偏移/DWORD 数；0x18/0x1c 为索引偏移/项数；0x20/0x24 为数据偏移/长度；索引项 16 字节 | 标志为 1 时仅对声明的数据区 XOR 0xAA，拒绝未知标志；保留原文件 |
| YKS 指令流 | 每条首字为索引项编号；Func 后跟参数个数与参数索引；Ctrl 为控制项 | 输出代码区 word 偏移，未执行任何函数；Ctrl link 以原始整数输出，不猜测目标合法性 |
| YKS 索引 | Func=0, Ctrl=1, CInt=4, CStr=5, SStr=7, VInt=8, VStr=9, VLoc=10 | 按公开实现解释；11/12 等明确失败，不把未知语义吞掉 |
| YKG000 | 0x28 至 0x3c 为颜色、Alpha、帧区的偏移/长度；帧记录 32 字节 | 三段均检查范围；保留所有帧字段；只预览颜色平面 |
| GNP | PNG 前四字节置换为 89 47 4e 50 | 在解码副本中还原 PNG 签名，不写回源文件 |

旧 YKG 的颜色偏移/长度回退行为参考 GARbro。它与独立 Alpha/帧段共存时的解释仍需真实样本确认。YKG 帧的 duration 保留为原始值，不承诺毫秒单位；type/action 等字段仅输出数值。

## 验证与未知项

`tests/integration.py` 生成独立的合成样本，覆盖两种 YKC、日文文件名、原样提取、加密与明文 YKS、已知元素、负 link、YKG Alpha/帧元数据、BMP/PNG/GNP 上传和 SDL 软件渲染，以及截断、畸形长度、无效编码和未知类型。

这些测试验证读取器与预览路径，不能证明任何游戏可运行。仍需实际 EXE/资源包确认启动路径、函数语义、版本差异、资源搜索、Shift-JIS 扩展、文字换行/禁则/字体/字距、透明度、动画控制、视频、音频、输入和存档。

本机验证记录：

- Debug 构建通过；CTest 执行 10 组合成样本测试全部通过。
- AddressSanitizer + UndefinedBehaviorSanitizer 构建与同一组测试通过，没有检测到异常。
- SDL dummy video + software renderer 下，四种图像载体与包内图像成功创建纹理并呈现一帧。
- 原生 macOS 显示服务下执行 `yuka view /private/tmp/yuka-preview.png --once` 成功，输出 `Preview 2x2`。沙箱内无法枚举显示器，使用获准的沙箱外执行完成原生窗口验证。
- 初始格式工具阶段尚无游戏样本；后续真实游戏验证见下节。


## 参考样本 当前运行时（2026-10-31）

仅分析原版 `game.exe`（SHA-256 `08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267`），配合原始五个YKC中的脚本。原生Ghidra会话已导出至 `analysis/original-engine/story/session-evidence.json` 并关闭。

- [剧情与菜单字段审计](story-menu-audit.md)：StrOut、KeyWait、PF、MouseCase、设置/存读档脚本与场景字段。
- [存档格式](save-format.md)：原版writer/reader、current/next、类型化locals、可变数据别名，以及实验性写出边界。
- [标题运行时](title-runtime.md)：起始脚本至标题的基础图层/精灵与音频证据。

验证包含8个CTest目标（formats、VM、audio、render、text、save、original_scene、original_system），ASan/UBSan通过，16个真实纯VM脚本分支通过。标题及剧情集成覆盖菜单、槽位确认、同/跨进程恢复，原生槽恢复后继续同一KeyWait位置；测试检查原游戏EXE、资源包与save文件的size/mtime保持不变。78个现有槽另做只读内存结构与VM往返检查。

静态结构证据和Yukari读回验证不能替代Windows原引擎实机互操作。FreeType排版、淡化转场与历史界面属于移植实现；原版Already.dat、AVI和全部路线仍待适配与验证。

长剧情加速回归从序章首段继续推进850个KeyWait，完整通过prologue01及Eye_Catch04章节过场并进入prologue02，最后停在word1977；过程中修正NowModeGet所需的跳过/自动状态查询。该验证覆盖所经默认路线，未代表全分支/全作通关。

## 字形与边框补充（2026-10-31）

原版 GDI 软件路径的新增会话证据：`analysis/original-engine/gdi-fonts2/session-evidence.json`，七个 helper 导出完成且会话已关闭。实现与验证边界见 [字形管线](native-glyph-pipeline.md) 和 [像素核心](native-glyph-kernel.md)。原版 405250 软件缩小 helper 的 1,012 字节机器码另经 SHA 锁定与私有 RX 页执行，2,108 组 / 2,163,635 个输出像素与生产实现逐字节一致；三个重定位操作数仅指向隔离 selector、malloc、free，算术字节未改，未执行游戏启动路径。详见 [独立报告](../tools/gdi_oracle/native-downsample-report.md)。Wine 采样与替代字体不能充当原生 Windows 字形一致性证据。当前 CTest 49/49 通过，另有 Metal 真实绘制读回与 ASan/UBSan 验证。

## Metal 像素管线补充（2026-10-31）

新增 Metal compute 与 SDL 公共编码器交接，覆盖资源遮罩、图像调色、画布、PATCOPY、原始 Scalar/MMX 复制、重叠自复制、逐字 straight-alpha 合成及转场遮罩。63 组 / 286,388 个 RGBA 字节差分通过，整数操作精确、褐色滤镜容差 1 色阶；Metal API Validation、普通/GDI 文字 GPU 模式及 49 项 CTest 通过。真实游戏 Metal 标题、菜单、历史及跨进程原生/Yukari 存读档回归通过。保存预览缺失的 DBCS 与字形尺寸接口已补齐，并增加日文注释预览回归。源游戏保持只读。架构、显式读回及 CPU 源数据准备边界见 [GPU 文档](gpu-rendering.md)。

## 资源与流式音频补充（2026-10-31）

本轮未增加原 EXE 逆向，重构的是已有资源与音频宿主实现。新增只读范围源与持久归档描述符，普通音源、分段流、历史/小游戏和 CD 虚拟轨道均由后台线程增量解码，每源 PCM 队列固定 64 KiB。Audio v6 保存消费位置，继续读取旧版；跨不同采样率片段的旧游标已与整段转换参考逐样本比较。图像新增 64 项 / 估算 128 MiB LRU 与延迟载荷读取，YKG 元数据读取不再复制图像平面。

50 项 CTest、ASan/UBSan、ThreadSanitizer 通过；一小时稀疏音源验证不会全量读入或分配完整 PCM。真实 Metal/CoreAudio 标题（静音、正常时钟）18 源 PCM 队列共 1,179,648 字节、underruns=0；Metal 剧情、历史、菜单和跨进程存读档回归通过。完整架构及限制见 [资源与音频管线](resource-streaming.md)。
