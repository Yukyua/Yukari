# NEW GAME 的 VM 与 GPU 开销

2026-10-31，使用 game 原版资源、macOS 15.7.3 arm64、SDL2 Metal 默认 HiDPI、`RelWithDebInfo`。基准在标题等待后点击 NEW GAME，采集接下来的 1,600 个完整真实时钟帧，覆盖转场、首句逐字显示及随后等待；没有启用 `--fast`，没有跳过原脚本指令或转场。

## 本机实测

| 指标 | 优化前 | 优化后 |
| --- | ---: | ---: |
| 开场执行指令 | 659 | 659 |
| 开场 Scene GPU 操作数 | 1,334 | 101 |
| 1,600 帧 P99 | 9.466 ms | 9.387 ms |
| 最慢一帧 | 100.720 ms | 47.225 ms |
| 自动存档 Yukari 文件 | 1,133,790 bytes | 85,390 bytes |
| 纯 VM 固定循环中位数 | 105.541 ms | 60.392 ms |

逐字合成的操作数降低约 92.4%；纯 VM 循环墙钟降低约 42.8%。密集空银行的诊断阶段中，VM 快照/原版编码各约 26/29 ms；最终为 0.510/0.841 ms。普通银行清空是这些存档阶段下降的主要原因。

最慢帧仍包含首次图像加载，尚未达到开场所有帧低于 20 ms。`Vm::run()` 含主机服务累计时间本轮为 90.215 → 102.257 ms，包含首载资源和等待，不能当作纯解释器速度证明。设置页另外完成 2,080 帧严格预算回归，最高 15.813 ms，详见 [设置页性能](settings-performance.md)。

[完整对照](../analysis/performance/newgame-2026-10-31/summary.json)、[纯 VM 样本](../analysis/performance/newgame-2026-10-31/vm-benchmark.json)、[密集银行诊断](../analysis/performance/newgame-2026-10-31/dense-bank-diagnostic.log) 和同目录逐帧 CSV 均保留；对照二进制使用相同优化级别，最终二进制哈希记录在 JSON 中。

## 实现

- `FlagReset` 直接清空普通 Flag/String 银行，查询时仍得到原来的 0/空字符串。原实现会实际插入各 65,536 个空值，导致后续快照排序和原版字符串编码处理大量无用数据。全局银行、frame locals、读历史、当前执行位置保持原语义。
- Already.dat 的逐指令标记复用当前脚本/长度对应的银行，减少重复规范化名称和 map 查找。复制、移动、显式 rebind 都会使游标失效；不同大小、大小写别名和读历史恢复仍受测试约束。host 参数一次预留容量。
- VM 快照预留缓冲区，直接写入字符串与整数块，排序使用连续数组。原版存档的 flags、代码和元素索引按已验证块批量写入。容器/VM 版本、校验、LE 字段及恢复验证保持不变。
- 逐字文本检测相同布局/样式前缀，GPU 复制上一张画布后只叠加新增 glyph。旧 GPU image 保持不可变，避免与 SDL 独立 queue 的导出竞争；共享画布、缩短、换色、裁剪变化回退完整重绘。
- 隐藏文字的记录和样式继续更新，栅格化延后到 parent 显示前；隐藏/显示、GraphicRefresh、存读档的结果通过合成截图对照验证。
- FreeType 替代字体的高分辨率单色字形源独立缓存，上限 16 MiB。key 包含实际选择的字体文件/face、文本、height/width/weight 和采样倍率；颜色和边框/阴影复用同一轮廓。相同实际字体、字形源、效果及颜色的最终位图通过弱引用共享，同时复用 GPU glyph 上传；不同请求字体名仅在实际解析为同一个 face 时共享。真实 GDI capture 和严格匹配模式仍优先，未扩大字体等价性声明。

## 解释测量

逐帧 CSV 增加 `vm_steps` 和 `vm_ms`：后者是 `Vm::run()` 的墙钟时间，**包含同步主机服务、资源加载和 GPU 等待**。日志 `HOST_PROFILE` 给出各主机指令和存档子阶段的调用次数、总时间、最大值。存档阶段也计入完整 `frame_ms`，没有因为它发生在 `Vm::run()` 之外而漏掉。

`gpu_operations` 是现有 Scene 操作计数，不是 GPU timestamp。操作次数下降不能直接换算为 GPU 执行时间下降的相同比例。纯解释器性能另用固定指令序列测量，避免把资源解码波动误当成 VM 分派开销。

可复现命令：

```sh
python3 tools/benchmark_newgame.py --game-dir game --exe build/yukari \
  --output /private/tmp/yukari-newgame-benchmark
# 可选 --baseline /path/to/saved-before-binary，按相同顺序记录优化前后版本。
python3 tools/benchmark_vm.py --baseline /path/to/saved-before-binary \
  --exe build/yukari --output /private/tmp/yukari-vm.json
```

NEW GAME 基准创建独立 state 目录，输出日志、完整 CSV、首句截图和 JSON。输出目录必须尚不存在、路径没有空白；源游戏目录只读。纯 VM 基准执行 1,000,000 个相同 label/expression/jump 指令，7 次完整进程计时，包含启动和微型合成资源包加载，验证每次恰好执行指定指令数。

## 验证范围

新增普通银行清空对全局/locals/读历史的隔离、紧凑存档、Already 游标复制/移动/rebind、隐藏文字刷新后显示与存读档回归。增量 glyph 通过 CPU 全量重播逐字节对照，覆盖重叠、越界裁剪、共享副本、缩短和换色。共享字形源与全新 rasterizer 在不同颜色/效果下逐字节对照。

真实原版游戏的标题/重启、剧情/历史/设置/菜单、自有与原版格式跨进程存读档继续回归；这不代表已经验证 Windows 原引擎读取 Yukari 输出。
