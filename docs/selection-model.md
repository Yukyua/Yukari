# 原版选择模型与宿主接线

本模块只使用原版证据，独立于SDL、runtime和CMake。`selection_model.hpp/cpp` 将注册声明转换为有界数据，并提供候选查询与视觉状态差分。原函数4480c0证据为 `seeded/scheduler-full.c.txt`；深层证据在 `instructions2/Selector*.json`、`MouseKeyCaseSetup.json`、`KeyTableCaseSetup.json`。

## 注册

`decode_selection_case(HostCall,item,SelectionRegistration)` 返回Added、ConditionFalse、NativeUnrecognized或Unhandled。HostCall需带VM提供的block_begin/end。新增项按声明顺序插入SelectionModel；原418520按item序号稳定排序，本模型要求宿主保持同样顺序。不可把未注册声明当可点击项。

* Case2：arg0=条件，arg1=文字；假条件不注册。可选arg2/3是文本绘制坐标，arg4..7是明确点击范围，arg8/9是进入/选择声音。无明确范围时宿主必须依据原文本布局计算text_rect，模块不会猜测字号或文字宽度。Case2文字normal/hover样式由宿主现有选择文字服务生成。
* MouseCase2Already：arg0=启用条件，arg1..4为范围，arg5/6为父层/源层，arg7/8为坐标；未读normal/hover/disabled组为arg9/10/11，已读为arg12/13/14；arg15/16为进入/选择声音。禁用项保留视觉，但不参与正常点击。
* MouseKeyCase：arg0=1左键、2右键、3中键、4滚轮正向、5滚轮负向。可选arg1..4范围；arg5/6为父层/源层，arg7/8坐标，arg9/10普通/指示组，arg11/12声音。无范围时为全区域。原4188e0对全零范围特殊地视为全区域。
* KeyTableCase：最多16个VK，第一参数为0不注册；418450把输入倒序后去掉0。4187a0将倒序序列与最近优先的按键历史比较。因此`KeyTableCase(A,B,C)`代表按下A、B、C的有序序列，不是三个键同时按住。
* WindowMessageCase：arg0为原Win32 message号，候选忽略wParam/lParam。418a30扫描整个链表，返回最后一个匹配项，并且不检查blocked。
* IndicateUsualCase：type5、无限范围、blocked/navigation-excluded，仅提供指示动画，不作为候选分支。arg0..4=父层/源层/x/y/group，arg5=进入声音。
* IndicateUsualTimeCase：type5，arg0=附加到sprite group前的等待帧duration，arg1..5=父层/源层/x/y/group，arg6=进入声音。该Time不是退出选择或调用分支的timer；模块保留为SelectionSprite.delay_ms，宿主必须复制group并前置等待帧。
* TogetherIndicateCase：type6，无独立候选；arg0..3=父层/源层/x/y，arg4/5=普通/指示group。它跟随链表中紧邻前一个注册item的focus，与前一个“可点击项”不等价。
* KeyCase：虽然dispatch有该声明名，4480c0完整调度器没有对该字符串的识别。模块返回NativeUnrecognized以明确保持原版无注册行为；未把它推测为KeyTableCase或单键别名。type2普通键数据结构和418710真实匹配仍有独立API数据可供宿主原生配置使用。

## 输入查询

`SelectionInput.query`明确指定MouseInput、KeyDown、KeyHistoryChanged、WindowMessage或VisualOnly。候选只返回item/branch/end，宿主决定调用VM.choose还是interrupt_selection，模型不自动结束或恢复选择。

原418f50事件入口的离线汇编419058..4190ac表明：WM_KEYDOWN时，bit30=0才把VK插入最近优先16键历史；重复消息不更新历史，但仍先查询历史序列；没有命中才查询418710普通键，最后418800导航/关联组合逻辑。宿主应每次WM_KEYDOWN先KeyHistoryChanged查询（包括repeat），再KeyDown查询。Input.key_history由宿主的窗口事件收集器提供，本模块不在鼠标移动或空tick时反复尝试该序列。

普通type2键418710取第一个非blocked、含VK的item；repeat默认因lParam bit30被抑制，allow_repeat=true时允许。历史序列4187a0取第一个匹配项。MouseKey4188e0也取第一个匹配；1..3默认只认WM_xBUTTONDOWN消息，允许hold的原item字段+9c为真时改看mouse_held；4/5认WM_MOUSEWHEEL的有符号高16位。声明MouseKeyCase本身不设置+9c，宿主不要把SDL重复/鼠标保持擅自变成该标记。

普通点击与hover使用`hit(x,y)`：原4186b0采用**包含右边/下边**的范围；默认扫描到最后一个重叠可用type0项，遇到+cb标记才提前停止。它并不等同SDL_Rect的右下排除规则。原MouseKey单独的type1扫描与普通type0 hit不可随意互换。

## 视觉/音频差分

`update(input, focused_item)`输出SelectionChange(item,slot,visible,reset)，slot=normal/indicated/disabled。宿主先按SelectionSprite创建真实sprite，再依差分显示、隐藏或重置动画。type5项独立看鼠标范围，全部同时更新，不需要成为focus；其余项按focus更新。Together跟随前一注册item。声音输出包含进入播放与离开时loop声停止；select_sound仍作为item数据保留，宿主应在真正接受候选时播放，不能在候选查询时先播放。

本模块未替代4480c0的动态Case2字体创建、SDL输入到原VK/Win32消息映射、原选择器特殊skip标记、批量声音管理与定时中断调度。type7/8与导航在下节第二阶段中实现。它们必须继续由宿主服务接线，不能以“声明已collect”声称实际行为完成。动态资源存在性应由宿主Loader验证。

`selection_model_tests`覆盖：有序历史/非组合键、repeat门控、右键及滚轮、范围右下端点、重叠取末项与stop标记、窗口消息最后项/blocked差异、Case2条件/文字范围、已读三组替换、UsualTime前置等待、Together关联前一注册item；ASan/UBSan通过。

## 25条声明、选项与导航的第二阶段

新增 `SelectionDeclarations` 有状态parser覆盖VM `case_command`的全部25名。它包含model、navigation、decorations、timers；`consume`返回Added代表已消费声明，**不代表一定新增item**，宿主应比较`model.items().size()`。KeyCase仍按原SelectCore未识别报告NativeUnrecognized。普通Case/MouseCase/MouseCase2/IndicateCase现在也可直接解码，保留文字、矩形、sprite组、声音和flags。宿主原有Case文字服务可以继续使用，本模块不代替字体排版或源层资源存在性检查。

SelectDisable无参数清零，有参数取首值非零。它只改变后续MouseCase/MouseCase2的navigation_excluded(+bc)，不会将它们blocked，也不禁止鼠标点击。TabGrurpCheng设置Tab组导航标记，不分配item。KeyOverPrev/Next插入type0零矩形节点，分别标记+cd/+ce：顺序导航抵达对应方向的标记时立即返回activate=true，宿主应执行其branch，而不是只改变focus。

IndicateString/C产生静态文字装饰（C将x减去最终sprite宽度/2），仅原文本渲染模式0时生成；AlreadyIndicatePreceding产生静态sprite装饰，4参复用之前Case文字坐标，6参有明确坐标，按之前选择项的already状态选两个组之一。装饰不参与focus/Together邻接。Case显式坐标为非负时优先，否则调用者可通过text_position提供原排版后的坐标。TimeInterrupt/IntervalInterrupt解析为timer描述，仍由宿主按原VM interrupt服务执行，不能使用普通choose结束选择。interval后续deadline应增加原interval及回调耗时，详见4480c0末段，不能将callback耗时视为全局时间暂停。

### SelectingKey与type7/8

SelectingKey(arg0, key...)将余下最多16键按原418450倒序并压零，item.kind=Selecting(7)，arg0是**从本节点沿链表前/后移动的相对节点数量**，并非目标CaseID、branch word或减去不可点击项后的序号。418800同时支持type8，本模型提供SelectingAlternate(8)以供宿主原生配置创建，25声明内没有另一个把kind置8的名字。

Input.query=SelectingKey时需提供focused、wparam当前VK及raw key_state。只有偏移指向当前focus才可触发；第一个倒序键等于当前VK，其余键按原GetKeyState结果检查。原汇编41889d执行`and al,0x80`，因此接口检查的是SHORT**低字节bit7**，未擅自改成常见文档中的高位0x8000。不同平台映射必须由宿主明确处理；这个事实本身不能证明Windows真实GetKeyState返回值必定导致多键失败。单键SelectingKey不查额外状态，repeat不被此函数抑制。证据为 `SelectingKeyAssembly.txt`。

### CursorMoveTypeIndex/Auto及navigate

这两条并非修改全局mode：4480c0插入type9/10节点，参数会求值但结果未使用。418f50从当前focus向前扫描最近此类节点，Index强制该段mode1、Auto强制mode0。Auto候选范围从最近marker后开始，到下一marker结束。节点必须保留在同一链中，否则Together/SelectingKey关联偏移会变化。

`SelectionModel.navigate(vk, focus, options)`提供以下原逻辑，返回focus及需要立即执行KeyOver边界分支的activate标记：

* 默认mode1按节点顺序前/后寻找kind0、非blocked且非navigation_excluded的项，抵达末端不循环；无focus时向后取首项、向前取末项。
* mode0按矩形中心选目标方向最近项，垂直方向代价`dy²*0.5+dx²`，水平方向代价`dx²*0.5+dy²`，同代价保留先遇到者；只改变focus，不执行branch。
* mode2按每项neighbors(up/down/left/right)链查找，跳过disabled/excluded项；邻接必须由宿主提供原值，不推测生成。
* Tab组模式从当前位置向后寻找KeyOverPrev标记，末尾重新从链头查组；选该组内部首个普通项，不把KeyOverNext当内容项。

41acf0 constructor于41ada5明确将mode设为1。原默认prev表为UP/LEFT/NUM8/NUM4，next为DOWN/RIGHT/NUM2/NUM6；up/down/left/right各包含方向键及相应数字键，已作为SelectionNavigationOptions默认值。证据为 `SelectorConstructorAssembly.txt` 和 `SelectorWndProc418f50.json`。与KeyTableCase不同，navigate处理的是宿主提供的单个VK，不维护按键历史。

鼠标flags保留clickable_cursor(+c8)、native_c9、ignore_input(+ca)、navigation_excluded(+bc)、stop_hit_scan(+cb，MouseCase专属bit0x40)、loop_enter_sound(+cc)。原候选helper本身不排除ca，418f50在接受候选时才检查；宿主必须据ignore_input拒绝激活，不能偷偷改变原helper查询顺序。MouseKeyCase额外flags bit0x20设置allow_repeat/hold(+9c)。

新增 `selection_options_tests` 验证默认模式/表、Index不循环、空间分段/权重、显式邻接跳过禁用项、SelectDisable不禁止点击、Tab状态、定时描述、文本装饰/既读组、SelectingKey相对关联和raw键状态quirk、type8、KeyOver边界激活、MouseCaseflags。与原selection_model_tests均通过ASan/UBSan。宿主仍需把这些新API接入事件流、创建装饰资源并执行timer；不能仅依据parser通过报告UI全部完成。
