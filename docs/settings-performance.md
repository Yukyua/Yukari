# 设置页帧预算

2026-10-31，本机 macOS 15.7.3 arm64、SDL2 Metal、默认 HiDPI（1024×576 逻辑画面、2048×1152 输出）、`RelWithDebInfo` 优化构建，game 原版资源实测 **2,080 帧全部小于 20 ms**。

| 场景 | 帧数 | P50 / ms | P99 / ms | 最大 / ms | ≥20 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 画面设置静止 | 240 | 8.096 | 9.359 | 9.375 | 0 |
| 画面设置交互 | 1,000 | 8.275 | 13.021 | 15.813 | 0 |
| 声音设置交互 | 600 | 8.184 | 12.160 | 12.971 | 0 |
| 声音设置静止 | 240 | 8.085 | 9.366 | 9.415 | 0 |

画面设置交互包含字体、边框/阴影切换、透明度/文字速度滑块及按钮悬停；声音设置交互包含音量滑块和效果音单元切换。首次字体/效果切换使用冷字形缓存，没有预热、丢弃异常值或仅以平均值验收。测量时没有并行编译或运行其他测试，音频静音但按真实时钟运行。进入设置页的资源加载、页面切换以及截图导出位于采样区间之外。

本轮 NEW GAME 优化后的复测记录见 [summary.json](../analysis/performance/newgame-2026-10-31/settings/summary.json)、[运行日志](../analysis/performance/newgame-2026-10-31/settings/run.log) 和同目录四份 CSV。首次设置页优化的历史记录保留在 [原记录目录](../analysis/performance/settings-2026-10-31/summary.json)。20 ms 回归门槛会在运行机器上重新检查。

## 改动

- 完整合成结果留在 GPU target。逐帧比较精确的图层绘制状态及图像所有权/内容版本；未变化时只向窗口复制已合成纹理，省去清空、排序和重复图层混合。变换、顺序、可见性、裁剪、像素更新与前缀快照变化仍会重建。
- 连续 Metal compute 操作按顺序编码进同一个 command buffer，直到 SDL 导出或显式 CPU 读取才提交、等待。文字字形之间保留 encoder 依赖，不改变重叠覆盖顺序及跨 queue 同步。
- 相同 positioned-text 布局与样式直接复用纹理；弱引用不延长资源 LRU 生命周期，也不破坏独占文字纹理的更新条件。原位文字更新使用内容版本使合成缓存失效。
- 原 GDI 4× 缩小使用两个独立 16-bit lane 累加四个字节，每个源像素只读取一次，保留四通道向下取整及原颜色/透明度结果。随机全字节标量差分、全 255 边界和已有 GDI/Metal 字节差分测试验证结果。
- 主循环从原来的固定 `SDL_Delay(8)` 改为扣除本帧工作耗时后再等待；忙帧及已等待 GPU/vsync 的帧不再额外等待 8 ms。

## 复测

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j 6
python3 tools/benchmark_settings.py --game-dir game --exe build/yukari \
  --output /private/tmp/yukari-settings-benchmark
```

输出目录必须尚不存在且路径没有空白。基准自动创建独立存档目录，不写游戏资源。需要真实 macOS 显示会话；强制 Metal，保留默认 HiDPI，使用真实时间，不启用 `--fast`。不要同时运行其他性能测试或编译。普通 Debug 构建用于调试，不用于这个帧预算。

运行时 actions 新增：

```text
profile LABEL FRAME_COUNT BUDGET_MS
# 此处可以继续执行 hover / click / wait-ms 等动作
profile-wait
```

`profile` 开始采集接下来的完整帧；`profile-wait` 等待本轮全部样本。`BUDGET_MS=0` 只报告，正值按严格“小于”检查，任一帧达到预算都会使进程返回失败；未完成采集就退出同样失败。CSV 位于 state-dir 的 `frames-LABEL.csv`，只在本轮结束后写入。

`frame_ms` 包含主循环更新、事件/VM、渲染/SDL 提交，以及帧末实际等待；同时分别记录 update/render/present/sleep 和 GPU 操作、纹理更新、资源解码数。`present_ms` 是 SDL_RenderPresent 调用的墙钟时间，不是 GPU timestamp，也不表示显示器扫描输出延迟。P99 使用 nearest-rank，最大值取全部样本。

## 回归

保留像素精度、真实游戏剧情/历史/设置/菜单和 Yukari/原版存读档验证；优化构建的测试目标显式启用 assert，避免 Release/RelWithDebInfo 将断言移除。`frame_profile` 单元测试检查分位数、20 ms 边界判定及 CSV 完整性，渲染测试检查静态复用与位置、可见性、删除、原位文字更新后的失效。
