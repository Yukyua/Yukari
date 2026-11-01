# 原版字形与边框管线

当前实现精确复刻原版 **软件 DIB 后处理**，并提供实际 GDI 字形采样的接入路径。它不等于已完成全部 Windows 字形的逐像素验证：本机没有已验证的微软原版 MS Gothic/MS Mincho 字体，也没有原生 Windows GDI 全字符基准。现有 `.ygdi` 样本明确标记为 Wine 11 与 Wada/efont 替代字体。

## 字形来源与使用

```sh
./build/yukari play game --font-directory /path/to/fonts --gdi-cache /path/to/ygdi
# 缺少完整匹配采样时明确失败，防止无声替代：
./build/yukari play game --gdi-cache /path/to/ygdi --require-gdi-capture
```

`NativeFontRasterizer` 优先匹配采样的字体名、文本、LOGFONT 高宽、字重、charset/quality、采样倍率、内部效果、厚度及各原始颜色。缓存保留实际选中的字体、字体 SHA-256 和宿主来源；仅同名字体不证明其轮廓等同微软字体。`--font-directory` 提供候选字体，并不会把 FreeType 变成 Windows GDI。

匹配成功时，读取真实 TextOut 产生的彩色平面和白底黑字透明度平面，再执行原版像素核心。缺失时默认使用 FreeType MONO 在 1x/4x 栅格化，来源为 `freetype-mono`；轮廓、hinting 和字体度量仍是替代。严格模式直接报错。采样工具、格式与生成命令见 [GDI oracle](../tools/gdi_oracle/README.md)，不分发字体文件。

`YKGDI003` 使用 168 字节头，保存 native finish 之前完整 scratch，包括原版 TextOut 叠加的阴影与彩色背景；不能凭单个灰度 mask 反推任意前景颜色下的 GDI 舍入。采样文件在加载时验证尺寸、描述与平面字节数，按需加载像素。

## 原版像素语义

- 内部效果 1–5 对应普通、4x 抗锯齿、阴影、描边、描边加阴影；脚本 DefaultFontEffect 写入值加一。
- 4×4 采样对 DWORD 的四个字节分别累加后右移 4 位，不四舍五入。
- 软件描边保留原版 bottom-up DIB 遍历顺序，判断整个 DWORD 是否为 `0x00ffffff`，向左、向下原地写黑。灰色源点会触发纯黑扩张，其自身灰度保留。
- 奇数厚度保留整数截断的前景/mask 原点，组合阴影位移保留 `(4s,4s-2)`，不改成对称描边或整数显示像素阴影。
- 场景纹理保留采样彩色平面，透明度来自 mask；原始 cell 坐标、裁剪和推进由 TextModel 控制，不叠加旧渲染器的装饰 padding。
- GraphicRefresh 更新继承的全局样式，保留字形精灵显式覆盖。运行时 v9 和原生 kind=3 精灵均保存 effect/边框/阴影状态，见 [存档格式](save-format.md)。

完整算法、汇编位置及边界条件见 [像素核心](native-glyph-kernel.md)。零阴影厚度产生的原版越界访问在移植中裁剪，避免复现内存破坏。DirectDraw 灰度重复 TextOut 描边不属于当前软件 DIB 核心的验证范围。RGB 子像素透明度 mask 会明确拒绝，尚需逐通道合成实现。

## 验证记录（2026-10-31）

- 对原版 SHA 锁定的 `0x405250` 软件缩小函数执行独立机器码对照：2,108 组、2,163,635 个 BGRA 输出像素逐字节一致，输入/输出 guard 与分配释放检查通过。只执行该 helper，未执行游戏入口、TLS 或导入初始化。详见 [机器码报告](../tools/gdi_oracle/native-downsample-report.md)。这项结果验证缩小函数，不代表整条 GDI 绘字管线或所有字体已等价。
- 字形效果测试覆盖四字节截断、灰度触发、定向膨胀、奇数厚度和组合阴影；捕获格式与字体来源测试覆盖严格匹配、缓存及缺失失败。
- Wine x86/x64 实际 GDI 采样验证质量 0 仍可能输出灰度。现有两个“日”字样本分别覆盖默认描边与 LOV.ini 的 3 像素描边。
- 捕获样本的原始纹理数据精确比较通过；SDL Metal 实际绘制和 framebuffer 读回通过，显示混合允许 2 个色阶的量化差。不得把 GPU 容差写成原始像素完全相同。
- 49/49 CTest 通过；字体来源与像素核心另通过 ASan/UBSan。
- 真实游戏标题、21 页剧情与保存/读取通过；该剧情页保存前后正文区域的 607 个近白像素坐标完全相同。整幅截图存在角色/背景动画差异，未声称全画面逐像素一致。

新增原版反编译会话导出于 `analysis/original-engine/gdi-fonts2/session-evidence.json`，包含缩小与 DIB 分配/选择/填色相关七个 helper，已关闭会话。逆向目标始终只有原版 EXE，未读取汉化程序或补丁字体。
