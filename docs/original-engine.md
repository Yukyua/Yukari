# 参考样本 原版引擎逆向

日期：2026-10-31。目标是为 Yukari 提供可复核的原版行为依据。

本报告主体保留首次逆向时的实施状态。后续已完成基础 VM，最新实现、验证与边界见 [VM 文档](vm.md) 及 [标题运行时](title-runtime.md)。

## 范围与身份

唯一程序目标：`game/game.exe`。

- SHA-256：`08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267`
- PE32，x86，映像基址 `0x400000`，入口 VA `0x4674F6`。
- 原版资源范围：`data01.ykc` 至 `data05.ykc`；详情见 [资源审计](original-resources.md)。
- 不读取或分析 `game_chs.exe`、CNP 和补丁 DLL。原始 EXE 与资源均不修改，也没有运行 Windows 游戏。

使用 REA 6.4.0 与本机已有的 Ghidra 12.1.4 做静态分析。Ghidra 工程使用临时目录，详细 Evidence 保存在本地 `analysis/original-engine/`，不纳入版本控制。默认分析没有识别出部分间接调用函数，后续以从原文件读取的处理器地址作为 function seeds；种子只是分析提示，不等同于已证实语义。

可复现原文件 PE 元数据和候选分发表：

```sh
python3 tools/inspect_original.py game/game.exe /tmp/game-pe.json
```

`tools/inspect_original.py` 的结果是普通文件字节观察；只有下述 REA 结果才带 Evidence ID。

## 已确认：函数分发

原文件 `.data` 中从 VA `0x4BC3E0` 开始有 357 个连续记录，每项 12 字节：名称指针、处理函数指针、标志 DWORD。读取器输出原文件偏移、VA 与处理器 VA，可与反汇编对照。571 脚本中出现的全部 157 个函数名均在表中。

这张表的用途已通过原版解释器代码确认：

- `0x42F6B0` 遍历名称表，以逐字节字符串比较判断名称是否存在。
- 解释器循环 `0x4468D0` 在 Func 元素的第四 DWORD 缓存 `匹配表项 + 1`；函数名按区分大小写的字节比较查找。
- 它从代码流读取参数个数及参数元素索引，将参数转换为指针，再经 `0x4BC3E4 + index * 12` 间接调用处理器。
- 调用前默认 next word 设置为 `current word + argument count + 2`；非函数元素通常推进一 word。当前 word 与 next word 分别在脚本上下文 `+0x0C`、`+0x10`。
- 函数返回值可被解释器写回此前选择的目标元素；因此赋值不是简单地把目标元素当成可忽略的标记。
- 解释器会将本地变量元素在运行中改为类型 `0x0B` / `0x0C`。磁盘样本未出现这两种类型，Yukari 读取器目前仍拒绝它们；后续 VM 必须区分磁盘结构和运行时类型。

表项对应地址：

| 名称 | 表项 VA | 处理器 VA | 标志原始值 |
|---|---|---|---:|
| StrOut | `0x4BC3E0` | `0x445ED0` | 0 |
| = | `0x4BC410` | `0x43E940` | 1 |
| if | `0x4BC428` | `0x438720` | 1 |
| GraphicLoad | `0x4BC500` | `0x43D8C0` | 1 |
| ScriptCall | `0x4BC6D4` | `0x441A00` | 1 |
| ScriptJump | `0x4BC6EC` | `0x441CC0` | 0 |
| Return | `0x4BC8D8` | `0x437A50` | 0 |

标志被调度相关分支读取，但尚不把它命名为“阻塞/异步”之类完整语义。

Evidence：

- 概览（默认分析 2,298 个过程）：`ev_488a0d7bc4983b80bdb91096e0414a3abec5c98deee9680c0b2b40fdbd5bd8f7`
- 表入口交叉引用：`ev_a6fba2d8de5a08e0ea93c18e342ba89ae6a7d9a6019010e77de69c192b2c0e4c`
- `0x42F6B0`：`ev_12d3e025bee3f6263b7f93cbf52dab5838db56c7d3573dd001c24648c453c920`
- `0x4468D0`：`ev_8b3543742f0d919a75763ab0a0042211c8ab90e9cfe3066cafbf623425364390`

## 文字与启动边界

原文件 PE 导入表包含 GDI32 的 `CreateFontIndirectA`、`TextOutA`、`GetTextExtentPoint32A`。这支持 GDI ANSI 字体与文字路径的存在，但导入本身不能证明全部画面文字都经过同一函数。仍需逐个追踪字体创建、测量、逐字显示、换行与描边路径。

原始 `start.yks` 的调用链已确认，画布配置为 1024×576。脚本引用的 `OptionSystem.yks` 等名称与包目录中的小写名称不一致，现有精确匹配会失败。**函数名的比较规则不等于资源名比较规则**，资源查找规则随后已从下述原版读取路径确认。

## 未决问题

1. 额外自定义包、包外文件回退、非 ASCII 文件名和分隔符处理的完整行为。
2. ScriptCall / ScriptJump / Return 的完整调度与跨脚本返回值传播；已恢复的处理器行为见下文。
3. `=` 的运算符结合顺序、类型转换、溢出与除零；if/循环/函数声明的控制链接。
4. 图层、独立 Alpha、动画动作和调度时序。
5. 字体/GDI 布局、音频、输入、选择项、存档与视频。

当前结果是静态原版证据与资源兼容验证，尚未构成可玩的游戏运行时。


## 已确认并实现：原版资源搜索

`0x43CF20` 加载脚本时先调用 `0x42A2F0` 查包，未命中再通过 `CreateFileA` 读取包外文件。加载器按 YKS 头复制代码/索引/数据段、清空 Func 元素的缓存字段，且在加密标志为 1 时仅 XOR 声明的数据区；这与 Yukari 现有解析方式一致。

`0x42A2F0` 从 `0x4BC228` 顺序读取 archive 对象指针，首个命中立即返回。`0x429400` 初始化对象文件名。已映射的前九项顺序为：

`data04 → data01 → data02 → data03 → data05 → data06 → data07 → data08 → data09`

指针表还有第十个对象 `0x557900`，其自定义名称来源尚未调查；不将未知对象猜为补丁包。当前样本只有五个原始 YKC；Yukari 的 参考样本 资源配置挂载上述已知名称中实际存在的包，不读 CNP/汉化程序，不实现未知第十包或包外回退。

单包读取函数 `0x42A190` 从索引首项开始线性扫描，通过 `__stricmp` 比较名称，首个匹配条目胜出。Yukari 改为首项优先的哈希索引并忽略 ASCII 大小写，保持分隔符原样。本次五包全部 24,743 个名称均为 ASCII；非 ASCII 名称与 CRT locale 的精确对应仍未声称兼容。

对应代码在 `src/formats.cpp` 的 `Archive::read` 和 `Resources`。新增 `game-script` / `game-view` 命令读取原始游戏目录。混合大小写的 OptionSystem / createText / TITLE_MAIN 引用已验证通过。

Evidence：

- `0x43CF20` 脚本加载：`ev_9e2eb413b5af36328a082ac3345ea75f4dc7fd1872fe0bba8ba0b5a0b36e6eb4`
- `0x42A2F0` 跨包查找：`ev_fafdcf1e109293e1dc46c6ee262d811bc326f0044e66bc9a1c7a89c664f09e82`
- `0x42A190` 单包读取：`ev_a9fba44e1c0d0b9eabd3a5c9d2d1f2bb0763378a0df69a2e5e2178085183557d`
- `0x429400` 初始化：`ev_baead8b173356d77a1f605e76045ee7d750a6578bf20e1da2ce8e644b4bd7a6a`
- 原始指针表字节与默认分析的完整源身份均保存在本地 Evidence，原文件字节也可用 `inspect_original.py` 复核。

## 已恢复的控制处理器（尚未实现 VM）

函数种子来自分发表的 325 个不同处理地址。Ghidra 报告 325 个函数创建成功，零失败；`Yuka_` 前缀名称是分析者提示，不是恢复出的原始符号。

- `ScriptCall`：单个 Ctrl 参数时先创建同一文件的子脚本上下文，再按标签名查找其链接并设置子上下文 PC/next；字符串参数则加载指定脚本。双参数形式加载文件并定位标签。标签比较为区分大小写的逐字节比较。
- `ScriptJump`：单个 Ctrl 参数直接设置当前 next word 为链接；字符串参数通过同一子上下文加载入口，随后将原上下文 next 设为 code_count，结束原上下文。双参数形式加载文件并定位标签后结束原上下文。
- `Return`：将 next word 设为 code_count；若一个参数，则生成携带整数的返回对象。退出上下文与上层传播需要调度器配合，不能直接实现成“PC 从一个普通栈弹出”。
- `if`：首个整数参数为零，且 next 指向 `{` 控制项时，将 next 设置为该控制项的数据区链接。这里是链接本身，不擅自加一。
- `=`：单参数时按实参类型返回整数或复制后的字符串，目标写回在解释器中完成。多参数运算器已定位，但结合顺序、全部运算符与错误行为还需继续核对。
- `0x432AD0` 新建并链接 0x120 字节脚本上下文，`0x43F140` 调用它后交由 `0x43CF20` 加载脚本。这是“上下文链”的代码证据，尚未证明全部调度生命周期。

Evidence：ScriptCall `ev_324488c38d9359178a8f328ceb284878433eb4a8276d1eb2b858d412df75544b`；ScriptJump `ev_fe61911876b640445dc5da423bc9095da75d98f26497c09db24ec292338d1fcd`；Return `ev_edb0688adfe1485d2a6ca1750a728b0ef28a0ce92af9f5275cef0b7f8dce6c98`；if `ev_0870e7e85c0ba43ad838f62ab60407f65e9376f6ad7384e2ce2d26b9f3c9da4a`；赋值 `ev_5b22d6e73f78ac940909aa107053481d41bec437ca7b7fb0fe668b55ad25642f`；上下文创建 `ev_ade4ec7942059f0e6202f96a7ea1ab72540366432dc10279de4d1103b8d96263`。

`0x4480C0` 的 dossier 完整结果超过 MCP 传输预算；使用 retained Evidence `ev_a05f1b0f8e407e8ec516649928fedcdf564475ae858f4063fed17c6e8684dbde` 读取了伪代码前 40,000 / 133,874 字符，其余没有在本轮阅读。完整 session Evidence 已导出到本地 `analysis/original-engine/seeded/session-evidence.json`。该过程的原生 value-flow 被工具标记为截断；不据此声称已还原调度器。

## 本轮验证与覆盖

- Debug 与 ASan/UBSan 构建、12 组合成测试通过，新增测试覆盖大小写、重名首项、data04 优先级及不挂载 CNP。
- 真实五包目录和 571 脚本全量验证；8,515 YKG 结构全量验证。
- 5 个真实图像样本经过 GNP/PNG 解码与 SDL dummy/software 单帧呈现；包含背景、图集、独立 Alpha 的颜色平面、标题背景与标题文字。
- 原生 macOS `game-view ... title_bace.ykg --once` 成功，输出 `Preview 1024x576`。这是标题背景的图像预览，并非脚本驱动的标题界面。
- `tests/original_game.py` 为显式 opt-in 样本检查，不携带游戏文件。

| 调查项 | 状态 |
|---|---|
| 原版程序身份与范围 | 已确认 |
| 原始包、脚本/图像格式读取 | 已验证本样本 |
| 分发表与函数名覆盖 | 已确认 |
| 本样本资源大小写与已知包优先级 | 已实现并验证 |
| 基础控制处理器与上下文 | 部分恢复，VM 未实现 |
| 完整标题/剧情/选择/存档/音视频 | 未实现，未验证 |
