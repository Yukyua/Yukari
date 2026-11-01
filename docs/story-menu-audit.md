# 原版剧情、菜单与存读档脚本审计

日期：2026-10-31。范围仅原版 `game.exe` 及 `data01.ykc` 至 `data05.ykc`。下述行为来自真实 YKS 反汇编；未运行 Windows 原版。除明确引用 REA 的条目外，参数意义属于脚本交叉观察或实施推断。

## 剧情入口与最小推进路径

`title_main.yks:0x273 LABEL_NEW` 在转白与清标题后设置 Flag:290=1、Flag:395=1，开启 KeyWaitIcon/SubMenuEnableMode，再跳入 `yks\root\root_common.yks`。

`root_common.yks` 首先置 Flag:310=0，调用 `etc\prlogue_skip.yks`。后者设置 Flag:100=1；GlobalFlag:150=0 时立即 Return，因此全新状态不会出现“跳过序章”的选择。随后设置章节索引/窗口标题，`0x27` 调用 `system\AutoSave.yks`，`0x2A` 调用 `story\common\prologue01.yks`。AutoSave 只有 `SaveDataSave(1,"")` 与 Return。

序章开始所需服务和同步点：

| 脚本 word | 调用 | 宿主要求 |
|---|---|---|
| 0x00..0x0D | SoundLoadStream、SoundVolume、SoundVolumeToFade、SoundPlayLoop | 淡入音量与流式循环音频 |
| 0x12..0x38 | GraphicLoad/Hide、Transition、SpriteCreate | 背景、叠图、胶片噪声动画 |
| 0x40/0x45 | GraphicTransmittanceScroll(99,1000,2500)、GraphicTransmittanceScrollWait(99) | 图层透过率异步动画及阻塞恢复 |
| 0x4B | ScriptCall(WindowShow.yks) | 显示正文图层11/12/13；依据 Flag:220/230 决定姓名窗14 |
| 0x6C | StrOutNWC(15,name) | 写姓名图层；不等待玩家输入 |
| 0x70 | SpriteCreate(14,0,0,0,125,44) | 姓名装饰图集组44 |
| 0x86/0x89/0x8B | StrOut(text)、KeyWait()、PF() | 正文输出→输入等待→清页；不允许一次执行穿过所有对话 |

首个稳定正文等待点是 `prologue01.yks:0x89`，下一个为 `0xDB`。首个 KeyWait 后还有独立图层淡入、声音和多个正文页，可以用数次推进测试实际场景状态变化。正文文本不是宿主内置字符串，应直接使用 YKS 解码后的文本。

整份 prologue01 中出现的非基础宿主命令还包括 StrOutNW、SoundDeleteToFade、Effect、FontSet、UserTransition、GraphicScroll/Wait。该脚本有839次 StrOut、827次 KeyWait/PF；只跑通首句不等于序章全部适配。

## 剧情底栏与快捷键

`system\createMenu.yks` 将图集101的10个精灵挂到正文层13；按钮位置是绝对逻辑画布坐标。区域服务仅在对应父层可见且 KeyWait/菜单功能启用时应生效，这一约束需要原版 AreaSet 处理器补证。

| 功能 | 点击矩形 left,top,right,bottom | 脚本/服务 |
|---|---|---|
| 跳过 | 452,554,503,575 | AreaSetAlreadySkip 或 AreaSetSkip，由 GlobalFlag:3 选择 |
| 自动 | 508,554,559,575 | AreaSetAutoMode |
| 保存 | 564,554,615,575 | AreaSetMouseLeftScript(1,m_save,...) |
| 读档 | 620,554,671,575 | AreaSetMouseLeftScript(2,m_load,...) |
| 快存 | 676,554,727,575 | AreaSetMouseLeftScript(3,q_save,...) |
| 快读 | 732,554,783,575 | AreaSetMouseLeftScript(4,q_load,...) |
| 历史 | 788,554,839,575 | AreaSetHistoryMode |
| 选项 | 844,554,895,575 | AreaSetMouseLeftScript(5,m_option,...) |
| 重播语音 | 900,554,951,575 | AreaSetMouseLeftScript(6,playVoice,...) |
| 隐藏窗口 | 956,554,1007,575 | AreaSetMouseLeftScript(7,WindowHideSys,...) |

`createScKey.yks` 的 KeySetScript 参数为 `(id,script,-1,-1,VK...)`；其真实映射是 F1/A 自动、F2/R 跳过、F3/O 选项、F6/S 保存、F7/L 读档、F8/B 回选择、F9/T 回标题。F4/H 历史与 F5/Space 隐藏窗口由 SystemKeySet 服务设置。这里的数值是原版 Windows VK；SDL 按键需转换，不能把112等值当作SDL scancode。

`m_option.yks` 根据 Flag:240 跳 m_game 或 m_sound。m_game 设置界面包含 FullScreenModeGet/FullScreenMode、TransitionModeSet、DefaultFontEffect、CursorAutoHide、DefaultKeyinSoundStopSet、SubMenuScript、MouseXPointGet、TextSpeedSet、AutoSpeedSet、AutoModeCharactorWaitSet 等。只处理菜单按钮不足以完成设置界面的滑条与模式切换。

## 共用选项侧栏与退出

m_game/m_save/m_load 共用右侧菜单，x=869..1023：

| y范围 | 操作 |
|---|---|
| 56..88 | 保存（Flag:290 限制，当前保存页禁用） |
| 96..128 | 读档（当前读档页禁用） |
| 136..168 | 游戏设置（当前游戏设置页禁用） |
| 176..208 | 音频设置 |
| 232..264 | 跳到后续选择，Flag:395 限制，确认类型1 |
| 272..304 | 回此前选择，确认类型2 |
| 328..360 | 关闭菜单、恢复游戏 |
| 368..400 | 返回标题，确认类型3 |
| 424..456 | 快捷键说明 |
| 464..496 | m_game 独有恢复默认设置 |

`MouseKeyCase(2,...)` 注册全屏右键，设置 Flag:199=1 并跳 m_cls。`m_cls.yks` 只删除图层200..349；Flag:199=1 时清 Flag:241/242、播放取消音并转场，然后 Return。因此菜单必须作为剧情等待之上的可返回上下文执行，而非覆盖剧情入口。侧栏跨页使用 ScriptJump，最终 Return 应回到最初的菜单调用者。

## 原版存读档脚本协议

m_save/m_load 起始从 GlobalFlag:85、86 复制页面/行选择到 Flag:4000、4001。slot = page*10 + row，其中 page=0..9、row=1..10。第一页的 slot1、slot2 分别是自动和快速存档；m_save 的 SAVE_DATA 分支明确拒绝手动写入1、2。可人工写入的第一个槽是3。

每页十个存档项矩形如下：左列 x=24..423，右列 x=436..835；y依次为64..151、160..247、256..343、352..439、448..535。左列行1..5、右列6..10。页面标签位于顶部 x=438+40*page..473+40*page、y14..49。

`GraphicCreateSaveData(layer,slot,x,y)` 为每个槽创建预览图层271..280；不能只显示截图而忽略 `OptionSystem.yks` 设置的日期、注释、文字样式和缩略图布局。数据不存在的槽仍需要可点击的“无数据”背景，供保存使用。

保存步骤（m_save:0x71B起）：

1. 计算 Flag:4002=slot，String:15置空。slot1/2直接返回列表。
2. SaveDataCheck(slot) 返回值写 Flag:4003；脚本按1=存在、0=缺失处理。
3. 已存在设置 Flag:100=6，否则=5，再调用 m_YesNo。
4. Flag:110=0则取消；否则更新 GlobalFlag:85/86。
5. Flag:282=0 时调用 SaveDataSave(slot,String:15)；否则修改 slot998 的注释并 SaveDataCopy(998,slot)。这条特殊路径不能错误保存当前菜单上下文。
6. 重新进入 SAVE_LIST，重建预览。

读档步骤（m_load:0x71B起）：计算slot，SaveDataCheck为0直接回列表；设置 Flag:100=7 调 m_YesNo；确认后停止声音、转黑、更新页面选择并 AutoSkipClear，最后 `0x76E SaveDataLoad(slot)`。成功读档应恢复已保存的脚本/画面等待点，不能继续执行紧随其后的菜单循环。

m_YesNo 将 Flag:110先置0；确认矩形 `(324,302)-(499,341)` 将它设1；取消矩形 `(524,302)-(699,341)` 或全屏右键保持0。保存类型5/6有可选注释输入区 `(346,262)-(677,292)`，调用 InputDialog(String:15,"",26,811)，成功后 InputDialogGetText 返回字符串、PF(809)/StrOut(809,text)重绘注释。直接确认允许空注释。

q_save 使用 slot2，Flag:290=0时禁止；q_load 首先检查slot2，GlobalFlag:40=1时显示确认类型9。AutoSave使用slot1。slot0供回选择，slot998用于CG模式前快照；这些槽的含义来自脚本，不能把slot0当作普通自动存档。

## 建议自动化验证

1. 用隔离的 Yukari 存档目录和全新系统状态启动；标题点击 NEW `(220,410)`，等待 `prologue01:0x89`，检查章节索引、正文非空、姓名15和图层14；不要仅以未报错判定。
2. 推进一次到下一个 KeyWait，比较正文与图层状态；保存当前VM location、局部变量、正文、层资源/坐标和BGM以作后续断言。
3. 点击底栏保存 `(590,565)` 或按S；等待m_save选择，点击slot3 `(200,300)`，再确认 `(400,320)`。检查实际文件和缩略图，并验证列表显示已存槽。
4. 右键关闭菜单，推进两页；用L进入m_load选择slot3、确认。恢复后断言回到原存档对话页/VM等待点，菜单层200..349与确认层800..811消失，继续推进不会返回m_load循环。
5. 跨进程重启后重复步骤4，验证存档不是内存内的临时快照。
6. 分别验证取消保存、取消读档、缺失槽读档、slot1/2不可手动覆盖，以及从设置页跳保存/读档再关闭仍恢复剧情。
7. 对 m_game 点击返回 `(940,344)`、右键、调整文字速度滑条与选项切换，断言设置改变并持久化；返回标题前需选择确认，不能一击直接丢失剧情。

原版二进制格式兼容、真实存档反序列化与写回另需存档结构逆向，本审计只给脚本契约，不能证明 Yukari 格式可与原版互读。

## 原版二进制存档的新增静态证据

以下仅分析原版 EXE，不以现有存档中可能受汉化程序影响的正文作为引擎文本证据。

- `Yuka_SaveDataSave`，REA `ev_a3b06536d7a7f882ff8c6a03093b56536656afe428b13dcdf215d2a79a615381`：普通执行状态通过 `FUN_00443030(slot,context,comment)` 直接序列化；context+0x80118 为1/2，或 context+4 所指对象+0xC9 为1时，改为复制 `savedata.---` 到目标槽并修改注释。因此“菜单保存应使用菜单前快照”有原版代码直接支持。
- `FUN_00443030`，REA `ev_e72105395f12b7d5f25d1347f0a0a6f8a18dcc6826503763193a0f9b0a893bcc`：由slot建立路径后调用 `FUN_00442970`。`FUN_00432bd0`，REA `ev_b3e16cb6a4403b8803fb211105e7f7204512d51978183a274a85a94e4a2e4c40`：路径模板明确为配置存档目录下 `savedata.%03.3d`。
- `FUN_00442970`，REA `ev_c2d20fa45bc6af2646c126cb0873ca73eb84e7b54e6802c3b0ba8aad0f2d2a1d`：容器头长0x48，magic为`Yuka  `；写注释、索引/额外字符串、BMP缩略图、VM/scene状态、可选历史与可选嵌套存档。状态由 `FUN_0043f160` 写。历史由context的1000项环队列按每项count、count×0x1C记录、声音字符串长度及字符串写入。嵌套存档在全局开关 DAT_0056c868 开启时从 `savedata.000` 整文件复制，证实其为前选择快照，而非压缩后的第二份当前状态。
- `FUN_0043f160`，REA `ev_c158dfe33026b0a974cee83df9467e5052e5be2fd7ec2387a8c1842185126aa4`：state本身以0x160字节头开始。其+0x14/+0x18为Flag区offset/size（0x40000=65536个int32）；+0x1C/+0x20为音频区；+0x24/+0x28为图层offset/count；+0x2C/+0x30为脚本上下文链offset/count；+0x110/+0x114为String索引offset/size（0x80000=65536对offset/length）。这些是容器文件绝对offset，不能无条件当作state相对偏移。图层通过`FUN_00415c00`，上下文通过`FUN_00433600`逐项序列化。
- `Yuka_SaveDataLoad`，REA `ev_0c5aa707ddeda4b94012927e9822079b14a9260f83dfd3b5b65ddc64d2d7e825`：处理器自身先检查文件、标记全局待读槽，并通知插件；它不在当前handler内立即反序列化，主调度循环随后负责实际恢复。

原始SystemDataWrite处理器只向主窗口发送0x800D消息（REA `ev_fd83631f11a5e034f4d30a1e9c3a2179986dd11d0c110b421c9b98d1844d77ef`），初始尚未确认System.dat格式；后续从写入错误字符串定位的直接writer证据见末节，不能套用savedata容器头。

`FUN_00433600`（REA `ev_644e0c631cbc951c89d743585e40475d4cfc06c0380d6befb969ea1f47c68c17`）确认脚本frame头长0x50，+4为下一frame文件位置，+8/C为脚本名offset/length，+10/14为code offset/DWORD count，+18/1C为index offset/16字节记录数，+20/24为data offset/length；+28/+2C分别复制当前/下一word PC；+30为当前返回目标index，+34为该目标是否激活；+40/44为局部变量表offset/count，每项12字节，tag1为整数、tag2为字符串offset/length、tag0未初始化。脚本数据段可由配置省略，恢复时必须重新读取对应原版资源。

`FUN_00415c00`（REA `ev_e6374f68ed2a0dbe3a9bd9917fed345169857bbff0ce9cd08e03c9d6a1e2cb32`）确认图层头长0x140，+4为下一图层位置、+8为ID、+C为图像类型、+20/24为坐标、+28字节为可见；+5C/60为资源路径descriptor、+64/68为额外mask、+6C/70为精灵记录offset/count。精灵记录长0x7C，+8为kind、+C可见、+10/14坐标、+18/1C尺寸、+20..2C裁切矩形、+34源图层ID、+38播放mode、+48/4C动画组/当前帧、+5C槽位、+60命名精灵标记、+70/74私有payload描述。kind0/1均可有动画数据；kind3为逐字glyph，其+3C原始双字节字符、+40字体索引、+44颜色。

`FUN_0045eb30`（REA `ev_912831989b2e007311fecabb868cb6b38432a4b05a8623823b070c924cfea646`）确认音频记录长0x20，+4为下一记录位置、+8 ID、+C正在播放标记、+D循环字段、+E附加模式、+10/14资源路径descriptor、+18音量、+1C附加整数。音量另与SoundVolume处理器交叉确认（REA `ev_a1cbcf64d222b51b2aeb6ff443a277316b6290d05a38dcbe1bd52ea5d51f9333`）；其他音频字段不代表已经还原精确播放进度。

新增`decode_original_scene`解码器保留图层原始header及尚未解释字段；后续`encode_original_scene`支持向新建或内存副本中的原版容器追加原生场景记录。独立记录/畸形描述符/嵌套容器base测试及ASan/UBSan通过；本地现有savedata文件均可结构解码。这是结构与边界验证，不是与原版运行画面的逐像素一致证明。

`FUN_00416fe0`（REA `ev_81fdac41ca9ee0b561d11fa97466f6b3c181b628c739589ba2db3bfbf2c6e02d`）进一步直接确认透过率来自layer header+0x74；kind0精灵为临时区域/hover元素，原版读档时跳过，kind1为动画图像，kind3为文字。新建层未启用的缩放/旋转等flag保持0；缺省文字白色在+0x30，+0x80/+0x84缺省-1。字符WORD按编码字节低位在前保存，不能直接把Unicode code point写进去。

原生场景写入器`encode_original_scene`追加0x140层头、CP932路径、精灵私有payload、连续0x7C精灵记录和0x20音频记录，并回填所有链表next与文件offset。现有header未知字段保留；新建header使用已确认缺省值。保存原生图层/精灵时必须设置图像类型、真实资源名、裁切/组/帧、透过率和可见性；没有资源名的纯文本层也必须有文字kind3记录或在恢复后由脚本重建。该模块不声称单独完成VM、历史、系统档或原版实际执行验证。

验证增加：78份现有存档的场景结构decode→encode→decode往返、UTF-8/CP932路径、未知header保留、新建空容器、嵌套base与写入失败原子性；ASan/UBSan通过。所有往返在内存完成，未覆盖原游戏存档。

## System.dat 全局状态格式补证与实现

沿原版 `System.dat` 写入错误字符串定位启动函数0x42DAB0，再到直接写入器 `FUN_0042acb0`，REA Evidence `ev_eb47df0dcbb139b1136ff698f5995454c63d749c9bd5fc970503f428ae54f8a8`。原版writer直接写40000字节全局整数（10000×int32），从10000个字符串指针写字符串内容，再写80000字节索引（10000×offset/length）。字符串分配长度是文本字节数+2。

与现有System.dat的结构交叉核对：header+0x10=0x7C；+0x18/1C为整数bank offset/40000；+0x50/54为字符串索引offset/80000。0x20/24为原版字体表区；其他配置字段及前16字节没有确认完整语义，不能按普通Yuka存档magic处理。

新增`original_system.hpp/cpp`支持两个全局bank的读取和基于原生模板的写入。编码器保留完整原文件作为模板，在其后追加新bank及字符串索引并回填header；字体和其他未知配置区原位保留。没有原生模板时明确拒绝写入，避免把全零未知配置伪装为完整兼容系统档。调用方应输出至选定的Yukari状态目录，不能默认覆盖游戏源System.dat。

CP932合成样本、截断/错误bank尺寸/越界字符串/编码失败/模板保留测试及ASan/UBSan通过。现有System.dat含翻译编码字符串，验证时显式使用CP936完成10000个整数及字符串decode→encode→decode一致；这仅验证数据容器，不将翻译文本当作原版引擎行为。全程未修改源System.dat。
