# 原版专用节奏小游戏、脚本状态与系统弹出菜单

仅使用 game 原版 EXE 的证据。`GamePart_PlayMusic` 和 `NowScriptStateGet` 在现有全部 YKS 静态审计中均为 0 次调用；原版 YKC 索引也没有 `system\game\minigame_*` 资源，因此不能以 game 标题或剧情通过来声称该内嵌小游戏的显示验证完成。

## GamePart_PlayMusic

入口 `43cca0`（ev_b2f5363c9c9d2515f7fcd65eb335c17a8760361052cee8a4b540224b4a10c42b）要求至少一个文件名参数，无参数返回整数 0。参数顺序：乐谱文件、成功阈值覆盖（默认 -1 表示不覆盖）、速度除数（默认 6）、五位人物动画变体开关（默认 0）、背景覆盖（默认 0，正数才覆盖）。

`427550` constructor、`427620` loader、`427940` wrapper、`426bc0` gameplay 已导出至 `analysis/original-engine/instructions2/GamePart*.json`。Gameplay canonical 将返回类型误识别成 void；原版反汇编 `4274e1: mov eax,[esi+70]` 确认返回命中数，wrapper 将其保存 EDI 并最终返回。成功画面比较命中数与 success border，返回值不是成功布尔值。

新增 `game_part.hpp/cpp` 提供 chart parser、判定模型和屏幕坐标；`MusicGame::tick(elapsed_ms, keys, abort)` 由宿主每 10 ms 调用，返回命中/漏判事件，宿主驱动人物及效果动画。没有SDL依赖。

乐谱是逐行文本，保留原文件编码；关键字区分大小写：`BackGraphicType,`、`SoundFile,`、`SoundNameGraphic,`、`SuccessBorder,`、`MusicTotalTime,`、`StartDelay,`。其余行按 `lane,time,flag` 扫描；lane 1..5、time 非零才追加，每轨保持输入顺序。时间在读取该行时加当前 StartDelay；后续 StartDelay 不追溯修改前面音符。未显式设置 total time 时为最后最大原时间 + 最终 StartDelay + 5000。flag==1 的标记保留；原 gameplay 不读取该标记，不得把它猜成长按音符。原数组每轨容量 1000，新实现超出时拒绝，避免复现内存破坏。

人物初始位置 `(9,11),(79,151),(9,291),(79,431),(9,571)`，初始动画组为变体开启时 3、否则 0。每轨按键依次 `Z/S/X/D/C`；原版读取 GetAsyncKeyState，使用前次非零/本次非零的边沿。命中严格满足 `time-150 < elapsed < time+150`，且必须是新按下。恰好上界既不命中也不漏判，超过上界才漏判；每次循环每轨最多消费一个音符，不一次追赶所有旧音符。命中人物组为 base+1，漏判为 base+2（base=变体?3:0）。命中隐藏当前音符，播放该轨效果。`elapsed > total_time` 才结束；另有原引擎load/restart/quit全局标记可中断。

音符挂在 parent layer10 下：初始本地 y 为 `-(time + lane_offset)/speed`，offset `{-34,-3,0,-3,-34}`；parent y 为 `elapsed/speed+536`。仅绝对y=1..719且未命中的音符可见。五轨x依次为 `367+y*152/676`、`487+y*81/650`、`607`、`731-y*81/650`、`848-y*152/676`，整数截断。

专用场景资源与宿主接线：

| layer | 资源 / 用法 |
|---|---|
| 0 | `system\game\minigame_bg01.png`..`bg06.png`，背景值2..6对应同编号，其余01 |
| 10 | `system\null.png`，移动音符父层 |
| 40 | `system\game\minigame_effect.ykg`，轨道效果源 |
| 51..55 | `minigame_button_red/yellow/white/green/blue.ykg` |
| 61..65 | `minigame_otome/natumi/yuduki/megumi/saki.ykg` |
| 20 | 准备/失败提示 PNG，动态创建 |

效果位置 `(447,501),(502,492),(558,491),(617,492),(670,500)`，每轨复用一个效果精灵，开始隐藏，命中时重置并显示。所有上述文件除 black/null 均在 `system\game\`。

wrapper先保存原scene状态、黑场淡入淡出，再初始化专用scene。准备提示 `minigame_wait.png` 从 `(0,-720)` 在300ms移到 `(0,0)`，全窗口确认输入后300ms移到 `(0,-720)`。之后将 SoundFile 以原TAB分段流加载在声音1并播放，以 timeGetTime 为游戏起点。成功加载 `minigame_finish_good.png` 淡入；失败显示 `minigame_finish_bad.png`，y依次 `-720→0(300ms)→-50(100ms)→0(100ms)→-10(50ms)→0(50ms)`。停留3000ms，转黑并恢复原scene/上下文。SoundNameGraphic虽然被parser保存，实际426bc0未使用。

`game_part_tests` 已验证精确窗口边界、持键不重复触发、晚帧单步漏判、分数、变体组、初始几何与乐谱参数，ASan/UBSan通过。SDL专用scene现已由独立 MusicGameScene 接入并用 synthetic fixture 验证；game 没有这些原版资源，因此真实专属资源动画仍未验证。

`game_scene.hpp/cpp` 提供 `MusicGameScene(Scene&,Audio&,ResourceReader)`；
`begin(chart_name,options,now)`、`tick(now,keys,abort)`、`confirm()`、`render()`、
`phase()/finished()/result()/successful()`。render只画backbuffer，present由runtime负责。
父Scene及原音频上下文由runtime保存并在finished后恢复；专用scene结束于黑场。
原始资源缺失/空数据或动画组不存在均显式报错，没有占位图片或静默降级。

新增深层证据 `GameFadeOut42e560` 确认原wrapper开始时对声音1执行1000ms淡出；
`GameTransition42e5a0` 直接调425f70，故1000参数不是所有配置下都等于1000ms总时长。
scene用同一 `transition_timing()` / `render_transition()`：host通过
`set_transition_sampling()` 传入425f70质量缩放后的有效duration及quality/timing，
quality<0保留原wrapper跳过过渡分支。固定移动helper `GameMove42e5c0` 按位移/时间
设线性速度，准备提示与失败弹跳沿该速度实现。Gameplay若收到globalabort，仍进入
原Core的结果分支、3000ms停留和末尾黑场，不能把partial hits解释为成功布尔值。

`GameSprite42e770` 确认人物/音符实例创建的mode为1，最后参数是group；命中效果
初始使用group1并隐藏，命中调用原reset语义、保留group和mode，而不是猜成group0。
人物初始组、命中组和漏判组沿上述0/3变体映射。额外确认准备提示layer+0x1108写500，
失败提示写100，分别使用Scene alpha127/229；它们并非全不透明。
声音+0x2d=1是stream源类型标记，沿Audio.load_stream(1,TAB fragments,0)+play(1)，
不擅自开启循环。SoundNameGraphic在原Loop未使用，scene同样不加载它。

`tests/game_scene_tests.cpp` 在无窗口dummy驱动下构造原始YKG组、PNG和PCM WAV，
验证准备确认、五键判定接线、人物/音符坐标、命中隐藏/效果显示、good fade、bad弹跳、
半透明提示、globalabort仍显示结果及缺失资源拒绝。该fixture仅验证实现与证据映射；
它不证明game缺失的专用图片、音乐和动画素材已做真实运行验证。

## NowScriptStateGet

`DAT_0056ca70` 是BSS全局int，初始0；函数43bd10直接返回，不清零。原EXE绝对地址引用扫描只有三处写：4442e3、4452bc写0；44c350写1。前两处为文字等待处理器调用 item+44 的脚本后写0；后一处为4480c0选择调度器调用所选item+44脚本后写1。这些调用均先保存 `savedata.---`、ScriptCall并设置子frame+c9。普通同脚本的选择分支、普通ScriptCall/SubCall，以及其他菜单注入本身不写该全局。

VM已提供 `script_state()/set_script_state()` 与 `NowScriptStateGet` intrinsic。Yukari snapshot版本9保存该值，读取旧版本时保留当前值；reset与原版slot import保留当前值（原版slot不序列化该全局）。runtime应在上述两种事件脚本注入后分别set0/set1，不能在所有choose或call入口盲设。`vm_script_state_tests` ASan/UBSan通过。

## SavePopMenu / LoadPopMenu / FontPopMenu

主WndProc为410590，证据 `instructions/WindowProcedure.json`。439ed0/439f10/439f50分别发送8003/8004/8005。argc<2时使用已保存鼠标坐标DAT5570c4，wParam=8（TPM_RIGHTALIGN）；否则把前两参数按低16位x/高16位y打包，wParam=0。WndProc将客户区坐标转屏幕坐标，TrackPopupMenu显示 `this+1f8/+1fc/+200` 对应菜单。命令本身不执行save/load/font修改，也不返回操作结果。

WM_CREATE创建十个slot，显示01..10；保存命令ID48001..48010，读取49001..49010。WM_COMMAND保存需保存许可DAT5587d0且非DAT56c338阻止状态；弹确认后暂停VM事件，取窗口标题为描述。若 VM+80118 为1或2，或当前frame+c9已置1，调用4333a0(0,slot)再4335a0(slot,title)保存缓存上下文；否则443030(slot,vm,title)实时写。结束恢复VM事件。读取命令确认后设置DAT56c344=slot、DAT56c340=1并通知子控件销毁；实际load留主调度器处理。

字体菜单由EnumFontFamiliesExA生成，charset=0x80（SHIFTJIS_CHARSET），callback40fb10过滤以`@`开头的纵排字体；每次枚举后递增序号（被过滤字体也递增），命令ID为47000+序号。WM_COMMAND47000..47999读取菜单文字，并写入原全局LOGFONT数组的16份font face，然后405a70重建字体。因此原版FontPopMenu修改全部16 font slots的字体族，不只是当前文本层；不会自动改字号、字重等其余字段。mac字体列表需按宿主实际字体可用性映射，菜单可复用同一字体族选择逻辑。
