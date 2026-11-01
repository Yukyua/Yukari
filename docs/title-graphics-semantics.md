# 标题路径的图层与精灵证据

日期：2026-10-31。只针对 `game/game.exe` 与原版 `data01.ykc` 至 `data05.ykc`；不分析汉化程序与补丁。本文区分原始脚本/资源观察、原版静态代码和实现推断。没有运行原版 Windows 程序。

## 启动与标题构成

以下是 Yukari 对原始 YKS 的反汇编观察，可用 `build/yukari game-script game ENTRY` 重现。

- `start.yks` 依次初始化系统、文本/菜单等，再调用 `logo.yks`、`logo_e.yks`、`title_ani.yks`、`title_ani_e.yks`，最后跳入 `title_main.yks`。新状态的 `GlobalFlag:51=0` 会执行这些动画。
- `title_main.yks:0x26` 调用 `SET_DATA`；`GlobalFlag:600=0` 时设置按钮坐标为 `(162,350)`、`(306,350)`、`(450,350)`、`(594,350)`、`(738,350)`。没有通关状态时 `Flag:50=0`，不创建第四个 OMAKE 按钮。
- `title_bace.yks:0x17` 加载图层 0 `ykg\title\title_bace.YKG` 于 `(0,0)`，图层 29 `title_logo.YKG` 于 `(280,61)`，图层 30 `system\damy.YKG` 于 `(0,0)`；以隐藏方式加载 31 `title_btn.YKG` 与 32 `title_alt.YKG`。
- `title_main.yks:0x36` 起将图层 31 的动画组 0、1、2、4 创建为图层 30 的精灵槽 0、1、2、4；`UserTransition(2, effect01.YKG,500)` 后进入 `MouseSelect`。这是标题界面的稳定输入等待位置。
- 标题文字与按钮已绘入 YKG；进入默认标题不依赖 GDI 文本排版。`createText.yks` 创建的 11、13、14、15 图层均隐藏，设置正文与姓名绘制区域后供剧情使用。

上述默认状态只指没有持久化系统数据时的脚本分支；`GlobalFlag:600!=0` 的解锁背景具有不同坐标与图片。

## 原版 GraphicLoad 代码

已保留静态证据：`analysis/original-engine/seeded/function-graphicload.c.txt`，REA Evidence `ev_3c2a300f9f8fb94b371e9c70e6666e734ebb416bb396bdce25a63438c50ee9f8`，函数 VA `0x43D8C0`。

观察：第一参数是图层 ID，通过图层集合查找或创建；第二参数是资源名。四参数形式 `(id,name,x,y)` 调用 `FUN_00415ba0(layer,x,y)` 并调用 `FUN_00415b80(layer,1)`。三/五参数形式还有一个字符串资源参数，不能把所有第三参数都作为 x 坐标。复用已有图层时会清零若干状态字段。

图像请求 `FUN_00416860` 的保留证据为 `ev_c254f930b49fc438fdef23a94f207591d8815254bc75855d5a8f9459818c88bf`。它先把图层位置 `+0xE0`、`+0xE8` 清零；匹配专门的资源名前缀时可能按屏幕宽/高居中。标题的四参数形式显式覆盖坐标。

## YKG 动画组与图片

直接读取 `data02.ykc` 原版资源并调用 `build/yukari graphic FILE.ykg` 的观察：

| 资源 | 图像尺寸/通道 | 动画记录 |
|---|---|---|
| `system\damy.ykg` | 16×16 RGB，独立 Alpha 段 230 字节 | 无 |
| `title\title_bace.ykg` | 1024×576 RGB | 无 |
| `title\title_logo.ykg` | 464×237 RGBA | 无 |
| `title\title_btn.ykg` | 1240×248 RGBA | 40 条，每条 type=1 的 124×124 矩形后有 type=0 记录，共 20 组 |
| `title\title_alt.ykg` | 304×320 RGBA | 10 个 全零记录分隔组 |
| `system\logo\hook_soft.ykg` | 动画图集 | 20 条 type=4 帧，duration 为 32/128/256，末尾 type=0 |

由脚本与资源共同推断：`SpriteCreate(parent,slot,x,y,source,animation)` 的末参选择由全零记录分隔的动画组，不能直接索引 raw frame。标题按钮组 0..9 位于图集第一行，10..19 位于第二行，每格宽高 124。父层 30 只有 16×16 的透明占位图，精灵必须能绘制到父图像边界以外；把父图像尺寸当作子精灵裁切区域会丢掉标题按钮。

`hook_soft` 的 duration 值与 `logo.yks` 的 700 ms 等待吻合，但仅凭脚本不能确认 type=4 的精确播放/循环规则或 duration 单位，须由原版动画更新函数补证。

## 动画与提交的脚本约束

- `title_ani.yks:0x40` 将图层 4、5、6 的 `GraphicTransmittance` 设为 1000，随后对 5、6 使用 `GraphicTransmittanceScroll(id,0,700)`；`title_bace.yks` 同样对图层 25 使用 1000→0 的淡入。故此处参数是透过率：0 不透明，1000 完全透明。
- `GraphicScroll` 的标题调用是 `(id,duration,target_x,target_y)`，例如 `GraphicScroll(25,300,446,0)`；不能交换 duration 与坐标。
- `GraphicTransmittanceScroll` 是 `(id,target_transmittance,duration)`。
- 先 `DrawStop`，再加载/删除/创建图层，最后 `DrawStart`、`Transition` 或 `UserTransition` 的模式遍布启动脚本。因此需要保留已呈现画面与待提交图层状态，避免准备场景中途闪现。
- `GraphicScrollWait(5)`、`SpriteMotionWait(6,0)` 是异步动画的同步点；标题菜单前的 `MouseSelect` 应进入输入等待，不能直接顺序执行全部 MouseCase 分支。

透过率取值、参数顺序和提交用途目前有脚本证据；精确插值、转场模式枚举、type/action 全集、图层遮挡顺序仍需原版静态处理器确认。

## 本轮原版处理器交叉核对

以下均为共享 REA 静态会话导出的 `analysis/original-engine/title/*.c.txt`，不是运行观察。

| 处理器 | Evidence | 观察 |
|---|---|---|
| GraphicLoadHide `0x43DE80` | `ev_0e9cc607733b0823f49a820b98200c7eae5fdc35b2bcdb0dc06e09e66f2d102d` | 与 GraphicLoad 使用相同查找/创建和载入函数，调用 `FUN_00415b80(layer,0)` |
| GraphicShow `0x4361B0` | `ev_8bc92c505e9f62989db80b1084b94697c46d36cea6248ee5f43b21bf83d5d90a` | 查找已有图层并调用 `FUN_00415b80(layer,1)` |
| GraphicHide `0x436200` | `ev_fcc22afdf79ce0cd19bb260ed4dbc76b71c1ccdf6bffa26b4d4bc2e4cda344f2` | 查找已有图层并调用 `FUN_00415b80(layer,0)` |
| GraphicMove `0x435AF0` | `ev_d30eb6cd5687d7e634f2ec56fba9db130da0d421a8ec51e45e80317b7d9a258e` | `(id,x,y)`，调用 `FUN_00415ba0`，图层不存在时不创建 |
| GraphicTransmittance `0x435C00` | `ev_b46872243fa1a0c6394d053e76ae4bbd3fe18c58cf7cb92afb889f80be98e2d3` | `(id,value)`，将整数转 double 写入图层 `+0x1108` 并标脏 |
| GraphicDelete `0x436330` | `ev_80bf604bd615a1b80968349a3c58d2993e265eb7aae8e5dd9de1dc5c443c4cb4` | 隐藏图层后经 `FUN_00416540(collection,id)` 删除 |
| SpriteCreate `0x441890` | `ev_bbc17f076484d36cc85757c7c0432f4cd2426a2427bf6edb2da284fbb92180dd` | 精确参数为 parent、slot、x、y、source、group；先删除同 slot 旧精灵，再调用 `FUN_0040a5e0(sprite,source,group,x,y,2,1)`，加入父层并显示 |
| SpriteMove `0x436980` | `ev_ea10e051bc7b61d457c096091c786b10b44d53cb92f548265fda83ac527f0731` | `(parent,slot,x,y)`，查父层精灵 ID 后写 `sprite+4/+8` |
| SpriteMotionWait `0x436B70` | `ev_888d952a21185a32be4c0b360fba68f640537811aa7ffdc3d4d1a0d94a26236b` | `(parent,slot)`；mode 4/104 等待帧索引到最后，mode 105 等待索引回到 0；原版以 Sleep(10) 等待 |
| TextWindowSet `0x436CB0` | `ev_3b0f08ebe453040a0b5242590f4d15c4cf34291470ef922b8788f9ea95335c4e` | 选择活动文本图层；五参数形式使用 Win32 `SetRect(left,top,right,bottom)`，不是 x/y/width/height |
| DrawStop `0x436E70` | `ev_dab4aca79239ba36060d7ef5961983d7f245fcfca3c7300b62a017e5e9ddc257` | 准备/同步旧图像后设置全局 `DAT_005570dd=1` |
| DrawStart `0x436FC0` | `ev_5297268d3a5d7089f0a1864e8a84aeebf01e4a2de514d863da0506ec7ef01ef7` | 普通路径设置 `DAT_005570dd=0` 并请求重绘 |
| Transition `0x437000` | `ev_7a5a8657d96b0b3f2902b08daa123a6f8582608402a8be5e83c13bad64f36f6c` | `(mode,duration)` 转发给 `FUN_00425f70`；六参数形式另传 left/top/right/bottom 的 RECT |
| UserTransition `0x437130` | `ev_00d0076c836e8ccc99ddeac48596b7664d004d092d4829fd5d0a6a2cc51b8cd5` | `(mode,mask,duration)` 载入 mask 后调用 `FUN_00425e30`；两参数形式默认 duration=1000 |

`SpriteShow/Hide` 只切换所选父层/slot 的首字节可见标志；`SpriteDelete` 另调用删除函数。其 Evidence 分别是 `ev_6615613315a58cb9673f6ae95e09bc64abddbf9944fc7b9f161768ad0deb6275`、`ev_586ff39c39ecb6b23014c51bc84428d06587349d4118f91a4c3d10576075b99f`、`ev_30aec1111b94f6629c3c56151520c6c2e457aded902191200a586dc4b99dcc3f`。

补充原始资源观察：`damy.ykg` 的独立 Alpha PNG 为 RGB8，PNG 解滤波后全部像素为白色 255。结合该资源在标题中充当透明父层，可推断独立 mask 是白色透明、黑色不透明的透过率；最终方向仍应以原版像素合成函数为准。嵌入 RGBA 的 PNG Alpha 与该独立 mask 不应未经验证地使用同一极性。

进一步内部静态证据：

- `FUN_0040a5e0`，Evidence `ev_5f1a70a844fcb35ee182efcab43b645657f06020fecfa3f49bb5dc848572e46f`：源对象 `+0x3C` 是动画组数，`+0x40` 是各组起始索引表，`+0x44` 是各组帧数表。按 group 复制 `count*32` 字节到精灵私有数组，设置 group 与 frame=0，随后用 mode=2 调用 `FUN_0040a4f0` 应用首帧。这确认了组索引与 raw frame 索引确实不同。
- `FUN_00415b80`，Evidence `ev_3aa505854e5e2e09b0b4555e61a089b282ec474e336291546b58f1f65156dfa8`：可见性是图层 `+0xF8` 字节。
- `FUN_00415ba0`，Evidence `ev_a6325b16743e7b1c3de8293dd57dbbac653ac25a13ee6de9d42e940c0347de08`：显式移动会停止滚动，位置 double 写入图层 `+0xD0/+0xD8`，滚动速度清零。`+0xE0/+0xE8` 是另外的加载偏移字段，不能与显式坐标混为一谈。
- `GraphicScroll`，Evidence `ev_bcf90790d9c7d889f1676470ec620228d7a00c86a8e35b0b335ab8dbcb9e5e5d`：四参形式直接以 `(target-current)/duration` 写入 x/y 速度，设置滚动模式=1。
- `GraphicTransmittanceScroll`，Evidence `ev_5f226470d353cbbb6751c88ddb58fa67023eb3584493637f964a142c3d5ff1fd`：目标透过率写 `+0x1118`，每单位时间增量 `(target-current)/duration` 写 `+0x1110`，证实参数顺序和线性变化。

`FUN_00416a00` 实际负责图像延迟重载与颜色滤镜应用，不是动画表解析；它不能作为 全零记录分组规则的直接证据。`FUN_0040f570` 的颜色加载经 `FUN_0040f4e0` 派发，额外 mask 加载 `FUN_0040f770` 的 YKG/PNG 分别经 `FUN_0040ef40`/`FUN_004098d0`，mask 极性仍需这些解码/合成内部函数补证。

## 动画控制的最终补证

- `FUN_00409d00`，Evidence `ev_3d6236ef127900ae4debd1bc36e72cb9cb698b302012ef4af955846a63d3d180`：分组按**整个 32 字节记录全零**终结，不能只检查 type=0。末尾若无全零记录也会补算最后一组。起始索引与计数不包含全零终结记录。
- `FUN_0040a390`，Evidence `ev_97069aa329461fd0662fa67b268f7ed03cc543bcc146106e53d209b85364135f`：action=0 应用矩形与 duration，type 非零才覆盖当前播放模式；action=1 以 x 字段作为组内帧索引跳转；action=2 以 x 字段切换动画组、重置帧并复制该组。单帧组的等待值强制设为 INT_MAX。
- `FUN_0040a260`，Evidence `ev_4227966f6a662f2a592320688b66998631611e58cd34a925547794d4037222b9`：mode 2/6/102/106 正向循环；3/7/103/107 反向循环；4/104 正向单次，到末帧保持；5/105 反向单次，到首帧保持。故 hook_soft 的 type=4 是一次性正向动画，不能循环。
- `FUN_0040a4f0`，Evidence `ev_54711c85963b6cab8d0bc954d29b99305b9b10a631697f449cb67962ea39d916`：重置正向单次到首帧、反向单次到末帧；连续应用/推进零等待的控制帧，直到等待值变为正数。

本轮已答：标题脚本构图、精灵参数/替换、动画分组、主要播放模式与控制 action、图层可见/坐标/线性动画、绘制停止/恢复入口。部分回答：转场参数和提交入口。尚未回答：转场模式像素算法、独立 mask 合成极性、动画时间单位的计时器来源、完整遮挡/滤镜算法。

## 原版文本光标、LF 与 PF

2026-10-31 的补证只读原版 EXE：`TextGlyphPlace.json` (`0x415280`，Evidence `ev_171e82a975f69ba5bc6513bbe566cf675c2cc1e209c68eceeafdcd12539b9895`) 在 glyph 当前位置排版后，按字体固定格宽推进 x（DBCS 全格，SBCS 半格；格宽不正时用 glyph 位图宽），再把 cursor 复制到 anchor。随后若 `x + cellWidth >= rectRight` 就自动换到 rectLeft、y 加行高；这一步不更新 anchor。四个 rect 边全零时不自动换行。

`LFCore.c.txt` (`0x4166f0`，Evidence `ev_5e8907dbba021e42ecfea8ababa98ef6fa5cdcacaa95c95e4d7402318b2b2edf`) 只在 cursorX 等于 anchorX 时换行并追加 type7 零尺寸记录，最后总把 anchor 更新到 cursor。因此普通行末 LF 换行，刚自动换行后首次 LF 不会重复换行。`TextLFCore.json` (`0x415420`，Evidence `ev_c5e439e61b51ca5a56dfd5e2990302c14ccb66286ca59679b532477f25d6d0e1`) 证明 type7 仅记录新光标坐标。

`TextCursorReset.json` (`0x412d80`，Evidence `ev_5bf23d318086a22759fee960d520d7854cbd63a0e87e8c132424e4d6b1489b12`) 将 cursor 和 anchor 都复位到 rectLeft/Top；`TextClearCore.json` (`0x415890`，Evidence `ev_bfb1bbebd3076d5b7d36f9c15950a6a0a1b799a90cbc8a8b3f2aba840bee0fa4`) 调它并删除 type3/4/5/7，清 overflow。LF 不具有这种 PF 清屏语义。

`include/yukari/text_flow.hpp` 是上述光标运算的独立实现，`tests/text_flow_tests.cpp` 验证全半宽、比例宽、自动换行后 LF、重复 LF、PF 复位和底部溢出。实际 UTF-8 到原版 DBCS 的判定、禁则表、glyph 绘制仍由 runtime 负责，不能按 UTF-8 字节长度判断全半格。

## 原版连续图层运动（2026-10-31）

`instructions2/LayerMotionTick.json` (`0x417780`，Evidence `ev_5c79725822cb757e618ef2213867e43394bb0b0e529e00b0c82858f424396ce5`) 已确认为图层集合更新函数。`LayerMotionDeltaClockAssembly.txt` 中 `0x42a540` 调用 IAT `0x4a33a0`（PE inventory 确认 `timeGetTime`），相减得到 DWORD 毫秒增量；`LayerMotionDeltaForwardAssembly.txt` 中 `0x41a6e0` 原样转发给 `0x417780`。故不是“每秒速度乘帧数”的猜测：位置/缩放/旋转的有效步长为 `delta_ms * collection[+0x30]`，光栅使用原始 `delta_ms`。

`motion_model.hpp/cpp` 实现了 `GraphicScrollLoop` 的两种参数形式、`GraphicScaleScroll`、`GraphicRotateScroll`、`GraphicRotateScrollLoop`、`GraphicRasterScroll`，并提供原版直接 Scale/Rotate 的状态同步。关键边界规则如下：

- Scroll 仅在可见层推进；边界包含两端。任意轴越界使普通滚动停止，循环模式每个 tick 只加减一次跨度，保留越界距离，不做通用取模。七参数速度除以 1000，八参数速度为指定位移除以指定毫秒。
- Scale、Rotate、Raster 在隐藏层也推进。缩放每轴独立夹至终点。原版缩放标志 `+0x1132` 的特殊情况保留：首次 tick 已到非单位终点时，不会额外设为 true；若目标最终恢复单位缩放，则清为 false。直接 GraphicScale 不停止已有 scale scroll。
- RotateScrollLoop 持续使用 `(target-current)/duration` 的恒定角速度，无 ping-pong 和自动角度归一化。单参数停止循环会保留当前角度；直接 GraphicRotate 则清旋转速度与模式。
- Raster 首次启用设置 amplitude/frequency/period 并清 phase。已启用时的新命令只更改目标 amplitude 和渐变，不更改 frequency/period；新渐变以旧目标幅度开始，而非当前中间幅度。相位为 `phase += raw_ms/period`；quality<=0 时相位和幅度渐变都暂停。

`RasterRowAssembly.txt` 记录软件绘制的逐行算法：先截断 phase，再取 `trunc(sin((row+trunc(phase))/sourceHeight*frequency*3.141592)*amplitude)` 作为横向偏移。常数直接从原 PE `0x4a3950` 读得，非 2π。幅度为 `target - velocity * remaining`，remaining 为零则直接用 target。非单位缩放或非零旋转时，此软件分支不执行 Raster。

`tests/motion_model_tests.cpp` 覆盖毫秒/倍速区别、单次跨界回卷、隐藏层行为、缩放结束标志、恒角速度、Raster 原始时钟/重设规则/扫描线整数舍入及运动快照恢复；`tests/vm_select_id_tests.cpp` 另验证 SelectID 的 frame 隔离、未设置返回 -1，以及 native header `+0x48/+0x4c` 和 VM 快照往返。

### 普通 GraphicScroll 的完整参数形式

已补 `MotionModel::command` 的argc1/4/5/7/8，其余argc原样无动作。证据为4353d0 handler和 `instructions2/GraphicScrollAssembly.txt`。4参 `(id, duration, target_x, target_y)` 使用 `(target-current)/duration`。5参 `(id, denominator, magnitude, target_x, target_y)` 为 `velocity = delta * sqrt(magnitude²) / sqrt(dx²+dy²) / denominator`；原汇编435568加载arg2并平方，调用465090，其4650cc执行fsqrt，故负magnitude取绝对大小，并不反转。该形式不能误写为按目标距离除duration。

4/5参若两轴delta都为0直接return，已有scroll不被取消。边界按当前double截断成int32后生成：current_int<target时为 `[current_int,target]`，否则为 `[target,current_int+1]`；两轴分别生成，等值轴也包含+1。因此当前位置到达目标时仍保持mode1，直到下次tick越界才钳制并清mode。7/8参布局与GraphicScrollLoop一致，仅mode为1；argc1仅将mode2改为1。回归覆盖正向/反向分数坐标、5参3-4-5归一化与负magnitude、相等目标保留旧motion、7/8参以及恰好终点不停止；ASan/UBSan通过。
