# 已接入服务的语义兼容审计

> 历史审计快照：下文记录本轮指令实现前的缺口，不代表当前状态。诸如颜色、选择、影片、Ruby、音频与已读标记等条目已陆续实现；当前状态以 [兼容性审计](compatibility-audit.md)、[指令契约](instruction-contracts.md) 和各功能文档为准。


审计日期：2026-10-31。范围是 `src/runtime.cpp`、`src/render.cpp`、`src/audio.cpp` 中已经返回成功或已有可见行为的路径；本文不重复函数覆盖数量，也不修改实现。目标仅为原版 `game.exe` 与原版资源。未执行 Windows 原版，未将汉化程序、字体或补丁作为行为证据。

这里的“已确认差异”指当前 C++ 与已保留原版静态代码或真实脚本契约存在直接差别；“未补证”指实现采用了可工作的移植规则，但现有证据不足以证明等价。行号按本轮审计时的源码，后续修改应按函数名重新定位。

GPU 路径已经实现并有 Metal 测试，不能再将这些问题描述成“尚未 GPU 渲染”。剩余问题主要在命令语义、原版像素算法和状态还原。GPU 性能/路径依据见 [GPU 绘制](gpu-rendering.md)。

## 原版依据索引

| 编号 | 保留的直接证据 | 能支持的结论 |
|---|---|---|
| E1 | [UserTransition](../analysis/original-engine/title/UserTransition.c.txt)，`ev_00d0076c836e8ccc99ddeac48596b7664d004d092d4829fd5d0a6a2cc51b8cd5`；[Transition](../analysis/original-engine/title/Transition.c.txt)，`ev_7a5a8657d96b0b3f2902b08daa123a6f8582608402a8be5e83c13bad64f36f6c` | 原版传递 mode、mask、duration 和可选 RECT；UserTransition 两参数形式默认 1000。wrapper 本身不能证明最终像素公式。 |
| E2 | [Effect](../analysis/original-engine/story/Effect.json)，`ev_db615af035e318c24fccdc4a46a44717f0fbf1361de1b4af6775b8b5c20b880f`；[EffectApply](../analysis/original-engine/story/EffectApply.json)，`ev_359aba9efd355573a802098dcbfe5bf3928816ceb5be26ba36118adf8b73edfd`；[Shake1](../analysis/original-engine/story/EffectShake1.json)、[Shake5](../analysis/original-engine/story/EffectShake5.json) | effect 1..6 分派到不同函数；1 为四步正负纵移，5 为十二步具体坐标表；还有全局关闭与加速分支。 |
| E3 | [图层/动画证据](title-graphics-semantics.md)，尤其 `0x40A390 / ev_97069aa329461fd0662fa67b268f7ed03cc543bcc146106e53d209b85364135f`、`0x40A4F0 / ev_54711c85963b6cab8d0bc954d29b99305b9b10a631697f449cb67962ea39d916` | action 1 跳帧，action 2 换组；零等待控制帧连续处理；反向单次从末帧初始化。 |
| E4 | [GraphicLoadHide](../analysis/original-engine/title/GraphicLoadHide.c.txt)；[GraphicLoad 和图层内部函数](title-graphics-semantics.md) | 三/五参数载入额外 mask；居中依赖载入器资源前缀规则；显式 GraphicMove 会停止滚动。 |
| E5 | [StrOutNWC](../analysis/original-engine/story/StrOutNWC.json)，`ev_733ee792b936eab42ed0660d77fe3e49d35b5aa8d6d1dfb7f6dc371ca8108e6c`；[StrOut](../analysis/original-engine/story/StrOut.json)，`ev_20274ab56cfded6cb18fafa2994deb9c0684885640a82e2c5f2e7990996647ef` | NWC 先测量宽度并改写居中起点，再以速度 0 调用 StrOut；普通 StrOut 维护逐参数输出进度并调用内部逐字处理器。 |
| E6 | [TextWindowSet](../analysis/original-engine/story/TextWindow.json)，`ev_3b0f08ebe453040a0b5242590f4d15c4cf34291470ef922b8788f9ea95335c4e`；[PF](../analysis/original-engine/story/PF.json)，`ev_6ba6ac996dc9091662a96e55604ee85c7f644e6f7e77fd835f45687ed7235518` | TextWindowSet 首先选择活动文本层，超过四参才设置 RECT；PF 先收历史，再清指定层，`-1` 另清声音关联状态。 |
| E7 | [HistoryCollect](../analysis/original-engine/story/HistoryCollect.json)，`ev_84b3bf2611c2f5bf99dd3f93d75a03771cdcc0fafb2cd159f98b9cfd045635f6` | 收集配置层集合；识别 kind 3/4/5/7、可见/已消费标志、相对坐标和声音关联，使用 1000 项环队列。 |
| E8 | [剧情/菜单/存档审计](story-menu-audit.md) 及其保存的 StateWriter、LayerWriter、FrameWriter、AudioWriter 证据 | 原生层、精灵、文字、声音记录包含比当前运行时更多的状态；特殊保存分支复制已有快照，嵌套前选择档来自 savedata.000。 |
| E9 | [原版脚本的真实调用与界面坐标](story-menu-audit.md)、[字体初始化参数](title-startup-audit.md) | 已观察到字体族切换、RubyFontSet、设置滑条、按键/区域注册、保存注释及前选择分支。仅脚本出现不能证明内部算法。 |

## 转场、图层和精灵

| 状态 | 服务与源码位置 | 现在实际做了什么 | 游戏影响与验证边界 |
|---|---|---|---|
| 已确认差异，优先处理 | `Transition / UserTransition`，`src/runtime.cpp:481`、`render():324` | mode 被忽略；UserTransition 只加载资源，未使用 mask 像素；一律将旧 GPU 快照线性淡出。额外 RECT 未使用。两参数 UserTransition 仍读第三参数，缺省形式可能越界。 | E1 有直接参数证据。标题最终画面通过不能验证转场中间帧；同一遮罩不同 mode 目前不会产生不同擦除形状。 |
| 部分已确认，部分未补证 | `Effect`，`src/runtime.cpp:420`、`render():316` | 1/2 是纵/横交替移动，其他全部使用同一十二步近似；时长按 type>=5 选择 12 步，否则 4 步。 | E2 确认原版有六种分派；原版 5 的位移幅度使用 amplitude/2，当前表使用 amplitude/4，且表项有差别。原版加速/关闭条件未重现；3/4/6 的精确算法仍需内部函数证据。GPU 冻结帧测试只验证复制和移动。 |
| 明确为推断 | YKG 独立 mask，`src/render.cpp:15` | 以 `255-average(RGB)` 作为 opacity，再与颜色 Alpha 相乘。 | 全白 damy mask 和透明父层布局支持极性；尚无完整原版混色公式。彩色/带自身 Alpha 的 mask、精度和滤镜组合未做原版像素对照。白/黑/灰合成单测验证的是这条实现规则。 |
| 已确认差异 | `GraphicLoad / GraphicLoadHide`，`src/runtime.cpp:457` | 三/五参中的额外 mask 字符串未载入；二/三参全部按 1024×576 居中。 | E4 的原版 loader 传递额外 mask，居中存在资源前缀条件。四参默认标题路径通过不能覆盖其他调用形式。 |
| 已确认差异 | `GraphicMove`，`src/runtime.cpp:464`，`tick():336` | 改位置但未移除该层已运行的 position motion，下一 tick 会继续按旧轨迹覆盖新坐标。 | E4 的 `FUN_00415BA0` 明确清滚动速度/模式。移动中的显式重定位会与原版不同；目前标题测试没覆盖这类打断。 |
| 已确认未完成 | `asset / apply_frame / tick`，`src/runtime.cpp:107`、`:150`、`:345` | 动画组只保留 w/h 非零记录；action 非零时报错；创建总从 frame0 开始，反向单次未按原版初始化到末帧；没有连续消化零等待控制帧。 | E3 已经确认 action1/2 与初始化规则。w/h 为零的控制帧还可能在进入 apply_frame 前被筛掉。启动路径 action0 通过不代表整部游戏图集可播放。 |
| 状态保留不等于显示支持 | 原生图层/精灵恢复，`src/runtime.cpp:200`；SDL 合成 `src/render.cpp:306` | 恢复资源、坐标、显隐、透过率；kind1 仅恢复 named 且 source_layer 存在者，kind3 被串回正文后重新排版。渲染只用 RenderCopy 的矩形缩放。 | E8 中的额外 mask、图层变换/颜色状态、无名或独立 payload 精灵、其他 kind、逐字位置/颜色/字体差异，并没有全部应用。未知 header 得到保留不等于对应视觉已生效。不得将 kind0 被跳过本身当作缺陷，原版 reader 也会跳过临时 hover kind0。 |
| 精度/同步边界 | `GraphicTransmittance / Scroll / Wait`，`src/runtime.cpp:465`、`:466`、`:611` | 透过率提前量化到 8 位 Alpha，坐标插值每帧截成整数；两类 Wait 都等同一层的 position 与 alpha 两种 motion 全结束。 | 原版保存 double 透过率/位置并线性推进（E4）；精确取整、并行滚动的等待边界尚未逐帧比对。已验证参数顺序与最终值，不宜据此宣称计时完全一致。 |

## 文本、设置和输入

| 状态 | 服务与源码位置 | 缺口与影响 | 验证边界 |
|---|---|---|---|
| 已确认差异 | `StrOutNWC`，`src/runtime.cpp:411` | 与 StrOutNW/NameOut 共用追加/左对齐路径，未执行 E5 明确的测量与居中。姓名窗、确认文字等需要居中的区域会偏位。 | 已看过的正文截图文字清晰；清晰不等于与原版对齐。 |
| 已确认差异 | `TextWindowSet`，`src/runtime.cpp:407` | 没有选择 `text_layer`，且无条件读取五参；原版单参可以只选活动层。后续单参 StrOut/PF 可能针对错误层，短参数形式会抛越界。 | 默认初始化恰好使用五参、默认层13；不足以验证接口变体。 |
| 已确认遗漏 + 移植近似 | `DefaultFontSet / OriginalFontSet / RubyFontSet`，`src/runtime.cpp:405`、`:382`；`TextRasterizer`，`src/render.cpp:110`、`:364` | DefaultFontSet 仅使用字号、部分行距/字距，忽略字体名和其他原版参数。OriginalFontSet/RubyFontSet 只登记。默认始终选系统日文 Gothic；MS明朝切换、ruby 排版未接入。 | E9 确认初始化会按选项选择 MS Gothic/Mincho；原版使用 GDI。FreeType 的 ascender、kerning、逐字符折行、方形膨胀描边不能视为 GDI 等价；禁则、竖排和 ruby 未验证。 |
| 已确认当前实现无效 | `TextShadowColor / draw_text`，`src/runtime.cpp:409`、`:301` | 文本层能记录 shadow_color，但存在 font 槽时用 base style 覆盖，只回填 color/border_color，导致配置的阴影色丢失。 | 黑阴影的默认视觉测试不会发现彩色阴影配置失效。 |
| 未补证的简化 | `StrOut / StrOutNW / NameOut / KeyWait* / PF`，`src/runtime.cpp:411`、`:418`、`:419` | 多种输出/等待族合并；只 StrOut 按 UTF-8 codepoint 重绘整个内容实现逐字。没有对应原版内部文本控制码解析器；继续追加时会从整个已有 content 的字节0重新 reveal。KeyWaitNA/TTE/TTELoop 与普通 KeyWait 同行为。PF 只处理当前 TextBox 内容，没有 E6 的 `-1` 声音关联清理。 | E5/E6 证明原版还有进度及内部处理；各变体精确语义仍需补证。文本单测覆盖 UTF-8/换行/折行/描边，不覆盖原版控制码、连串输出、等待族或 ruby。 |
| 可见设置未全部生效 | `registration`，`src/runtime.cpp:382`；`tick():335`、`:418` | 已读跳过与全跳过共用 `advance()`，未查“已读”标记；AutoModeCharactorWaitSet、SoundGroupAutomodeSkipSet 没有作用于自动等待，自动只用固定 AutoSpeedSet；CursorAutoHide、TransitionModeSet、SelectTransitionSet、KeyWaitIcon/PositionSet 等仅保存配置。 | E9 的设置菜单能点击/保存，不能证明这些开关已改变行为。尤其已读跳过可能直接越过未读正文。 |
| 配置解释不完整 | 键盘/区域/选择事件，`src/runtime.cpp:400`、`:568`、`:579` | 注册 KeySetScript/区域大部分参数后仍使用写死快捷键、底栏坐标和脚本路径；F8/B、F9/T 不能由注册表通用分派。KeyTableCase/WindowMessageCase 只收集，没有对应事件处理。未统一检查 SaveEnableMode/LoadEnableMode/SubMenuEnableMode、父层可见及区域启用条件。 | 默认标题鼠标 hover/click、剧情菜单流程通过；秘密按键、禁用状态与更换区域/键位配置未覆盖。 |
| 功能型替代，未证明等价 | `InputDialog`，`src/runtime.cpp:270`、`:426`、`:565` | 以固定位置/字号的 SDL 文本框替代原版对话框；忽略提示/窗口参数。实际输入按 Unicode codepoint 限长并检查编码；自动化 `input` action 直接 accept，绕过这两项检查。 | 26 的原版计数单位、IME composition、光标选区、原版返回/取消边界未补证。动作文件通过不等于真实 IME 已验证。 |

## 历史、音频与存读档恢复

| 状态 | 位置 | 缺口与游戏影响 | 验证边界 |
|---|---|---|---|
| 已确认不等价 | `collect_history / read_history`，`src/runtime.cpp:285`、`:292` | 收集只在当前正文层非空时触发；用 native_scene 重新生成 kind3，声音名取 String20。没有 E7 的配置层过滤、逐精灵消费标志和 kind4/5/7 表示。reader 保留原始记录但显示只解释 kind3，非文字历史元素会丢失。 | 1000 页环队列和 0x1C 结构有证据；字形位置/字体/颜色等结构读回不等于显示恢复。 |
| 硬编码替代 UI | `show_history / history_mode events`，`src/runtime.cpp:272`、`:566` | 每页固定四项、固定25px字号/颜色、固定128行高和按钮命中框，忽略 HistoryWindowSet/HistoryTextColor、AreaSetHistory*、HistorySound* 等配置；重播固定占声道99。 | 能打开/滚动/重播是移植功能；原版 HistoryToVoiceContinue、HistoryExitToNext、分页大小、滚动条/声音组规则尚未完整执行。 |
| 流式音频与资源范围 | `Audio::load_sources / ResourceSource` | 普通 sound 与 stream 均由后台线程增量解码，资源包内只按 entry 范围读取，每源 PCM 队列 64 KiB；保留两类原生循环语义，移除旧 256 MiB 整段缓冲限制。见 [流式管线](resource-streaming.md)。 | 已验证多采样率逐样本恢复、旧快照迁移、真实 OGG 与有界长音频；未据此声称原版调度延迟或所有循环接缝与 Windows 相同。 |
| 未补证的移植规则 | 音量/渐变，`src/runtime.cpp:332`、`:421`，`src/audio.cpp:114`、`:125` | 通道音量乘组倍率、重叠组以后配置者优先；线性 fade。SoundDeleteToFade 与 StopToFade 一样停止并保留载入缓冲，没有删除；原版音量曲线与重叠组规则未直接证明。 | 当前测试覆盖播放状态和设置操作；没有原版输出波形、重叠组与删除后再次播放的对比。 |
| 已确认信息丢失 | own runtime snapshot，`src/runtime.cpp:223`、`:235` | 未保存 audio PCM 游标、motion余时、sprite余帧时长、transition/effect进度、reveal进度、auto/skip/输入/历史UI状态；TextStyle只保存RGB/字号/行字距。读档重新播放声音，apply_frame重新开始本帧时长。 | 稳定 KeyWait 保存、正文mask和后续脚本位置已测；不能推论任意动画/淡化/输入中的即时保存可精确恢复。 |
| 已确认可能污染恢复 | `restore_runtime`，`src/runtime.cpp:238` | 清了 sounds/motions 等，但未清 audio_fades、旧 channel/group volume 映射、native_layers、history_mode/displayed_history；未出现于快照的旧组配置仍可能留在 Audio 对象。 | 在读档前有未结束 fade/历史界面/不同组状态时可能把旧状态带入新档。现有 stable-page save/load 测试未覆盖这些组合。 |
| 原生状态子集恢复 | `native_scene / restore_native_scene`，`src/runtime.cpp:181`、`:200` | glyph 按编码宽度猜位置写出，读回又合并成字符串；忽略各glyph独立坐标/颜色/字体差异。声音从头播放；尾部模式/附加整数未使用；active文本层写死13。System.dat只应用两个全局bank，字体与其他配置保留模板而未应用。 | E8 已确认这些记录字段存在。容器往返、Yukari加载原生档后能推进，不等于 Windows 原版接受新档、画面逐像素一致、音频精确续播或系统选项完整同步。 |
| 已确认配对文件不一致 | `SaveDataCopy / SaveDataCommentChange`，`src/runtime.cpp:431`、`:432` | 只处理 `.yukari`，不复制/更新对应原生 `savedata.NNN`；native-only源槽还可能因own文件不存在而失败。 | E9 中 Flag282 的 slot998→目标槽路径是实际脚本分支。普通保存slot3/777成功未覆盖CG快照复制及原生注释修改。 |
| 前选择状态未动态维护 | `save_slot`，`src/runtime.cpp:260` | previous 子档只从 original_template 复制已有嵌套内容；未按当前运行的 savedata.000 重新取前选择快照。 | E8 原版 serializer 明确读当前 savedata.000。原生容器结构正确并不能证明回前选择回到本次游玩的正确节点。 |
| UI布局近似 | `GraphicCreateSaveData / slot_info`，`src/runtime.cpp:174`、`:434` | 缩略图、日期、注释的位置/字号写死，SaveDataDisplaySetTo*只登记。native日期直接由文件mtime估算，尚未证明与原版日期来源一致。 | 当前保存菜单截图可用；调整显示配置和拷贝文件后时间显示不保证原版一致。原生时间字段的完整契约仍需补证。 |

## 验证范围及后续取证

已有的像素/音频/VM单测、标题集成和剧情/菜单/存读档集成证明本机 Yukari 的被测路径能到达稳定等待、绘图和继续执行；GPU测试证明实际使用 Metal 与 render targets。它们没有运行 Windows 原版，未建立转场中间帧、GDI排版、音频波形、所有设置和全剧情的行为等价。

本审计没有重跑全部集成，也没有增加通过数量；只阅读当前实现、已有报告及原版导出静态证据。优先可直接修正而不需猜新语义的是：NWC居中、TextWindowSet短参/活动层、GraphicMove打断滚动、已确认的sprite action/反向初始化、配对存档复制/注释、读档清理旧fade等状态。实现前仍应补对应回归。

需由根代理统一查询的新证据地址：`0x425E30` / `0x425F70`（转场mode/mask/RECT内部）、`0x4455F0`（文本控制码/逐字）、`0x4435C0`（KeyWait变体/auto/skip）；mask最终像素公式沿 `0x40BB60` 及 loader 链进一步追踪。现有wrapper不足以回答这些问题，不能把未补证项目默认为某一像素算法。
