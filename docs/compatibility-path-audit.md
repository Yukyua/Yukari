# 兼容性路径审计：标题以后的实际缺口

> 历史审计快照：下文记录本轮指令实现前的缺口，不代表当前状态。诸如颜色、选择、影片、Ruby、音频与已读标记等条目已陆续实现；当前状态以 [兼容性审计](compatibility-audit.md)、[指令契约](instruction-contracts.md) 和各功能文档为准。


日期：2026-10-31。仅原版EXE与data01..05.ykc；不分析汉化程序或补丁。本文是只读源码/脚本/既有静态证据审计，不把短程Yukari测试视为完整游戏运行证明。

## 扫描口径

按原版资源优先序data04→01→02→03→05检查全部571个唯一YKS，解码函数调用、实参及脚本跳转。得到824,603处调用、157种函数；原EXE分发表有357项。其他200项没有在本游戏静态YKS中出现，不等于200项都应实现或都已实现。

可复核中间数据位于忽略目录 `analysis/original-engine/compatibility/script-calls.json` 与 `summary.json`。调用图只提取常量文件名的ScriptCall/Jump/SubCall，不代表分支一定执行，也不完整覆盖变量拼接的动态脚本跳转。word均为YKS代码区DWORD偏移。

当前源码存在18个真实脚本函数名未被runtime/vm识别字符串覆盖：SelectMode、SelectPosition、Select、SelectTTE、TTE、MovieVolumeSet、MoviePlay、ScriptSubCall、TimerStart、TimerGet、TimerWait、GraphicAddColor、GraphicSubColor、LF、GraphicFilter、GraphicSepia、GraphicMonochrome、Restart。此名单是静态筛查，仍需逐路径验证；“登记为configuration”也不能算完成语义。

## 默认NEW的阻断顺序

| 可达链 | 具体命令 | 影响与证据类型 |
|---|---|---|
| title NEW→root_common→prologue01 | 本体没有上述18项 | 已能进入正文和有限页推进；不证明其全部动画/排版准确 |
| root_common:0x30→prologue02 | GraphicAddColor，首次0xC2B | 下一章较早触及未实现色调；全资源共有3303次，不能当罕见特效忽略。脚本+宿主源码 |
| 完成prologue02→root_common:0x38→etc/playMovie | MovieVolumeSet(GlobalFlag:100)在0x5，MoviePlay在0x8 | 影片前先在音量命令停止；MoviePlay路径是avi/OriginalGame.mpg。脚本+目录观察 |
| GlobalFlag:150已置1后再次NEW→etc/prlogue_skip:0x35→function/selectA | SelectMode在0x0；Select/Case在0x57 | 已看序章的玩家连“是否跳过序章”选择都不能完成，独立于影片。脚本+宿主源码 |
| 跳过序章→com01 | GraphicAddColor在0x52DD | 即使绕过影片仍不能认定共通线完整 |
| com01:0x7A05→etc/select_gt→右键保存 | ScriptSubCall在0xA2 | 正常地图/选择页保存要用子调用快照协议，不是普通ScriptCall |

本轮已用当前 Yukari 从已有稳定页快照继续运行，实际在 `prologue02:word=0xC2B` 报 `unimplemented engine function: GraphicAddColor` 并以 exit 1 退出。调用栈为 start:0x9D → title_main:0x297 → root_common:0x30 → prologue02:0xC2B。输出保存在 `analysis/compatibility/prologue02-probe.stdout.txt` 和 `prologue02-probe.stderr.txt`。因此这是动态复现的阻断；后续影片等位置仍属静态可达路径分析。

prologue02完成后的GlobalFlag:150赋值位于影片调用之后；不能假设影片未实现时用户自然能解锁跳过序章。

Movie不是“缺一个AVI文件”：目录中实际有约206MB的`avi/game.mpg`，而原脚本也明确播放MPG。当前音频libsndfile/SDL图层服务不能代替视频解码、帧时钟、音画同步、取消输入及播放完成恢复。不能只检测文件存在后无操作返回。

## 选择、回选择和ScriptSubCall

文本选择与标题MouseSelect属于不同服务。`function/selectA.yks` 用SelectMode设颜色、SelectPosition设绘制层/版式，再由Select收集Case(text,x,y,...矩形...)。宿主目前只有MouseSelect收集入口；VM识别Case名称不代表能创建文字选择界面。另一族`direction/mail/selectMail.yks`同样使用Select，在0x58/0xB2有2选/3选块。

限时选择`function/select_rtc.yks`还需要SelectTTE(Flag:128)（0x168）与4个TTE分支（首次0x231）。35个love分支脚本静态调用该文件，例如`story/common/love/com01_ls_ai1.yks:0x5B1`。因此限时选择缺口可从正常共通恋爱事件到达。

12处ScriptSubCall全部指向`system/option/m_save.yks`，分布于select_gt、map、love_02/03/04/06/07/08。原版VM文档已有静态证据：`Yuka_ScriptSubCall`先保存`savedata.---`，再ScriptCall，若上下文改变则设新frame+0xC9=1（REA `ev_47f780c8fa8701b457c63ca0c01c351e0b68a2fbefa93ca680fba79f5566e958`）。普通call无法完整替代这条保存路径。

原版MouseSelect还会自动建立选择点快照：`title/MouseSelect.c.txt`，REA `ev_893061c4e82f929a95fca79821745f801ced476f0cc2d4700151b3d181b173d0`，开关DAT_005587D0开启且非subcall上下文时先`FUN_00442fd0(0,ctx)`和保存`savedata.---`，再进入选择。当前runtime MouseSelect只登记regions，尚没有这些自动保存动作。原生slot容器能写和slot0“在正确时刻自动产生”是两个要求。

`m_YesNo`的回此前选择分支调用SaveDataCheck(0)/SaveDataLoad(0)；`function/NextJump.yks`通过Flag:198选择后续快照槽并保存/恢复相关Flag。`select_gt`和地图脚本显式写slot998，不替代slot0。未创建自动选择快照时，“回选择”按钮可能只是因为缺档直接返回，不能视为已完成回退。

## Already.dat与既读判定

当前AlreadySkipModeSet/Check仅维护布尔mode，源码未见按脚本/代码位置记录已读的集合，也未读写Already.dat。输入事件底栏跳过/F2直接切skip_mode；这会使“只跳过既读”退化为普通跳过。

真实`createMenu.yks`根据GlobalFlag:3把底栏注册为AreaSetAlreadySkip或AreaSetSkip；`system/com/AlreadySkip.yks`只有AlreadySkipModeSet()。没有脚本AlreadyDataWrite调用并不表示不需要持久化：解释器/退出清理也可能负责写Already.dat。原版分发表有TextAlreadyColor、AlreadyNoWaitModeSet/Get、IconSetAlready、MouseCase2Already等完整族。本轮已取得启动/退出调用401AE0/4010C0的直接静态证据，schema与更新粒度补证见下文；它与System.dat、剧情存档是独立格式。

建议验证标准：未读页不得因既读skip自动跳过；读过页重开后应可跳过；不同脚本/word地址不能相互污染；严格保持下文原版“执行前读旧值再置1”的顺序；重启进程保留判定。只有布尔开关测试无法覆盖这些要求。

## CG、回想、音乐与解锁状态

OMAKE默认由GlobalFlag:200等进度开关控制，不应在新状态下强行显示。已解锁后真实路径为title_main→title_omake，再到extra/cg/cg_main（CG/回想/音乐）或extra/ls/ls_main（恋爱事件重播）。这些界面使用大量现有MouseCase2/SpriteCreate/GlobalFlag条件，入口画面可能能显示，但还未据此验证浏览全集。

- cg_main:0xAA保存slot998，然后建立页面/缩略图/音乐按钮。0x6A4调用view_cg；后者有585次GraphicLoad/Transition与585次cg_cancel调用，负责差分浏览。
- cg_main:0x6BB跳play_H；play_H静态调用24个剧情回想文件，结束清主画面后返回cg_main。此处需要正确处理Flag:280/282模式、slot998快照及正文菜单保存特殊路径。
- ls_main:0x8F保存slot998，0x27C跳play_story；play_story直接关联93个不同子脚本，覆盖恋爱事件和清理函数，而非单张静态预览。
- title_select在0x11E调用Restart(1)；m_YesNo返回标题确认也在0x288调用Restart(1)。当前未实现Restart，正常确认“返回标题”可直接硬停。Esc/关闭窗口退出不等于重启引擎状态。

这些路径有真实脚本证据，但没有Windows原版运行对照，也没有Yukari全分支回想测试。尤需区分GlobalFlag解锁、普通Flag临时界面状态、GlobalString和原版Already数据；读某个原存档不应把它们混为一份slot快照。

## 选项显示与行为完成度

已经有实际实现的部分包括音量组倍率、声音淡变、文字速度、字体描边/阴影选择、自动模式基础等待、全屏切换、保存注释输入、历史基本浏览/语音。它们仍未达到所有原版配置服务等价。

当前需进一步核对/实现：

- AutoModeCharactorWaitSet只登记配置，自动播放倒计时仍主要取AutoSpeedSet，没有逐字符/语音时长完整语义。
- CursorAutoHide、KeyWaitFunctionSet、SelectFunctionSet、MouseSelectFunctionSet等只登记，尚不能证明对应策略生效。
- KeySetScript允许脚本指定任意按键序列，当前SDL事件仍硬编码F1..F7等；createScKey登记的F8/B回选择、F9/T回标题和F键方向函数未完整接入。KeyTableCase秘密序列也未执行。
- 文字底栏部分区域硬编码；原版AreaSet注册的启用条件/父层显隐、历史区域滚动条/页大小、Indicate tooltip等需统一消费配置，而非只存参数。
- GraphicLoad五参数额外mask未被runtime传至Scene，虽然原版scene存档能保留mask路径，读取格式和宿主命令处理仍有差异；m_game背景预览实际使用此形式。
- UserTransition加载效果资源后统一淡化，原版mode与遮罩算法仍属近似；游戏设置TransitionModeSet即使登记，也不能证明已经改变渲染策略。
- LF共有4272次，GraphicAddColor/SubColor共3995次；后期另有Filter/Sepia/Monochrome及片尾TimerStart/Get/Wait。现有短程剧情测试未覆盖它们。

## 建议下一轮可复核测试路径

1. 直接用原生VM快照定位到prologue02首次GraphicAddColor前，执行并检查像素变化；随后完整扫到影片调用，不能以跳过异常服务代替通过。
2. 显式构建GlobalFlag:150=1再从NEW进入序章选择，检查Select二选项绘制、鼠标/键盘、选项返回值、slot0快照以及重启后既读判定。
3. 在com01选择页右键保存，经过ScriptSubCall保存后返回同选择；读取该档仍停在同一选择，正文和返回链都不被m_save污染。
4. 对限时选择分别测试超时、提前选择、菜单/保存暂停计时，验证TTE分支和timer恢复。
5. 在解锁状态下完成CG页切换→差分浏览→返回，回想播放→返回，音乐播放/停止；保存slot998并恢复时检查上下文/音频/界面临时Flag。
6. 每个设置控件以“脚本GlobalFlag改变+实际行为改变+跨进程持久化”三者验证，不能只比较按钮高亮。

这些是覆盖路线，不代表当前已执行完成。第一优先级是稳定阻断NEW后正常路径的GraphicAddColor/文本选择/影片和Restart；已读记录、自动选择快照及SubCall应作为完整行为一起补齐。

## 新增原版静态补证

Already.dat的读取器0x401AE0（REA `ev_a2b4d1218dc65ea7e358b7fb29c67925ebc7866e1a609339a228516cfa5d6c47`）、写入器0x4010C0（REA `ev_6443bbad730a4739c78becbb323f6b1e1259a3bf0e7037de41578d2d5f405a21`）确认：magic为`YKALREADY01`，头20字节，+0xC记录数、+0x10索引offset；每条索引16字节，依次为脚本名offset、脚本名length、已读map字节数、map offset。原目录Already.dat有547条，全部map长度等于同名原始YKS代码DWORD数，现有字节值仅0/1。

更早保留的解释器证据已确认其更新粒度：`function-dispatch-loop.c.txt`，REA `ev_8b3543742f0d919a75763ab0a0042211c8ab90e9cfe3066cafbf623425364390`，每条指令执行前从frame+0xCC指向的map[current word]读取旧值到ctx+0x80111，再把该字节设1，且检查PC位于0..codeword_count。因此它是**每代码word的字节标记**，不是每页/字符的bitmap。脚本载入0x43CF20（REA `ev_9e2eb413b5af36328a082ac3345ea75f4dc7fd1872fe0bba8ba0b5a0b36e6eb4`）使用脚本名、代码word数和frame+0xCC调用0x4019D0绑定该map。后续实现应保留“先读旧值，再标记本次执行”顺序。

Select wrapper 0x44C590（REA `ev_4d0878888f404f8452d6a5df9df1c4a06cc83bb61fa077ee46f256440dc82f57`）设置ctx+0x80118=1并进入共同选择核心0x4480C0。保留的`seeded/scheduler-full.c.txt`开头显示该共同核心也在非subcall时调用0x442FD0(0,ctx)，所以前选择快照不仅是MouseSelect的需求。该旧文件未携带内嵌Evidence ID，此处以保留函数文件定位，不给它虚构ID。

MoviePlay 0x43A850（REA `ev_d9bc5604bff2fa762c6b25edf7bf12388e5c61bfbd6f40a24dbea737e3217287`）设置播放全局标志，调用0x4126D0，返回后清标志并恢复图层/窗口。0x4126D0（REA `ev_b51361e371e25a328957ace3de8ac22affa4397f8c87f50b613068212a072cdf`）发送开始0x8008/结束0x8009窗口消息，暂存恢复窗口状态和播放矩形，再调用更深层0x4122A0。此证据支持同步播放生命周期需求；底层解码器、取消键和音量曲线本轮仍未完整逆向。

LF 0x433E10（REA `ev_862b67f937cb6c5801cc0a0be55d6bc273b4fec653e9f818bbd0a2e0f0ff1532`）可以零参作用于活动文本层，也可显式传图层ID；它查图层后调用0x4166F0。不能把所有LF参数忽略或当作清页PF；更深行距/绘制游标语义需按该helper补证。

本轮已回答：真实脚本覆盖与硬阻断位置、影片实际载体、Already.dat格式/执行更新粒度、SubCall和选择点快照需求。部分回答：CG/回想可达路径与选项消费完整性。未回答：原版视频底层解码/取消协议、所有图像滤镜像素算法、全回想/限时选择Windows对照运行。
