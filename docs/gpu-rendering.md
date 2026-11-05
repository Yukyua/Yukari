# GPU 绘制与合成

macOS 默认选择 SDL2 Metal renderer。SDL 负责窗口、逻辑坐标、图层绘制、几何变换、最终合成与呈现；原先仍在 CPU 逐像素执行的运行时图像操作，现在由 `src/gpu_ops.mm` 的 Metal compute shader 完成。启动输出实际后端，`--require-gpu` 可拒绝软件回退。

```sh
./build/yukari play game --require-gpu
```

软件 renderer 保留原实现，供无显示环境运行和独立差分参考。这里的 GPU 覆盖指 macOS Metal 运行路径；显式选择其他 SDL 后端不代表会启用 Metal compute。

## 当前路径

| 操作 | Metal 路径 |
| --- | --- |
| 图层、精灵、粒子、影片 | SDL GPU 纹理、裁剪、缩放、旋转、逐行波形及 alpha 混合 |
| DrawStop / Effect / 冻结帧 | GPU render target 快照与复用，不读回整屏 |
| YKG / 外置遮罩 | 解码后上传，GPU 按原平均 RGB、反相及整数舍入规则乘入 alpha |
| GraphicAddColor / SubColor | GPU 饱和加法后减法，每次从原图重新计算 |
| 褐色、灰度、反色 | GPU 像素处理；灰度保留整数 3:6:1 权重 |
| 画布及存档预览叠加 | GPU 清空、像素中心采样、straight-alpha Porter–Duff 合成 |
| OriginalFont 颜色替换 | GPU PATCOPY 修改 RGB，保留覆盖率、原始 mask 与共享资源隔离 |
| GraphicCopy / GraphicCopy2 | GPU 原始颜色平面操作，保留 Scalar/MMX、颜色键、RGB mask 顺序与 ADD/SUB 端点规则 |
| 重叠自复制 | GPU 单 lane + 私有 scratch buffer，保留 bottom-up、从左到右读写依赖 |
| 剧情、姓名、Ruby、历史文字 | 字形小图缓存，GPU 按绝对 cell 坐标和裁剪框合成，重叠字形使用原整数 straight-alpha 公式 |
| 普通及用户转场 | GPU 生成/缓存阈值或用户蓝通道 alpha 遮罩，再通过 GPU 混合快照；不逐帧在 CPU 扫描像素或生成像素行几何 |
| 截图、存档缩略图、修改后位图导出 | GPU 完成绘制后显式读回，CPU 编码 PNG/存档 |

原引擎加速分支跳过的 GraphicFilter 1–3、转场 15–28，以及 LOAD ADD/SUB 的普通 alpha 行为仍保留；接入 GPU 不改变这些已确认的分支语义。软件后端仍执行原软件语义，不能拿两种原本不同的分支要求逐帧相同。

图像/影片解码、字形栅格化与 GDI 字形效果小图生成、文本布局和脚本状态仍在 CPU；这些提供 GPU 绘制的源数据。无窗口的静态 `Scene::image_thumbnail()` 是离线 CPU 工具。窗口区域自动提取需要把画面交给原生窗口 API，也属于显式导出。没有把这些工作称为 GPU 绘制。

## 纹理与同步

`GpuPixels` 持有 Metal 图像；派生图像不会在 CPU 重新计算一份镜像。CPU surface 只保留尺寸与导出空间，直到 `layer_pixels()` 等显式请求才读回。原图、修改后图像、原始遮罩分别保留，保证关闭滤镜、图集共享和存档恢复不破坏原状态。

Metal 操作使用独立 command queue，连续操作合并到一个 command buffer，在 SDL 导出或显式读取时提交。连续文字合成只处理受影响的字形矩形。最终通过 SDL 的公共 `SDL_RenderGetMetalCommandEncoder` 写入 SDL target，然后切换 target 使 SDL 重建绘制状态；不访问 SDL 私有 texture 结构。跨 queue 交接前等待 compute 完成，不读回像素。当前仍有 GPU/CPU 同步开销，未声称零等待或给出 FPS 提升倍数。

逐字文字复用相同前缀，通过复制旧 GPU 画布仅叠加新增字形，旧 GPU image 不被原位修改。文字更新复用独占的同尺寸 SDL target；共享图集与快照保持独立生命周期。转场遮罩按模式/资源缓存，alpha 在 GPU 混合时应用。

静态画面按精确绘制状态复用整张合成纹理；位置、裁剪、变换、可见性、图像内容和前缀快照变化都会失效。相同文字布局可以直接复用。Metal compute 结果延迟到实际绘制时才导出；连续画布、滤镜和文字操作合并到同一个 command buffer，避免每步等待，连续颜色/单色/反相 setter 也只在消费前执行一次最终滤镜。逐行波形用批量 GPU geometry 提交，转场复用固定 render target，YKG 独立遮罩在 GraphicCopy 之间复用 GPU 纹理。`RENDER_STATS` 提供 `compositions/composition_reuses/text_layout_reuses`、`gpu_operations`、实际 compute `gpu_submissions` 与 CPU→GPU `gpu_uploads`，`readbacks/readback_pixels` 同时统计修改后图像的显式导出。测试直接调用 SDL 的观察读回不计入 Scene 统计，因此这些计数用来验证运行路径，不是性能基准。

## 验证

```sh
cmake --build build -j
ctest --test-dir build --output-on-failure
# 以下需要真实 macOS 显示与 Metal；不能用 dummy renderer 替代。
./build/gpu_composite_tests
./build/render_tests --gpu
./build/text_tests --gpu
./build/native_glyph_render_tests --gpu tools/gdi_oracle/fixtures/wine-11-substitute-ms-gothic-u65e5-border3.ygdi
python3 tests/title.py --game-dir game --exe build/yukari --gpu
python3 tests/story.py --game-dir game --exe build/yukari --gpu
```

新增差分测试同时建立 CPU 参考和 Metal Scene：85 组、556,724 个 RGBA 字节，覆盖独立 RGB mask、五种 image type、Scalar/MMX、双向重叠自复制、饱和调色、非整数比例缩放、PATCOPY、滤镜备份、重叠 GDI 字形、裁剪及存档恢复。整数操作逐字节相同，褐色浮点滤镜允许 1 个色阶差异；所有操作均断言显式导出前没有 Scene CPU 读回。

GDI 采样的原始颜色/透明度保持精确，最终 SDL framebuffer 混合仍采用既有的 2 色阶量化容差。当前样本来自 Wine 与替代字体，不构成 Windows GDI 全字形一致性证明。另见 [字形管线](native-glyph-pipeline.md)。

本机最终验证：50/50 CTest、普通文字 GPU 模式、GDI 采样字形、Metal API Validation 下的差分测试全部通过；真实游戏 Metal 标题悬停/重启、剧情/历史/设置/保存菜单、同进程与跨进程 Yukari/原版格式存读档均通过。保存预览另补齐 DBCS 判定及字形效果尺寸接口，并增加含日文注释的预览存读档回归。

设置页逐帧预算、实测结果与复测命令见 [设置页性能](settings-performance.md)。

NEW GAME 的 VM 银行、快照与增量文字优化见 [开场性能](newgame-performance.md)。
