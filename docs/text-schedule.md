# StrOut 多层逐字调度

结论：当前按layer顺序完整显示一层再显示下一层的runtime实现与原版不同。原445ed0明确轮转：每个完整round按声明顺序对每个target调用4455f0 **一次**，然后统一检查所有字符串是否结束，最后Sleep(TextSpeed)一次。不是每层字符各Sleep一次，也不是每层一口气完成。

## 证据

* `instructions/handler-0x445ed0.c.txt`，ev_20274ab56cfded6cb18fafa2994deb9c0684885640a82e2c5f2e7990996647ef：单参使用当前text layer，更多参数按(layer,string)两两成组，奇数尾参数不会成为额外target。每组有独立native frame+44+i*4偏移；同一个layer ID重复出现仍是不同target，按声明顺序各处理一次。
* `instructions/TextOutputCore.c.txt`，ev_476b05bd7c36d9df622b7d4f9069347c0b4989c8196cd92e3135469109f95853：4455f0是一次输出操作，通常consume一个SBCS或DBCS字符，控制码/禁则及回退可一次consume多个字节。调度器必须采用它返回的消费长度，不能按UTF-8 codepoint或固定每字节推进。
* `instructions/handler-0x446150.c.txt`：StrOutNW临时把TextSpeed置0后调用同一StrOut，再恢复，未绕过轮转。
* `instructions/handler-0x446190.c.txt`：StrOutNWC在首target偏移为0时对所有target分别计算居中位置并同步cursor与anchor，然后临时TextSpeed=0调用StrOut。NameOut446380只是将name layer ID置于参数首，转交NWC，不是独立整句打印路径。

例如 `StrOut(10,"ABC",20,"xy")` 的正常round为 `(10:A,20:x)`→Sleep→`(10:B,20:y)`→Sleep→`(10:C)`→Sleep。**最终round后仍会Sleep**，直到该次native StrOut返回，VM才继续下一条指令。已经结束的target，原4455f0空输入无动作；新helper省去这些空回调，不改变输出效果。

## 快进与延时

原StrOut开始时以首target偏移==0判断初始化，而不是“本instruction第一次调用”独立flag。它清DAT56db10，然后在skip predicate成立、TextSpeed=0、current_instruction_already(80111)且already_text_fast(80112)、或单参layer+12b9标志任一成立，并且原drawing-deferred(5570dd)为false时，获得该指令的defer所有权：5570dd=true且56db10=true。结束时只有拥有此所有权才清defer并flush；不能清掉调用者原本已设置的defer。

每round结束时重新查询4338f0 skip predicate。仅在 skip_now=false、TextSpeed!=0、56db10!=true、单参layer+12b9=false时，flush并Sleep(DWORD TextSpeed)。fast/NW只取消round间Sleep，仍然按target轮转；控制码与同layer重复target的顺序也不能改变。首target为空导致offset持续0时，原版会反复执行初始化，这一quirk需宿主在需要逐字节等价时保留。`text_schedule_latch`返回该次初始化是否能获得defer所有权，宿主仍负责BSS标志和flush生命周期。

4338f0不是简单的全局skip bool：至少还检查当前frame+c4非零，才允许skip来源（script runtime80110、普通skip56c31c、既读skip56c31d且当前80111、鼠标输入计数变化、前台窗口且skip-key配置、56c912）。已有Settings/VM服务应继续提供该predicate，调度helper不猜配置。原TextSpeed为负数传Sleep会按DWORD转换，helper不会擅自clamp成1ms。

slow路径还比较窗口输入计数57290与该指令保存的56db0c，变化时递增鼠标输入计数57258；每round尾部56c904都加56c900。这两种原全局副作用在fast/slow路径条件不同，宿主若已有对应计数应按原位置更新。

## 控制码与分页

4455f0识别`@r`、`@m`、`@gf`、`@of`，分别转交4348a0/434af0/434ca0/434e10；这些不是额外的round。纯调度器不重新解释它们，而调用已有TextModel.output_step，并采用其完整消费长度。缺失目标layer时原4455f0扫描到字符串末尾并返回剩余字节数，不会为缺失ID自动新建layer。若宿主需复制该行为，应由emitter直接consume剩余bytes。

4455f0在当前layer overflow时会**同步阻塞**在KeyWait；本round后续target尚未调用，不能先输出它们。确认后PF清的是**VM当前默认text layer ID(ctx+0xc)**，随后直接重置实际输出target的cursor到其rect左上并清overflow；当显式target不同于默认layer时，这两步不等价于clear(target)。原target旧records是否消失由PF实际选中的layer决定。代码还临时保存/恢复运行时history string80144及原defer标志。

因此适配为异步宿主后，应在emitter返回0时保存本round游标，等待分页服务，确认并处理PF/cursor后重试**同一个target、同一个byte offset**，不重输出本round前面的target；这个过程不额外插入TextSpeed等待。

## 独立模块与测试

新增 `TextSchedule` 保存target(layer,encoded,offset)、next_target和complete。`cycle(emit)`只执行一个完整round；返回RoundComplete及final_round、或PageBlocked、或Done。PageBlocked保留游标；RoundComplete永远将游标归零。最后round返回final_round=true后，宿主仍需执行一次策略等待，再将VM恢复；下次调用返回Done，不重复延时。

`state()/restore(State)`可沿现有runtime文本状态序列化，包含中途分页游标及每层offset。restore有长度/偏移/完整性限制并先验证后替换。一次consume超过NUL边界会拒绝。helper不将文本解码为UTF-8，也不拥有SDL、字体或VM。`text_schedule_delay(policy)`只在RoundComplete后使用。

`text_schedule_tests`验证不同长度target交错、同ID重复target、最终round等待、第二target分页后恢复不重输出第一target、控制token/DBCS消费、NUL边界、空参数round、fast latch/defer所有权与无效恢复，ASan/UBSan通过。已接入 runtime pump_text；即时输出也保留轮转，合成集成测试对同一目标层的 AB/xy 验证结果为 AxBy。分页继续保留当前 round 游标。
