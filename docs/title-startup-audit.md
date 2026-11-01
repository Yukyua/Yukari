# 原始 参考样本：启动至标题等待审计

2026-10-31。范围为原始 data01.ykc 脚本与 data02.ykc 图像元数据；未读取汉化 EXE、CNP 或补丁。以下偏移为代码区 DWORD 索引。调用实参按脚本观察记录，等待单位、图层顺序与动画枚举仍以原版 EXE 后续证据为准。

## 到达路径与配置门控

`start.yks` 设置游戏名、版本，然后依次调用以下标签：

1. SETUP_BOOTCHK：直接 Return。
2. SETUP_SYSTEM → optionsystem：WindowNameSet、Index、白色层0、禁用存取档和子菜单、选择设置；AutoSkipClear；存档显示格式。
3. SETUP_FORMAT：GF60=0 时设 Flag100=0，调用 optiondefault 设置三组默认选项；随后 GF60=1。
4. SETUP_SOUND → optionvoice：配置声音组、组音量与自动/跳读行为。
5. SETUP_TEXT：createtext → createfont，然后 createmenu、createsckey、createhistory、createicon。
6. SETUP_OPTION → optionsetting：文本速度、自动速度、字体、描边、文字窗透明度、转场与菜单/鼠标设置。这里再次调用 createfont。
7. SETUP_ETC：SoundLoad 100/101/102/105 为 click1/cur1/cur1/cancel。
8. SETUP_LOGO：GF51=0 时依次调用 logo、logo_e、title_ani、title_ani_e；之后 GF51=0。
9. PROLOGUE：Flag199=0，然后 ScriptJump 到 title_main（start word 0x9D）。后面的剧情准备并不在该跳转前执行。

因此已有全局存档 GF60/GF51 会改变初始化/开场是否执行，不能只检查一种持久状态。

`title_main` 在 `0x24` 调用 FlagReset，`0x26` 调用 SET_DATA，`0x29` 调用 title_bace，`0x2C` 加载 BGM 流1，`0x30` SoundPlayLoop(1)，SETUP 标签装按钮，`0x69` UserTransition(2,effect01,500)，`0x6E` 调用 vo_assist01，最终 `0x74` MouseSelect()。MouseSelect 之前没有正文 StrOut，标题文字主要来自图像。

## 文本与界面初始化

createtext 先隐藏加载层11 win_text@(0,436)、13 damy@(0,0)、14 win_name@(0,531)、15 damy。TextWindowSet(13,235,457,970,600)，GraphicTextRect(15,0,536,400,240)；默认文字窗组 `(15,13,11,12,14)`，RubyFontSet(3)，KeyWaitIconPositionSet(13,1,970,530)。RGB 的整数返回值通过 `$n; =(); RGB(...)` 保存，再传 TextColor/TextBorderColor，不能将 RGB 做无返回值配置调用。

createfont 在 GF2=0 时用 `ＭＳ ゴシック`，否则 `ＭＳ 明朝`；定义槽0、1、3至14，共14个槽。默认参数包括槽0 `(27,200,28,32)`、槽1 `(27,200)`、槽3 `(7,100)`、槽9 `(25,200,25,32)`、槽11 `(30,1600,30,46)`、槽12 `(45,1600,45,60)`。GDI字体名不能假设 macOS 原生存在。

createmenu 创建隐藏源101/102及 layer13 的10个子sprite，注册鼠标区域、回调脚本与提示区。createsckey 注册按键回调；createhistory 设置隐藏图像180..183/199与历史操作区；createicon 创建层120与隐藏源121/125/126。这些是注册行为，参数中的脚本路径不代表立即执行它们。

## logo 与标题动画

| 脚本/word | 观察到的关键操作 |
|---|---|
| logo 0x07..0x15 | 层0白色；隐藏层1 hook_soft；DrawStart；SpriteCreate(0,0,387,245,1,0) |
| logo 0x1D | Wait(700) |
| logo 0x23 | Random(4) 的返回值决定五个 SYS_*_001 语音分支之一；具体Random上下界须原版处理器确认 |
| logo 0x71/0x74 | SoundPlayWait(99)，然后 Wait(500) |
| logo 0x77..0x87 | 重建白色层0，Transition(2,300)，Flag0=1 |
| logo_e | 重建白色层0；Flag0=1走DrawStart，否则Transition(2,1000)；SoundStopALL |
| title_ani 0x05 | Wait(500) |
| title_ani 0x0A..0x3A | 白色背景0；damy层5@(480,151)、层6@(80,151)；隐藏源8/9为face_a/face_b |
| title_ani 0x40..0x4C | 层4/5/6 transmittance=1000，Transition(2,500) |
| title_ani 0x60/0x66 | moji层3@(280,151)，UserTransition(2,effect01,500) |
| title_ani 0x73/0x79 | flower层2@(280,151)，UserTransition(2,effect02,750) |
| title_ani 0x7E..0xB0 | SpriteCreate(5,0,83,113,8,0)，SpriteCreate(6,0,113,26,9,0)；层5/6透明度滚动到0(raw700)，位置滚动到(280,151)(raw800)；GraphicScrollWait(5) |
| title_ani 0xBB..0xCE | 同sprite改motion1；Wait(400)；SpriteMotionWait(6,0) |
| title_ani 0xDA..0xE5 | ribon层4@(280,151)，UserTransition(2,effect01,300)，Wait(750) |
| title_ani_e | 在99层加载白色，Transition(2,500)，删除层1..8及99（注意脚本没有删除层9） |

等待服务需要继续渲染和处理窗口事件。SoundPlayWait、GraphicScrollWait、SpriteMotionWait 必须有可完成状态，不能永久阻塞，也不能只因为载入图像就认定动画已完成。所有上表时间值为原始实参，未在此凭名字定义单位。

### 动画资源布局

hook_soft 图集996×430，21条记录：20个249×86有效帧加全零终结记录；type字段4，原始duration含32/128/256。

face_a、face_b 各540×60，20条记录形成两组：9个60×60有效帧+全零记录，再9帧+全零记录。第一组type2，第二组type4，duration均50。脚本使用motion0后切motion1，与这两组布局一致。此为组结构证据，不独立定义type2/type4的循环语义。

## 稳定标题图层与按钮

GF600=0 为默认标题。title_bace 的 LABEL_A 加载：

| 层 | 资源 | 坐标 | 资源尺寸 | 加载状态 |
|---:|---|---|---|---|
| 0 | title/title_bace.ykg | 0,0 | 1024×576 | GraphicLoad |
| 29 | title/title_logo.ykg | 280,61 | 464×237 | GraphicLoad |
| 30 | system/damy.ykg | 0,0 | 16×16 | GraphicLoad，含独立Alpha |
| 31 | title/title_btn.ykg | 0,0 | 1240×248 | GraphicLoadHide，按钮图集 |
| 32 | title/title_alt.ykg | 0,0 | 304×320 | GraphicLoadHide，提示图集 |

title_btn 有40条帧记录，20个有效矩形各接一条全零终结记录。前10个矩形为 `(124*n,0,124,124)`，后10个在y=124。title_alt 有20条记录，10个有效304×32矩形各接全零记录。SpriteCreate末参motion编号不能简单作为裸帧记录下标。

SET_DATA 在 GF600=0 时设置按钮坐标，层30的子sprite从隐藏层31引用：

| 按钮 | 子sprite/motion | 坐标 | 分支Jump word | 跳转 |
|---|---|---|---:|---|
| New | 0 / 0 | 162,350 | 0x144 | LABEL_NEW |
| Load | 1 / 1 | 306,350 | 0x16A | LABEL_LOAD |
| Option | 2 / 2 | 450,350 | 0x190 | LABEL_OPTION |
| Omake | 3 / 3 | 594,350 | 0x1B6 | LABEL_OMAKE |
| End | 4 / 4 | 738,350 | 0x1DC | LABEL_END |

各 MouseCase2 的首参控制启用；Omake 的启用与sprite创建均受 GF200 条件控制：GF200=0时 Flag50=0、Flag51=-1，无sprite3。其余按钮始终创建。

命中框由按钮坐标至(x+123,y+123)，传给MouseCase2；悬停 IndicateCase 使用层32，默认提示坐标 Flag80/81=(360,496)。MouseCase2 每项参数还引用layer30、source31、默认motion=-1、悬停motion10..14、语音通道110..114、音效100与raw128。

GF600=1..5 为其他标题变体：背景/按钮/提示来自 `title/org`，logo@(39,41)、cl层28@(135,552)，角色层25从(466,0)滚动到(446,0)，raw300；按钮坐标改为(65,291)/(209,291)/(353,291)/(65,420)/(209,420)，提示@(353,420)。该路径还有GraphicTransmittanceScroll/GraphicScrollWait等待。

## MouseSelect 的稳定边界

`title_main:0x74` 调用后，外层 `{` 为0x76，link=0x1E0，后有 ScriptJump START 的0x1E1。内部包括 KeyTableCase、五个IndicateCase、全屏MouseCase和五个MouseCase2及其分支体。

这是一个输入选择块；原版需要解析/注册这些case、维护悬停sprite/提示，并等待选择后进入对应分支。将MouseSelect当普通无操作并线性执行会跳过所有case，随后回跳START而形成忙循环。只停在0x74可以作为“到达标题等待”验收点，但不能据此宣称鼠标选择已兼容。此前0x72为DrawStop，渲染器必须在进入选择状态时发布准备好的标题画面并继续处理事件。

## 可复核遍历与host范围

另用临时VM遍历器检查新游戏零全局状态的可达性：所有host仅记录并立即完成，Random固定返回0，RGB占位返回0，FlagReset清理普通Flag；首次MouseSelect返回Waiting。该遍历在790个VM步骤抵达title_main:0x74，遇到92个不同host名称。它没有实现原版服务，也不是原版运行验证；特别是RGB占位不能用于显示效果比较。

以下是该路径遇到的host名称及原始脚本中一个静态实参例子。示例中的$n/GF尚未求值；同一服务可能有其他重载或条件分支。

| Host | 静态例子位置 | 实参例子 |
|---|---|---|
| AreaSetAlreadySkip | `yks\system\createMenu.yks:0x68` | `13, 452, 554, 503, 575, 101, 452, 554, -1, 10, -1, 102, -1, -1` |
| AreaSetAutoMode | `yks\system\createMenu.yks:0x8e` | `13, 508, 554, 559, 575, 101, 508, 554, -1, 11, -1, 102, -1, -1` |
| AreaSetHistoryExit | `yks\system\createHistory.yks:0x92` | `180, 800, 539, 937, 576, 181, 800, 539, 0, 1, 0, 101, -1, -1` |
| AreaSetHistoryMode | `yks\system\createMenu.yks:0xe6` | `13, 788, 554, 839, 575, 101, 788, 554, -1, 16, -1, 102, -1, -1` |
| AreaSetHistoryNext | `yks\system\createHistory.yks:0x72` | `180, 955, 512, 978, 535, 182, 955, 512, 2, 10, 2, 101, -1, -1` |
| AreaSetHistoryPageNext | `yks\system\createHistory.yks:0x82` | `180, 955, 544, 978, 567, 182, 955, 544, 3, 11, 3, 101, -1, -1` |
| AreaSetHistoryPagePrev | `yks\system\createHistory.yks:0x52` | `180, 955, 16, 978, 39, 182, 955, 16, 1, 9, 1, 101, -1, -1` |
| AreaSetHistoryPageSize | `yks\system\createHistory.yks:0xb2` | `3` |
| AreaSetHistoryPrev | `yks\system\createHistory.yks:0x62` | `180, 955, 48, 978, 71, 182, 955, 48, 0, 8, 0, 101, -1, -1` |
| AreaSetHistoryScrollBarV | `yks\system\createHistory.yks:0xa2` | `180, 955, 92, 978, 496, 183, 955, 12, 0, 0, 0, -1, -1, -1` |
| AreaSetIndicate | `yks\system\createMenu.yks:0x12c` | `20, 13, 452, 554, 503, 575, 102, 729, 425, -1, 0, -1` |
| AreaSetMouseLeftScript | `yks\system\createMenu.yks:0x9e` | `1, "yks\\system\\option\\m_save.yks", 13, 564, 554, 615, 575, 101, 564, 554, -1, 12, -1, 102, -1, -1` |
| AreaSetSkip | `yks\system\createMenu.yks:0x66` | `(无参数)` |
| AutoModeCancelLKey | `yks\system\OptionSetting.yks:0xdf` | `"GlobalFlag":23` |
| AutoModeCharactorWaitSet | `yks\system\OptionSetting.yks:0x16` | `50` |
| AutoSpeedSet | `yks\system\OptionSetting.yks:0x3` | `"GlobalFlag":5` |
| CursorAutoHide | `yks\system\OptionSetting.yks:0xbf` | `0` |
| DefaultFontEffect | `yks\system\OptionSetting.yks:0x35` | `3` |
| DefaultFontSet | `yks\system\createFont.yks:0xa` | `0, "ＭＳ ゴシック", 27, 200, 28, 32` |
| DefaultKeyinSoundStopSet | `yks\system\OptionSetting.yks:0x4f` | `"GlobalFlag":16` |
| DefaultTextWindowGroup | `yks\system\createText.yks:0x61` | `15, 13, 11, 12, 14` |
| DrawStart | `yks\system\logo.yks:0x13` | `(无参数)` |
| DrawStop | `yks\system\logo.yks:0x5` | `(无参数)` |
| FlagReset | `yks\title\title_main.yks:0x24` | `(无参数)` |
| FontSet | `yks\system\createText.yks:0x37` | `13, 0` |
| GraphicDelete | `yks\system\logo.yks:0x79` | `0` |
| GraphicLoad | `yks\system\OptionSystem.yks:0x10` | `0, "ykg\\system\\white.ykg", 0, 0` |
| GraphicLoadHide | `yks\system\createText.yks:0x0` | `11, "ykg\\system\\window\\win_text.ykg", 0, 436` |
| GraphicScroll | `yks\title\title_ani.yks:0x9e` | `5, 800, 280, $6` |
| GraphicScrollWait | `yks\title\title_ani.yks:0xb0` | `5` |
| GraphicTextRect | `yks\system\createText.yks:0x3b` | `15, 0, 536, 400, 240` |
| GraphicTransmittance | `yks\system\OptionSetting.yks:0x47` | `11, "GlobalFlag":10` |
| GraphicTransmittanceScroll | `yks\title\title_ani.yks:0x8e` | `5, 0, 700` |
| HistoryExitToNext | `yks\system\createHistory.yks:0x1e` | `1` |
| HistoryReset | `yks\title\title_main.yks:0x15` | `(无参数)` |
| HistorySoundGroupSet | `yks\system\createHistory.yks:0x3b` | `10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22` |
| HistorySoundIconSet | `yks\system\createHistory.yks:0x4d` | `199, 0, 1` |
| HistorySoundNumberSet | `yks\system\createHistory.yks:0x4a` | `99` |
| HistoryTextColor | `yks\system\createHistory.yks:0x30` | `$0, $1` |
| HistoryToVoiceContinue | `yks\system\createHistory.yks:0x21` | `1` |
| HistoryWindowSet | `yks\system\createHistory.yks:0x34` | `180, 110, 30, 1024, 550` |
| IconSetAlreadySkip | `yks\system\createIcon.yks:0x13` | `120, 846, 0, 121, 1` |
| IconSetAutoMode | `yks\system\createIcon.yks:0x1a` | `120, 846, 0, 121, 0` |
| IconSetSkip | `yks\system\createIcon.yks:0xc` | `120, 846, 0, 121, 1` |
| Index | `yks\system\OptionSystem.yks:0xd` | `"GlobalString":0` |
| KeySetScript | `yks\system\createScKey.yks:0x0` | `1000, "yks\\system\\com\\auto.yks", -1, -1, 112, 65` |
| KeyWaitFunctionSet | `yks\title\title_main.yks:0x1f` | `2047` |
| KeyWaitIcon | `yks\title\title_main.yks:0x19` | `0` |
| KeyWaitIconPositionSet | `yks\system\createText.yks:0x68` | `13, 1, 970, 530` |
| LoadEnableMode | `yks\system\OptionSystem.yks:0x19` | `0` |
| MouseSelect | `yks\title\title_main.yks:0x74` | `(无参数)` |
| MouseSelectFunctionSet | `yks\system\OptionSystem.yks:0x29` | `0` |
| RGB | `yks\system\OptionSystem.yks:0x3d` | `16777215` |
| Random | `yks\system\logo.yks:0x23` | `4` |
| RubyFontSet | `yks\system\createText.yks:0x5e` | `3` |
| SaveDataDisplaySetToComment1 | `yks\system\OptionSystem.yks:0x55` | `175, 46` |
| SaveDataDisplaySetToComment2 | `yks\system\OptionSystem.yks:0x59` | `175, 66` |
| SaveDataDisplaySetToDate | `yks\system\OptionSystem.yks:0x4b` | `175, 8, "%4.4d/%2.2d/%2.2d %2.2d:%2.2d", 1, 3, 4, 12, 13` |
| SaveDataDisplaySetToFont | `yks\system\OptionSystem.yks:0x40` | `8, 2, $2` |
| SaveDataDisplaySetToGraphic | `yks\system\OptionSystem.yks:0x45` | `9, 9, 126, 70` |
| SaveDataLoadToCallScript | `yks\system\OptionSystem.yks:0x37` | `"yks\\system\\option\\m_LoadCall.YKS"` |
| SaveEnableMode | `yks\system\OptionSystem.yks:0x16` | `0` |
| ScriptSkipMode | `yks\system\OptionSystem.yks:0x23` | `1` |
| ScriptSkipReset | `yks\system\logo.yks:0x0` | `(无参数)` |
| SelectFunctionSet | `yks\system\OptionSystem.yks:0x26` | `255` |
| SelectTransitionSet | `yks\system\OptionSystem.yks:0x1f` | `2, 10` |
| SoundGroupAutomodeSkipSet | `yks\system\OptionVoice.yks:0xa2` | `10, 1` |
| SoundGroupSet | `yks\system\OptionVoice.yks:0x0` | `0, 1, 9` |
| SoundGroupVolumeSet | `yks\system\OptionVoice.yks:0x19` | `0, "GlobalFlag":100` |
| SoundLoad | `start.yks:0x68` | `100, "ogg\\system\\click1.ogg"` |
| SoundLoadStream | `yks\title\title_main.yks:0x2c` | `1, "ogg\\bgm\\12th_bgm_01_short.ogg"` |
| SoundPlay | `yks\system\logo.yks:0x30` | `99, "ogg\\voice\\etc\\SYS_MI_001.ogg"` |
| SoundPlayLoop | `yks\title\title_main.yks:0x30` | `1` |
| SoundPlayWait | `yks\system\logo.yks:0x71` | `99` |
| SoundStopALL | `yks\system\logo_e.yks:0x24` | `(无参数)` |
| SoundVolume | `yks\title\title_ani.yks:0x54` | `161, 666` |
| SpriteCreate | `yks\system\createMenu.yks:0xc` | `13, 0, 452, 554, 101, 0` |
| SpriteMotionWait | `yks\title\title_ani.yks:0xce` | `6, 0` |
| SubMenuEnableMode | `yks\system\OptionSystem.yks:0x1c` | `0` |
| SubMenuScript | `yks\system\OptionSetting.yks:0x86` | `"yks\\system\\option\\m_option.yks"` |
| SystemDataWrite | `yks\title\title_main.yks:0x22` | `(无参数)` |
| SystemKeySetHistoryMode | `yks\system\createScKey.yks:0x18` | `115, 72` |
| SystemKeySetWindowErase | `yks\system\createScKey.yks:0x1c` | `116, 32` |
| TextBorderColor | `yks\system\createText.yks:0x33` | `13, $7` |
| TextColor | `yks\system\createText.yks:0x27` | `13, $3` |
| TextSpeedSet | `yks\system\OptionSetting.yks:0x0` | `"GlobalFlag":4` |
| TextWindowSet | `yks\system\createText.yks:0x18` | `13, 235, 457, 970, 600` |
| Transition | `yks\system\logo.yks:0x82` | `2, 300` |
| TransitionModeSet | `yks\system\OptionSetting.yks:0x6a` | `1` |
| UserTransition | `yks\title\title_ani.yks:0x66` | `2, "ykg\\effect\\effect01.YKG", 500` |
| Wait | `yks\system\logo.yks:0x1d` | `700` |
| WindowNameSet | `yks\system\OptionSystem.yks:0xa` | `$0` |
