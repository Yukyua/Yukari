# 参考样本 VM 脚本观察

2026-10-31。只读分析原始 `game/data01.ykc`，结合已保存原版 EXE 的 Ghidra 证据；未读取汉化 EXE、CNP 或补丁。以下脚本偏移均为代码区 DWORD 索引。

## 启动最早的引擎服务

这里将 `=`、`if`、`ScriptCall`、`ScriptJump`、`Return` 视为 VM 核心；其他调用仅列出脚本中的实参，不假设服务已经实现。

`start.yks` 首先设置 GlobalFlag:1000、GlobalString:0/1，依次调用 SETUP_BOOTCHK 和 SETUP_SYSTEM。前者只 Return，后者在 word `0x2B` 调用 `yks\system\OptionSystem.yks`，对应包内小写条目 `yks\system\optionsystem.yks`。

因此，在该静态启动路径中最早遇到的非核心服务是：

| optionsystem word | 函数 | 实参 |
|---|---|---|
| `0x0A` | WindowNameSet | `$0` |
| `0x0D` | Index | `GlobalString:0` |
| `0x10` | GraphicLoad | `0, "ykg\\system\\white.ykg", 0, 0` |
| `0x16` | SaveEnableMode | `0` |
| `0x19` | LoadEnableMode | `0` |
| `0x1C` | SubMenuEnableMode | `0` |
| `0x1F` | SelectTransitionSet | `2, 10` |
| `0x23` | ScriptSkipMode | `1` |
| `0x26` | SelectFunctionSet | `255` |
| `0x29` | MouseSelectFunctionSet | `0` |

`WindowNameSet` 前的表达式为 `$0 = GlobalString:0 + " " + GlobalString:1 + GlobalString:2`。脚本本身未在此之前设置 GlobalString:2，因此仅凭该脚本不能断定最终完整窗口标题。

后续设置 GlobalFlag:500=250、Flag:10=2，并在 `0x34` 调用 AutoSkipClear。该脚本仅使用赋值和 if。返回后继续 SaveDataLoadToCallScript、RGB、SaveDataDisplaySetToFont/Graphic/Date/Comment1/Comment2。RGB 前出现 `$2` 目标及零参数 `=`，其返回值消费不能只按单条赋值语法处理。

这是一条静态可达顺序；未运行原版程序，未将默认全局状态或各服务副作用当作已验证行为。

## if 与 else 的真实编码

遍历全部 571 脚本的执行流：

| 项 | 类型 | 次数 |
|---|---:|---:|
| `if` | 0，函数调用 | 73,122 |
| `{` | 1，标签 | 73,894 |
| `}` | 1，标签 | 73,894 |
| `else` | 1，标签 | 253 |
| `while` / `function` | 0 或 1 | 0 / 0 |

不能将所有括号都当成 if：标题脚本中的 MouseSelect、KeyTableCase、IndicateCase、MouseCase 等也后接括号结构。

`start.yks` 的第一个 if：

```text
0x31  target $0
0x32  = (GlobalFlag:60, operator "=", 0)
0x37  if ($0)
0x3A  {  link=0x42
       ... body ...
0x42  }  link=0x3A
```

原版 [Yuka_If](../analysis/original-engine/seeded/function-if.c.txt) 在条件整数为零时，确认当前 next 指向名为 `{` 的元素，将 context+0x10 设置为该元素的 data-backed link 值。其自身不处理 else。

首个 else 在 `yks\extra\cg\cg_mode_ai.yks`：

```text
0xD4  if ($0)
0xD7  {     link=0xE6
       ... true body ...
0xE4  }     link=0xD7
0xE5  else  link=-1
0xE6  {     link=0xF3
       ... false body ...
0xF3  }     link=0xE6
```

可直接确认：首个 `{` 的 link 指向 else 的 `{`，而不是 true 分支的 `}`。条件为 false 时 If handler 会跳到 `0xE6`，绕过 `else` 标记本身。条件为 true 的路径正常走到 else 标记，需要解释器跳过 false 分支，不能把 else 当普通标签忽略。

补充交叉核对：[已保存解释器](../analysis/original-engine/function-dispatch-loop.c.txt) 的非调用 type-1 路径比较 `DAT_004A4C7C` 的 5 字节内容，匹配后检查下一项 `{`，将 next 设置为它的 link+1。该代码与上述 else 布局一致；随后读取原版 EXE 的文件偏移 `0xA4C7C`，确认五字节原始数据为 `65 6C 73 65 00`（`else`）。

## 循环与函数体

原始执行流没有名为 `while` 或 `function` 的 type-0 调用或 type-1 标签。不能凭此宣称引擎不支持源语言 while/function；它们可能在编译前消除，本次无源脚本证据。

实际循环可见 `yks\title\title_main.yks`：`0x29E`、`0x30B` 调用 `ScriptJump(:START link=0x71)`，`0x2A5`、`0x319` 调用 `ScriptJump(:SETUP link=0x33)`。这是直接的命名标签跳转。

子过程实际使用普通标签配合 ScriptCall 和 Return。例：`optiondefault.yks` 的 OPTION_ALL 标签位于 `0x3A`，依次在 `0x3B`、`0x3E`、`0x41` 调用 OPTION_SYSTEM01、OPTION_SOUND、OPTION_ETC，`0x44` Return。不存在该处额外 function 标记。

## ScriptCall 的局部变量存储

原版证据支持 **每次 ScriptCall 创建独立 context 和独立 locals 存储，不与调用者共享 `$n` 数组**，包括对当前脚本命名标签的 ScriptCall。

证据链：

1. [Yuka_ScriptCall](../analysis/original-engine/seeded/function-scriptcall.c.txt) 对字符串脚本调用走 `FUN_0043F140`；对当前文件标签也先调用该函数，传入当前 context 的脚本文件名，再搜索新 context 的标签。
2. [FUN_0043F140](../analysis/original-engine/seeded/function-script-invoke.c.txt) 调用 context push `FUN_00432AD0`，再调用脚本 loader `FUN_0043CF20`。
3. [context push](../analysis/original-engine/seeded/function-context-push.c.txt) 分配并清零 0x120 字节的新 context，挂接前后链。它从旧 context 复制的仅为 +0xC4、+0xC8、+0xC9；没有复制 +0x2C locals 指针。
4. [loader](../analysis/original-engine/seeded/function-script-load.c.txt) 128–142 行将头部 locals 数量写入新 context+0x30，为每项 0xC 字节分配并运行构造器，数组指针保存到 context+0x2C。
5. [解释器](../analysis/original-engine/function-dispatch-loop.c.txt) 的 395–401 行从当前 context+0x2C 按索引×0xC 读局部值；657–685 行写入对应存储，按整数/字符串结果维护 tag=1/2。

原始 YKS type-10 在写入后会被解释器改写为 type-11（整数）或 type-12（字符串）。因此这些类型可作为运行时缓存出现；它们在当前原始包的初始元素表中没有出现。

范围限制：尚未读取局部值构造器 LAB_00432810 的独立反编译，因此本报告不指定未赋值局部变量的初始整数/字符串类型。Return 的值对象如何转交调用者、栈弹出与 caller 恢复也需要完整调度器证据；不以当前 Return 片段中的反编译返回类型推断 ABI。
