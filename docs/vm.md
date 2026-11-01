# Yukari 脚本 VM

2026-10-31。后续已接入窗口图层宿主并完成标题启动，见 [标题运行时](title-runtime.md)。下文的服务边界描述保留基础 VM 建立时状态。实现文件为 `include/yukari/vm.hpp`、`src/vm.cpp`。仅以 参考样本 原版 EXE 为语义来源，未执行或分析汉化程序/补丁。

## 已实现的执行行为

- PC 使用代码区 DWORD 偏移；所有控制跳转必须落在指令边界或脚本结尾，拒绝跳入参数。
- Flag/String 和 GlobalFlag/GlobalString 存于 VM 共享状态；每次脚本调用（包括同文件标签调用）都重新创建局部变量及数据区副本。
- 局部值可从整数变为字符串，采用类型化 Value；不直接模拟原版的危险裸指针。未赋值局部变量的读取明确报错。
- 赋值目标在随后函数返回时写回；`=()` 本身不调用赋值处理器，会保留目标给后面的函数返回。无返回对象则不改值，并清除目标。
- 保留解密后的脚本数据区。整数常量写入会影响共享该偏移的其他整数、变量 ID 或控制链接；不把数据区常量全部当作不可变立即数。
- 整数 `* / %` 优先于 `+ -`，再高于 `= < >`；同级左结合。加减乘按 32 位回绕，除法采用有符号整数截断。原版显式将除零结果置零；余零及 INT_MIN/-1 被本实现拦截为 VM 错误。
- 字符串支持赋值、拼接和 `=` 字节相等比较。多参数拼接为空时返回无对象，单个空字符串则会赋值为空。表达式不自行加入未证实的 C++ 运算符或类型转换。
- `if`/`If` 为假时跳到随后 `{` 的链接；`else` 跳到随后 `{` 的链接加一，保留原版编码差异。
- `ScriptCall` 单标签或文件+标签创建新的脚本上下文。`ScriptJump` 单标签改变当前 PC，跨文件形式创建子上下文并结束来源上下文。单文件字符串调用从文件起点开始。
- `Return` 结束当前上下文，可生成整数返回值。原版 ScriptCall 返回 NULL，上层调度弹出子上下文时不传递子 Return 值；Yukari 同样不转发。`last_result()` 仅提供根 Return 的诊断值。
- 原版共用 `0x4386D0` 处理器的 Case 家族，在普通解释执行中跳过其后 `{` 块。宿主的 SelectionModel 收集声明，VM.choose 执行所选分支，interrupt_selection 执行定时事件后恢复等待。

不支持的类型、运算、未初始化值、错误参数、超限或未实现服务均失败并给出调用栈。局部变量最多 65,536，调用深度最多 256，宿主参数最多 128，单字符串最多 16 MiB。变量索引比原版某些越界一项的检查更严格，不复现危险内存越界。

## 宿主边界

VM 自身不依赖 SDL。Loader 返回解析后的 Script，Host 接收函数名、脚本/word 位置及类型化实参：

- `std::nullopt` 表示服务未实现，VM 报错并保留调用位置。
- `HostResult{false, value}` 完成调用并写回目标。
- `HostResult{true, {}}` 挂起。`run()` 此后持续返回 Waiting，不重复调用 Host。
- `resume(value)` 完成挂起调用的目标写回和 PC 推进，再次 `run()` 才继续执行。

这使后续 SDL 窗口、输入、逐字输出和音频等待能接入事件循环，无需把等待实现成阻塞死循环。已提供选择回调、事件中断、脚本注入、VM v9 快照及原版存档上下文导入/导出；完整引擎调度由 runtime 的 SDL 事件循环承担。VM 本身不提供多线程并发执行。

`run(step_limit)` 的预算是本次执行的指令数；到限返回 StepLimit，可继续 run。执行步数包括 target 和 label。未知服务不会作为成功的空函数处理。

## 原版证据

先前固定目标身份、解释器、运算器、if、ScriptCall/Jump/Return 见 [原版报告](original-engine.md)。本轮补充：

| 观察 | 地址/结果 | REA Evidence |
|---|---|---|
| Case 家族普通解释路径 | `0x4386D0` 检查 next `{` 并跳其 link | `ev_9370d12e9a9e7c764f2d5c5f785cd901c738463452d33aa97cc49753fba57ba2` |
| 上层调度 | `0x44CCA0` 执行解释器，PC 到结尾时调用 context-pop | `ev_18b7790f263cc356d873ebb5753e766672de0d3007eaa6dbc4d37630c9b5f454` |
| context-pop | `0x441EB0` 解除双向链接、释放上下文，不传播返回值 | `ev_6b9713ed372a1173d26481532f62266ad161106b06e43f0ce20f29580e0c08f6` |
| ScriptCall 返回 ABI | 尾部 `XOR EAX,EAX`，返回 NULL | `ev_819b57113dd5eda9d749776d9d682117faf70c1d9ff229d818bbe86ce2fbf6d0` |
| 局部槽初始化 | `0x432810` 将 tag/value/pointer 三 DWORD 置零 | `ev_a2ef2b2007f5e148975b2712dac684d9bfaa5839d5950ea71b932d7e2e3c73ac` |

同时调查但有意未伪造：`ScriptSubCall` 先调用存档例程（参数 `savedata.---`），再调用 ScriptCall 并标记上下文，因此不能直接当普通 ScriptCall；`ScriptExit` 会发送窗口消息并影响上下文链，留给引擎宿主。对应 Evidence 为 `ev_47f780c8fa8701b457c63ca0c01c351e0b68a2fbefa93ca680fba79f5566e958`、`ev_e2fcbf93752e50bfc04dcac92e8558c390c02a95e0087fb8b832ba7accf691d7`。

详细结果保存在本地忽略目录 `analysis/original-engine/vm/`，包括完整 session Evidence。REA 会话已关闭，原版 EXE 不变。这些为静态代码观察和实现测试，不是运行原版程序做过的行为对比。

## 验证结果

`vm_tests` 为不含商业样本的 C++ 测试，覆盖优先级、左结合、回绕、条件分支、调用隔离、宿主返回/挂起、错误定位、步数上限、字符串规则和可写数据区别名。

`vm_game_tests game` 显式读取原始样本，无宿主桩；各场景从独立 VM 开始，预置哨兵并检查未选分支未改写。16 个场景：

| 原版脚本 | 场景 | 动态指令步数 |
|---|---|---|
| `yks\system\autoskipclear.yks` | Flag10=0/1/2/3 | 15 / 17 / 19 / 12 |
| `yks\root\state_chk.yks` | 无匹配、Flag1500…1550 的首个匹配 | 26；9 / 13 / 17 / 21 / 25 / 29 |
| `yks\system\optiondefault.yks` | Flag100=0/1/2/3/4 | 108 / 39 / 57 / 41 / 17 |

全部场景核对变量结果、未改写变量及步数。Debug 与 ASan/UBSan 构建下验证通过；原有格式及图像回归通过。

原始 `start.yks` 能执行初始赋值、SETUP_BOOTCHK 的标签调用与 Return、SETUP_SYSTEM 的嵌套调用和窗口名拼接。随后在 `yks\system\OptionSystem.yks:word=0xA` 的 `WindowNameSet` 明确停止。CLI 尚未接入窗口/图层宿主，不能据此宣称已运行完整标题或剧情。


## 快照与原生存档扩展

`Vm::call/in_call` 支持在宿主等待点注入菜单，返回时恢复原等待。`snapshot/restore` 保存调用栈、可变脚本数据、locals、待赋值目标、选择和全部银行；原生 `import_original/export_original` 与容器codec分别处理VM与格式。详细证据、版本与互操作边界见 [存档格式](save-format.md)。
