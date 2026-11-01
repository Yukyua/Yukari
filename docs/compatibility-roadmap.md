# Yukari 原引擎兼容性缺口

> 历史审计快照：下文记录本轮指令实现前的缺口，不代表当前状态。诸如颜色、选择、影片、Ruby、音频与已读标记等条目已陆续实现；当前状态以 [兼容性审计](compatibility-audit.md)、[指令契约](instruction-contracts.md) 和各功能文档为准。


审计日期：2026-10-31。只分析原版 game.exe 与原版资源；未分析汉化程序、CNP 或补丁 DLL。本轮只新增审计工具、报告和静态证据，不实现新服务。

## 结论

Yukari 已能启动标题、推进早期剧情、操作部分菜单、存读档，并使用 SDL Metal 做 GPU 合成。但还不能完整运行本作：本轮继续运行稳定剧情快照，实际在 **prologue02 第 0xC2B（3115）个代码 word 的 GraphicAddColor** 报未实现并退出。影片、文本选择和后续剧情还有独立阻断。标题通过、早期剧情通过都不是通关兼容性证明。

## 全量扫描

使用实际 C++ YKC/YKS 解析器扫描全部 571 个脚本，得到 824,603 个静态调用位置、157 个使用到的原版函数。静态调用次数包含未执行分支，不代表运行频率。

| 当前处理方式 | 原版357个分发表项 | 本作使用的函数数 | 含义 |
|---|---:|---:|---|
| 有具体处理分支 | 98 | 84 | 部分仍为近似或子集实现 |
| 只登记配置 | 48 | 47 | 接受参数，但没有对应行为消费者 |
| Case 跳过/收集 | 25 | 8 | 不代表完整选择和事件调度 |
| 无处理 | 186 | 18 | 执行到这些函数会报错 |

不能用这些数量计算“兼容百分比”。一个只出现一次的 MoviePlay 就能阻断主线；原版另外200个本作未调用的函数也不都需要立即补齐。

## 优先缺口

| 优先级 | 模块 | 尚缺内容及影响 |
|---|---|---|
| P0 | 正常剧情命令 | GraphicAddColor/SubColor、LF；另有 GraphicFilter/Sepia/Monochrome。AddColor 已动态复现阻断；其他大量出现在后续路线。 |
| P0 | 文本/限时选择 | SelectMode、SelectPosition、Select、SelectTTE、TTE 与完整 Case 调度；已看序章后 NEW 的“跳过序章”选择也依赖它们。 |
| P0 | 影片与片尾 | MovieVolumeSet/MoviePlay；本作实际播放 avi/game.mpg。缺解码、音画同步、完成/取消与场景恢复；片尾另缺 TimerStart/Get/Wait。 |
| P0 | 菜单流程 | Restart 缺失，返回标题确认会失败；ScriptSubCall 缺失，正常选择/地图页的右键保存受影响。 |
| P1 | 回选择与已读 | 未自动维护选择前 slot0/savedata.--- 和嵌套前选择状态；没有 Already.dat 的逐代码 word 已读映射，既读跳过尚不能区分未读正文。 |
| P1 | 图形语义 | Transition/UserTransition 未执行原版 mode、mask、可选矩形；六类 Effect 仍简化；Sprite action1/2、零等待控制帧、反向初始化未完整实现；外部 mask 与移动打断有遗漏。GPU 合成本身已实现。 |
| P1 | 文字与设置 | NWC 居中、TextWindowSet 活动层/短参存在明确差异；ruby、字体切换、控制码、等待变体不完整。大量自动、历史、快捷键、区域和禁用配置仅登记。 |
| P1 | 存读档与历史 | 原生结构可读写，但逐字属性、图层附加状态、音频游标等未完整还原；配对原生档的复制/注释不同步，读档清理不完整。历史布局与事件仍有硬编码。Windows 原版读取 Yukari 新写档尚未实测。 |
| P2 | 资源和音频完整性 | SoundLoadStream 仍整段解码；淡变/音量规则尚未原版输出对照。其余未使用原版指令按实际需求取证实现。 |

18 个硬缺口完整名单：`LF`、`GraphicAddColor`、`GraphicSubColor`、`ScriptSubCall`、`TimerWait`、`GraphicSepia`、`TimerStart`、`TimerGet`、`SelectPosition`、`Select`、`TTE`、`SelectMode`、`GraphicFilter`、`Restart`、`GraphicMonochrome`、`SelectTTE`、`MoviePlay`、`MovieVolumeSet`。

## 已确认和未验证的边界

- 本轮动态验证只新增了 prologue02 的首个报错复现。没有完整跑完共通线、角色线、CG、回想或片尾。
- 原版静态代码确认 Already.dat 按脚本代码 DWORD 数分配字节图；执行前先读取旧值，再把当前 word 标1。不能用简单页面集合替代。
- 已有 native 存档容器往返、Yukari 加载和推进验证，不能推导 Windows 原版能接受所有新写档或精确恢复所有状态。
- 原版 mask 混色、部分 Effect、GDI 排版和视频底层算法仍需更深取证或原版运行对照，不把当前可工作规则称为完全等价。

下一轮建议先补正常主线的颜色/LF 命令并持续推进，再补完整选择及快照链、影片/片尾和返回标题；随后统一处理设置消费者与视觉/存档语义。每一阶段以实际路线跑通为验收，不能将未实现服务无操作返回当作通过。

## 可复核资料

- [完整指令清单与扫描方法](compatibility-audit.md)：18个缺口的频次、脚本和地址，以及48个只登记配置项。
- [真实脚本路径与原版证据](compatibility-path-audit.md)：NEW、影片、选择、回想、Already.dat 和动态报错。
- [已有处理的语义差异](compatibility-semantics-audit.md)：图形、文字、音频、输入和存档逐项依据。
- 机器可读矩阵：`analysis/compatibility/dispatch-coverage.json`；复现命令：`python3 tools/audit_compatibility.py --game-dir game`。
