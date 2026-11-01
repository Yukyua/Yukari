# Yukari

面向 macOS 的 C++17 / SDL2 Yuka System 兼容实现项目。

**已能从原始启动脚本进入标题、运行剧情与菜单，并保存和恢复游戏。** 当前兼容性依据原版引擎和参考样本验证。已验证原始资源包中的 571 个 YKS 与 8,515 个 YKG 能被解析；汉化程序及补丁不纳入分析。详见 [原始资源审计](docs/original-resources.md)。

已实现：

- 按需读取 YKC001 / YKC002 目录和资源，分别用 CP932 / UTF-8 解码文件名；资源查找忽略 ASCII 大小写。
- 参考样本 原版包挂载，按已确认的原版优先级查找资源。
- 提取单个资源，保持原始字节，输出文件已存在时拒绝覆盖。
- YKS001 的头部、三段布局、0xAA 数据区解码、八种已知索引元素和指令流反汇编；显示代码区 DWORD 偏移与索引编号。
- 脚本 VM：变量与局部上下文、整数/字符串表达式、if/else、脚本调用/跳转、Return、Case 块跳过，以及可挂起/恢复的宿主接口。
- CP932 文本转 UTF-8；无效编码、未知元素类型、截断与越界会明确报错。
- YKG000 的颜色、独立 Alpha 和 32 字节帧记录解析。
- SDL2 / Metal GPU 图层合成、共享图集、转场与遮罩、图像调色、画布绘制、重叠复制及逐字文字合成；透明度、精灵动画与移动、标题按钮悬停和脚本事件分支。
- OGG/WAV 等音频按资源包范围读取、后台流式解码、有界 PCM 队列；混音、循环、音量淡变及语音/动画等待。
- 日文正文与姓名文字、逐字显示、点击/Enter 推进、历史回看与语音重播。
- 原脚本驱动的设置、声音、保存/读取与确认菜单；暂停菜单返回后保留剧情调用栈。
- 完整 VM 快照；原生 `Yuka` 槽容器、图层/精灵/音频/历史，以及 `System.dat` 全局银行的读写。
- 独立的资源预览模式；保持宽高比，Esc 退出。

已导出原版 357 个指令入口对应的 325 个独立处理函数，所有入口均有实现路径；入口覆盖不等于所有参数组合或所有路线已验证。已接入逐字、Ruby、禁则、原始图像字体、原版 Already.dat、选择调度、影片、MIDI、CD 接口、粒子和图层运动。各指令的证据与验证边界见 [指令契约](docs/instruction-contracts.md) 和 [当前兼容性审计](docs/compatibility-audit.md)。

原生存档已通过静态代码对照、结构往返和 Yukari 恢复验证；**尚未验证 Windows 原引擎读回 Yukari 生成的文件**。文字已接入原版软件 DIB 字形效果、GDI 双平面采样缓存与严格匹配模式；未采样字形使用 FreeType 替代，尚不能声称整作与 Windows GDI 像素一致。详见 [字形管线](docs/native-glyph-pipeline.md)。CD 的真实物理设备、全部路线及 Windows 消息行为仍需实测。

## macOS 构建

依赖 Xcode Command Line Tools、CMake、SDL2、libpng、FreeType、libsndfile 和 FFmpeg。Python 3 仅用于测试。

```sh
brew install cmake sdl2 libpng freetype libsndfile ffmpeg pkg-config
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build -j
ctest --test-dir build --output-on-failure
```

以上依赖在本机已存在；项目没有自动安装或修改系统依赖。`RelWithDebInfo` 保留调试符号并启用优化；设置页帧预算按优化构建验证。

## 启动游戏

```sh
./build/yukari play game
```

窗口图标默认从游戏目录内与目录同名的 `.exe` 读取；目录改名或主程序名称不同时，用 `--original-exe /path/to/original.exe` 显式指定原版主程序。该文件仅用于读取图标资源，不会执行，也不会自动选择补丁程序。

macOS 默认优先使用 Metal，启动会显示实际渲染后端；`--require-gpu` 可禁止软件回退。GPU 合成、纹理复用与验证记录见 [GPU 渲染](docs/gpu-rendering.md)。设置页已加入严格 20 ms 逐帧基准，见 [性能实测与复测](docs/settings-performance.md)。[NEW GAME 性能](docs/newgame-performance.md) 记录 VM、自动存档和增量文字绘制的优化与复测。

窗口会从原始 `start.yks` 执行 Logo 和标题动画。点击 NEW GAME 开始；左键或 Enter 推进，逐字显示时首次点击补全当前句。剧情底栏可打开保存、读取和设置；S/F6 保存、L/F7 读取、O/F3 设置、H/F4 历史，Esc/右键返回菜单，关闭窗口退出。A/F1 自动推进，R/F2 跳过（目前未区分原版 Already.dat 的已读状态）。历史用滚轮或上下键翻页，右键退出。保存确认框的注释栏支持文字输入，Enter 应用、Esc 取消。

资源解码采用有界图像缓存，音频默认流式解码，无需额外开关；架构、内存界限和验证见 [资源与音频管线](docs/resource-streaming.md)。

默认输出目录为 `.yukari/`，`--state-dir PATH` 可修改：`savedata.NNN.yukari` 是完整 Yukari 快照，`savedata.NNN` 是实验性原生槽，`yukari-system.dat` 保存 Yukari 全局状态，`System.dat` 保留原模板中的未知字体/配置并更新原生全局银行。原游戏目录只读。读取菜单会列出输出目录及原 `save/` 的槽；同编号优先 Yukari 快照。

```sh
./build/yukari play game --load-slot 3
./build/yukari play game --load-original-slot 3
# 仅当已有存档确实使用 GBK/CP936 时显式选择；默认仍为 CP932。
./build/yukari play game --load-original-slot 3 --original-encoding CP936
```

原生槽自身不含全局银行；读取源游戏的原生槽时同时读取其 `save/System.dat`。Yukari 快照保存全部银行。这里只解析存档数据，逆向目标仍仅为原版程序。

自动化验证可用：

```sh
./build/yukari play game --until-title --screenshot /tmp/yukari-title.png
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/yukari play game --fast --mute --until-title --state-dir /tmp/yukari-test-state
python3 tests/title.py --game-dir game --exe build/yukari
python3 tests/story.py --game-dir game --exe build/yukari
```

`--fast` 是加速时钟测试模式，会省略中间转场帧与逐字显示，正式游玩省略它。标题截图在 HiDPI 显示器上可能为 2048×1152，逻辑画布仍为 1024×576。具体实现和兼容边界见 [标题运行时](docs/title-runtime.md)。

## 汉化文本导出

```sh
./build/yukari dump-text game translation/original-engine
```

导出全部有效脚本的字符串常量为 UTF-8 JSONL 和 CSV，包含剧情、姓名、选项、界面及其他常量。保留原始字节、脚本/指令位置和用途，CSV 预留译文列；无需运行游戏，不修改资源。输出目录必须是源游戏目录外的新目录。详见 [文本导出](docs/text-dump.md)。

## 资源工具

```sh
./build/yukari list /path/to/game/data01.ykc
./build/yukari extract /path/to/game/data01.ykc 'script/start.yks' /tmp/start.yks
./build/yukari script /tmp/start.yks
./build/yukari script-entry /path/to/game/data01.ykc 'script/start.yks'
./build/yukari graphic /path/to/image.ykg
./build/yukari view /path/to/image.ykg
./build/yukari view-entry /path/to/game/data01.ykc 'image/title.ykg'
./build/yukari game-script game 'yks\system\OptionSystem.yks'
./build/yukari game-view game 'ykg\title\title_bace.ykg'
./build/yukari vm game 'yks\system\optiondefault.yks'
./build/yukari vm game --trace --max-steps 100000
```

资源名使用 `list` 显示的 UTF-8 名称，查找忽略 ASCII 大小写，保留分隔符。`game-script` / `game-view` 使用原版 参考样本 配置，按 `data04 → data01 → data02 → data03 → data05 → data06 → data07 → data08 → data09` 挂载存在的包；额外自定义包仍需配置适配，音频/MIDI 支持只读包外文件回退。详情及证据见 [原版引擎逆向](docs/original-engine.md)。`view`、`view-entry` 和 `game-view` 可附加 `--once` 渲染一帧后退出。提取不会自动解密 YKS；`script` / `script-entry` 在内存中处理解密。

解析器将单个资源限制为 512 MiB，索引数量与解码图像尺寸也设上限；这些属于实现限制。不同版本可能需要扩展格式处理。

## 原版适配与验证

文档中的 `game/`、`game.exe` 和 `analysis/original-engine/` 使用中性示例名称，复核时请替换为本地原版样本及证据路径。原始文件只读。原版 EXE 的 SHA-256 为 `08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267`。不分析汉化程序、CNP 与补丁 DLL。游戏目录和详细分析证据已列入忽略规则。

1. 已记录原版主程序身份、PE 导入、资源清单、启动脚本及 357 项函数分发表。
2. 已对照原版资源查找修复大小写匹配，并实现已知包优先级。
3. 已实现 VM 核心，并通过 16 个真实脚本分支场景的变量结果及步数校验，见 [VM 文档](docs/vm.md)。
4. 已接入 SDL2 图层、精灵、文字、鼠标、音频，验证标题、剧情、设置与存读档菜单。
5. 已连续推进2001页剧情并进入 common/com01，保存成功；已验证同进程及跨进程恢复、原生槽恢复后继续同一脚本位置，并对 78 个现有存档执行只读的结构/VM 往返检查。已完整解码本作 MPEG 影片；剩余路线及原生 Windows 互操作仍需验证。

证据与未决问题见 [docs/evidence.md](docs/evidence.md)。项目按 GPL-3.0 分发，公开格式工具的来源与归属见 [THIRD_PARTY.md](THIRD_PARTY.md)。


## VM 验证

```sh
ctest --test-dir build --output-on-failure
# 显式提供原始游戏目录；测试不携带商业样本。
./build/vm_game_tests game
python3 tests/original_game.py --game-dir game --exe build/yukari
```

`vm` 默认从 `start.yks` 开始。目前会在 `yks\system\OptionSystem.yks:word=0xA` 的 `WindowNameSet` 服务处明确停止；`optiondefault.yks` 等纯核心脚本可完整执行。退出码 0 为完成、1 为错误、3 为步数用尽或等待宿主。`result=none` 表示根脚本没有显式整数返回值。
