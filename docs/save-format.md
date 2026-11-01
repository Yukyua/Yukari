# 存档格式、VM 快照与互操作范围

2026-10-31。实现依据为原版 game.exe 的静态反编译和提供目录内已有存档的只读检查。没有读取或分析汉化 EXE、CNP 或补丁 DLL。

**原版格式目前通过结构解析、字段往返和 Yukari 运行验证；尚未用 Windows 原引擎实机读取 Yukari 输出，不能据此宣布完全互操作。** 场景中的字体、滤镜、滚动、历史和其他未执行功能也不能仅因保留了原始字段就视为兼容。

## 两种独立容器

`encode_save` / `decode_save` 是 Yukari 自有格式，魔数为 `YUKARISAVE\0\0`，版本1，40字节头，记录载荷长度与64位完整性校验。载荷含时间戳、UTF-8注释、缩略图、VM快照和调用方定义的运行时数据；整个文件限制128 MiB。这与原版 `Yuka  \0\0` 格式明确区分。

`encode_original_save` / `decode_original_vm` 使用下述原版布局。原版文件不会被自有格式解码器冒充接收。所有codec只操作内存；输出路径、状态目录隔离、临时文件与原子替换由runtime负责。不得将原游戏目录作为默认写入位置。

## VM 快照

`Vm::snapshot()` 当前输出版本9，可读取早期版本1至8。它保存：

- 完整脚本调用栈：源名称、指令/元素结构、可变解密数据段、整数数据单元、局部值及类型、PC、待赋值目标、子调用位置。
- 普通和全局 Flag/String 银行、特殊字符串、步骤计数、根Return诊断结果。
- host等待、选择块收集/等待位置，以及 `Vm::call()` 注入菜单时暂存的父等待状态。
- 导入原版后需要保留的原始header模板与尚未执行的原版next PC。

- ScriptSubCall 标记、脚本跳过模式/菜单状态、每帧 SelectID 与选择活动标记、NowScriptStateGet 状态，以及挂起的选择事件。

运行时自有块当前为 `YUKARI_RUNTIME_9`，包含 TextModel v4、Audio v6、图层运动/透明度、粒子、修改后图像、按键历史、TTE、等待图标、Auto/Skip 显示状态和 CD 时钟。TextModel v4 保存输入框几何与值；缺这些信息的旧 v3 活动输入框快照会明确拒绝。Audio v6 保存原版音量衰减值、实际消费的片段/帧游标与原生音源种类；不保存后台预读位置。继续读取旧 v1–v5，整段 PCM 游标按实际重采样输出迁移。见 [流式音频](resource-streaming.md)。开发期间生成的中间 v7/v8 测试文件不是稳定发布格式。

快照绑定loader返回的源脚本内容指纹；更换资源版本后恢复会拒绝。指纹和容器校验用于检测内容变化/意外损坏，不提供签名或来源认证。快照包含嵌入脚本结构，因此原版已保存的可变索引/数据不会错误套用当前资源包中的静态索引。

`FlagReset` 将普通 Flag/String 银行恢复为空的稀疏表示，查询仍返回 0/空串；不生成 131,072 个空项。全局银行、局部值类型和已读记录不受影响。快照缓冲区及原版 LE 块写入使用批量编码，格式版本不变。

snapshot、restore、call均应在 `run()` 外调用。restore先构建并验证候选VM，成功后整体替换；失败保留原VM。loader、host和trace配置保留。恢复一个已暂停的host不会自动重放该host；runtime需要重建对应等待并调用resume。

`Vm::call(menu)` 在当前暂停点上推入菜单脚本，菜单与正文共享变量银行，拥有独立locals；菜单结束后还原正文的原等待。`in_call()` 可用于判断菜单是否结束。这样更改选项后返回正文时不需要用旧快照覆盖新设置。完整snapshot/restore则按定义恢复全部银行，包含global。

## 原版容器头

已观察的文件均有72字节头，魔数 `Yuka  \0\0`，`+0x08 = 0x48`。下列每项是两个little-endian DWORD：offset、size。

| header偏移 | 内容 |
|---|---|
| `0x10` | 用户注释 comment1：SaveDataSave(slot, string) 的 string；CommentGet/Change 使用此项 |
| `0x18` | Index 文本 comment2，来自 context+0x8010c |
| `0x20` | 运行状态区，包含银行、脚本、场景、音频等 |
| `0x28` | 历史记录区 |
| `0x30` | BMP缩略图 |
| `0x38` | 前选择状态：追加的slot0原始完整容器 |
| `0x40` | 全局游戏/窗口标题，来自 0x56c950 |

offset相对该容器自身的base。嵌套容器中的offset不能直接当外层文件绝对位置。`decode_original_vm(bytes, base, encoding)` 和场景codec均支持明确的base。

例如 `savedata.001` 为1,662,346字节，SHA-256为 `104e09a77e1a1571f85c90d9c63e52d2db63e9a331719b92c600e11bd3a43841`：缩略图在offset100、长度26654，为126×70的24-bit BMP；状态区在26754、长度801311；offset828065起又有一份完整Yuka容器，长度834281。`savedata.998` 是两份839835字节对象的组合。

证据入口：[SerializeSave](../analysis/original-engine/story/SerializeSave.json)、[SaveNormal](../analysis/original-engine/story/SaveNormal.json)、[SlotPath](../analysis/original-engine/story/SlotPath.json)。菜单内SaveDataSave会复制进入菜单前保留的正文状态，不能把当前保存菜单本身当作正文存档。

## 元数据读取与追加改写

`read_original_save_metadata` 返回两个注释字段、游戏标题和 BMP 原字节。`rewrite_original_save_metadata` 可分别更新 comment1、comment2 和已编码 BMP；不修改原始输入。它只追加新数据并回填对应的根容器 descriptor，旧场景、银行、脚本、历史、前选择嵌套容器和所有未知字节保持原位。无修改请求返回完全相同的字节。新字符串严格编码并检查 NUL；BMP 检查头、尺寸、像素范围；失败不会产生部分修改。

原版初次保存 0x442970 将用户参数复制至256字节清零缓冲区（最多255个原编码字节），写入 header+0x10/14；Index 写入+0x18/1c；全局标题写入+0x40/44。机器码中稳定栈 header 基址为 esp+0x98：0x442ad5 写 esp+0xa8，0x442b26 写 esp+0xb0；临时 push 后对应长度写入分别位于 0x442afc、0x442b50。不能交换用户注释与 Index。

原版 CommentChange helper 0x432e50 追加字符串并只更新+0x10/14；CommentGet 0x439d50 通过 reader 0x432c30 返回第一输出，原接口最多拷贝255个编码字节。reader 的第二输出先取游戏标题，存在 Index 时被 Index 覆盖。ThumbnailChange helper 0x432fa0 先缩放图片并编码 BMP，追加后只更新+0x30/34。内存工具接受已完成缩放的 BMP，文件时间与路径操作由 runtime 负责；原版 CommentChange 额外保留文件最后修改时间。

证据：[SaveCommentAppend](../analysis/original-engine/instructions2/SaveCommentAppend.json)、[SaveMetadataReader](../analysis/original-engine/instructions2/SaveMetadataReader.json)、[原版固定范围汇编](../tools/evidence/scalar-services.asm)。此工具不会重序列化未知内容，仍不等于 Windows 原引擎实机验收。

## 运行状态头与银行

状态头大小为0x160。相对该头的关键字段：

| 偏移 | 含义 |
|---|---|
| `0x14/0x18` | 普通Flag区offset/字节数，观察值0x40000，即65536个int32 |
| `0x1C/0x20` | 音频记录offset/数量 |
| `0x24/0x28` | 图层记录offset/数量 |
| `0x2C/0x30` | 脚本frame链offset/数量 |
| `0x110/0x114` | 普通String索引offset/字节数，观察值0x80000，即65536组offset/size |

这些offset仍相对容器base，而非状态头。String实际字节另存，带NUL结尾。原版槽内没有GlobalFlag/GlobalString；`Vm::import_original()`保留当前global银行，仅替换槽内的普通银行和调用栈。

全局银行在独立的 `System.dat` 中，由 `original_system` codec处理；不能用旧槽文件回滚或伪造当前系统设置/通关状态。其已知布局由System writer证据确认，写回保留原始模板中的字体/配置字段，不凭槽文件生成一份臆造System.dat。

证据：[StateWriter 0x43F160](../analysis/original-engine/story/StateWriter.json)、[StateReader 0x4477D0](../analysis/original-engine/story/StateReader.json)。场景和音频见 [original_scene.hpp](../include/yukari/original_scene.hpp)，与VM codec分别负责自己的字段。

## 原版脚本frame

frame头为0x50字节。`+0x04`为下一frame位置，链中先父后子；使用数量遍历，而非把文件剩余字节全部当frame。

| 偏移 | 内容 |
|---|---|
| `0x08/0x0C` | 名称offset/字节数 |
| `0x10/0x14` | 代码offset/DWORD数 |
| `0x18/0x1C` | 索引offset/16字节记录数 |
| `0x20/0x24` | 解密数据offset/字节数 |
| `0x28` | 当前PC，代码DWORD索引 |
| `0x2C` | next PC |
| `0x30/0x34` | 待赋值元素索引/有效标记 |
| `0x38` | 原版context执行模式 |
| `0x40/0x44` | locals表offset/12字节记录数 |

local记录tag0为未初始化，tag1的`+4`为int32，tag2的`+4/+8`为字符串offset/字节数。运行时元素类型11/12是局部整数/字符串缓存；导入时规范化为VM type10，实际值类型由locals表决定；原版输出时再按当前local类型恢复11/12。

frame reader明确同时恢复current和next字段。Yukari从current执行，不能把载入当前KeyWait理解为立即跳到next：否则会直接翻过保存时显示的页面。未开始执行的next也保留到再次导出/快照；首次执行后由正常VM流程产生新的next。

原版可以不嵌入脚本，这时按名称从资源loader读取。嵌入脚本时必须保留其实际代码、索引和解密数据：本机001中root_omake代码与原包相同，但保存数据499字节、原包511字节，53项索引offset不同，不能混用。

整数目标可能与变量ID或控制link引用同一数据字节。writer直接写当前 `Script::data`，不拿已缓存的 `Element::value`重建数据；局部类型、目标索引、PC边界和长度均有检查。

证据：[FrameWriter 0x433600](../analysis/original-engine/story/FrameWriter.json)、[FrameReader 0x43F6D0](../analysis/original-engine/story/FrameReader.json)。

## 编码与原版写出限制

默认编码为CP932，严格解码/编码，不替换坏字符。提供的已有槽文件含翻译程序写出的字节；用默认CP932解码会拒绝，而显式CP936可读取。本轮仅将其作为相同原版二进制布局的样本，没有据此逆向汉化程序。编码需调用方明确选择，不能自动猜测后冒充原版日文结果。

`encode_original_save(state, options)`在内存构造原版头、银行、脚本和locals；scene callback追加场景/音频并修订对应字段。history 与前选择容器按原始块附带，历史 glyph/语音记录由独立 HistoryView 解析。缺缩略图时生成黑色126×70 BMP占位，runtime应优先传入实际截图。

`options.template_bytes`可保留原文件背书的未知header字符串/配置指针区；已解码的原始header随VM快照保留。没有模板的新档仅写已知字段，并清除无法提供背书数据的未知字符串指针。这个保守输出仍须Windows实机验证，不能因为Yukari能读回就视为原引擎已接受。

## 验证

`tests/save_tests.cpp`使用独立内存脚本覆盖自有容器损坏/版本/长度、暂停菜单和共享银行、选择恢复、可变数据别名、原版头、fresh原版布局与VM执行、current/next保留、当前KeyWait不翻页及pending目标恢复，以及native writer使用实时数据而非旧缓存。测试不依赖assert，ASan/UBSan通过。

对提供的78个原始savedata槽完成只读内存验证：明确CP936解码 → VM导入 → Yukari快照/恢复 → VM导出 → 原版容器和场景写出 → 再解码；核对Flag/String、frame数量/名称/current/next、locals、原始数据，以及图层/声音数量，全部通过。原始文件未覆盖。该测试验证结构与字段保持，不是Windows实机验收，也不证明尚未实现的效果/字体/历史都已正确执行。

字形扩展：运行时 v9 为每个文本样式保存原始 LOGFONT 家族、高宽、字重、charset/quality、内部 effect、两种厚度和原始 COLORREF，以及 effect/边框/阴影是否继承全局的标志；仍读取 v1–v8。原生 kind=3 字形精灵的 saved+0x48/+0x4c/+0x50 分别为内部 effect、阴影色、边框色，其余 kind 继续按 group/frame 解释该联合区。零 effect 与 -1 颜色的继承含义在导入导出中保留。
