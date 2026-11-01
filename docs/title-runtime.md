# 标题运行时

> 本文记录标题阶段的实现。后续剧情、菜单与存档进展见 [README](../README.md)、[剧情审计](story-menu-audit.md) 和 [存档格式](save-format.md)。


2026-10-31。Yukari 现在通过原版 `start.yks` 的实际调用链进入 参考样本 标题输入等待；没有直接加载一张标题图片来替代脚本执行。程序目标仍只有原版 `game.exe`，未执行 Windows 程序或分析汉化程序/补丁。

## 入口与模块

```sh
./build/yukari play game
```

- `src/runtime.cpp`：VM 宿主、窗口事件循环、资源/图层状态、精灵时间轴、音频等待、选择声明和系统状态。
- `src/render.cpp` / `include/yukari/render.hpp`：SDL2 Scene，PNG/GNP/BMP/YKG 解码、图层裁剪与透明混合、HiDPI 及截图。
- `src/audio.cpp` / `include/yukari/audio.hpp`：libsndfile OGG 解码、48kHz stereo 浮点混音、音量、循环与播放状态。SDL callback 不执行解码或分配音频缓冲区。
- `src/vm.cpp`：HostResult.collect_block 及 HostCall.block_begin/end；MouseSelect 收集声明、Case 参数求值、等待输入后 choose 分支。

启动经过系统/字体/菜单注册、Logo 和标题动画、title_main 的 SET_DATA、title_bace、BGM、SpriteCreate、UserTransition 后停在 MouseSelect。新系统状态的默认标题有四个按钮，未解锁的 OMAKE 不显示。

原版依据见 [启动审计](title-startup-audit.md) 和 [图层/动画语义](title-graphics-semantics.md)。详细 REA Evidence 位于本地忽略目录 `analysis/original-engine/title/`，会话已导出并关闭。原始文件不修改。

## 图层与动画

原版 GraphicLoad / Hide 创建或替换图层；父图层的位置、显隐和透过率作用于其精灵。层按编号排序；同一父层的精灵按槽位排序。精灵不被父图的像素尺寸裁剪：标题使用的 damy 仅 16×16，却承载屏幕中部按钮。

YKG 动画按 **完整 32 字节全零记录** 分组。type=0 但其他字段非零不是分隔符。组内记录的 type 非零时改变播放 mode；4/104 正向播放一次保持末帧，2/6/102/106 循环，反向模式按相应方向推进。单帧组视作静态。启动所用 motion action 均为零；跨 frame/group 的非零 action 仍明确报未实现。

GraphicTransmittance 0 为不透明、1000 为透明。GraphicScroll / TransmittanceScroll 按时间插值，等待函数在事件循环中恢复。对尚不存在层的 GraphicTransmittance 按原版忽略，支持 title_ani 在层4尚未创建时设置透过率的原脚本路径。

DrawStop 保留旧帧，DrawStart/Transition 提交新场景。当前普通转场使用旧/新帧淡化，UserTransition 会验证并加载指定效果资源，但同样用淡化近似，尚不复现原版遮罩阈值/混合公式。

独立 YKG mask 目前按白透明/黑不透明的反向 RGB 平均值，与颜色的内嵌 alpha 相乘。原版解码路径和 damy 全白 mask/透明占位布局支持此实现，但尚没有原版完整合成函数的直接等价证明；不能宣称所有 Alpha 变体已精确兼容。

## 稳定输入等待

MouseSelect 不作为普通空函数返回。VM 执行选择块内的声明表达式，向宿主提供 IndicateCase/MouseCase/MouseCase2/KeyTableCase 的参数与块入口，跳过各分支正文，收集完成后保持 Waiting。重复 run 不重复执行声明，不产生脚本忙循环。

SDL 鼠标事件支持标题 MouseCase2 的启用条件、矩形命中、按钮悬停帧、说明图集、悬停/点击音效。点击从对应脚本块入口恢复。Esc 和窗口关闭可以退出。KeyTableCase 已登记，但秘密按键序列及其他事件族尚未接入；标题后的脚本若调用尚未实现服务，仍会明确停止。

声明登记并不等于完整实现各子系统。启动中 DefaultFontSet/TextWindowSet、历史、快捷键、存档显示和菜单区域等服务保存配置供后续使用，尚未实现 GDI 等价正文文字绘制、历史界面、原版存读档或完整选项/确认菜单。这些不影响本次默认标题到达与显示，但不据此声称整部游戏可玩。

## 音频与状态

SoundLoad、SoundPlay、SoundLoadStream、SoundPlayLoop 使用同一内存解码后端，SoundPlayWait 等待实际播放完毕；--fast 模式使用解码时长和虚拟时钟完成等待。每个音频源/输出缓冲区限256MiB，stream 当前也是整段解码，并非磁盘流式播放。尚未实现声音渐变和完整原版音量曲线；分组音量采用通道/组倍率。

SystemDataWrite 将 GlobalFlag/GlobalString 写入 Yukari 自己的 `system.dat`，有独立格式头和原子替换；--state-dir 可指定位置，默认工作目录 `.yukari/`。不会读取原始 `save/`，也不会创建/覆盖原版 savedata。此文件仅是 Yukari 系统设置，不兼容原版剧情存档。

## 验证

- CTest 包含无商业样本的格式、VM、渲染、音频测试。渲染以实际像素验证透明 mask、顺序、位置、裁剪、显隐和截图；音频测试实际 callback 完成、循环、重载、音量/分组及非法输入。
- VM 保留16个真实脚本分支回归，及选择声明/等待/分支跳转测试。
- `python3 tests/title.py --game-dir game --exe build/yukari`：两次从原始 start 启动、同状态重启、截图像素/四个按钮、悬停局部变化；前后完整 SHA-256 确认五包不变，原游戏目录无新增文件。
- 新状态实际结果：`TITLE_READY layers=21 sprites=14 regions=12 steps=855`。数量包含初始化后隐藏 UI 图层/精灵，不代表可见按钮数量。
- macOS 原生窗口、正常时钟和实际音频完成等待下，从 start 到达标题，保存 `.yukari/title.png`；逻辑1024×576，当前 Retina 输出2048×1152。视觉检查通过，透明占位没有遮挡背景，四个标题按钮和Logo位置正确。
- ASan/UBSan 下的 CTest、真实 VM 场景和标题启动检查均通过。

与原版 Windows 的逐帧对照尚未做；上面的通过范围为静态证据、Yukari 自身测试和真实资源显示，不是原版运行行为观测。
