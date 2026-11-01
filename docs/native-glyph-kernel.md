# 原版软件 GDI 字形像素 kernel

仅依据原版 game.exe（SHA256 `08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267`）。对应证据为 `instructions2/GlyphRaster405cd0.json`、`FontBorderCombinedAssembly.txt`，以及 `gdi-fonts2/{Downsample4x405250,PlaneSwitch409170,PlaneAllocate4091b0,PlaneSelect409330,PlaneFill409490,FontDibCreate404dc0}.json`。本模块实现 `DAT_004cd894==0` 的软件 DIB 分支，不替代字体栅格化。

## 平面与舍入

4091b0 创建32位、正 biHeight 的 DIBSection，因此其内存行顺序为 bottom-up；409170 分别交换 image+4/+0c 的两个 HBITMAP，以及 +8/+10 的两个 bits 指针。409330 对第二个平面 PatBlt 白色；409490 对当前平面直接填 `R<<16|G<<8|B`，丢弃 COLORREF 的高字节。

原字形图片有**颜色和透明度两个独立平面**。透明度平面为白色透明、黑色不透明。GlyphRaster 先生成高分辨率颜色图并缩小，再交换目标平面，缩小高分辨率 mask，最后交换回来。它没有对颜色预乘透明度，也没有先把 mask 缩小再膨胀。

405250 软件路径逐字节累加一个4×4区块的16个样本，结果 `sum >> 4`；40556a、40558e、4055bb、4055e7 分别写四个输出字节。无加8、无四舍五入、无 gamma 校正。15白1黑得到239，而不是240。模块接完整4倍 scratch，要求尺寸为4的倍数；不实现该通用缩图函数在目标尺寸与源尺寸不同情况下的偏移细节，GlyphRaster 的新建 scratch 正好满足这个约束。

## 软件描边

令 B=4×BorderThickSize。输入 mask 是已经按 FontEffectPlan.mask_glyph_origin_hi 画好的完整白底黑字 scratch。原始 DIB 行号记 rawY：

```
for rawY = 0 .. H-1:
  for x = 0 .. W-B-1:
    if mask[rawY][x+B] != 0x00ffffff:
      mask[rawY][x .. x+B-1] = 0
for rawY = 0 .. H-B-1:
  for x = 0 .. W-1:
    if mask[rawY+B][x] != 0x00ffffff:
      mask[rawY .. rawY+B-1][x] = 0
```

两个阶段均为原地写入，且检测整个 DWORD。写入位置落在后续读取源的前方，因此不会无限扩散。原始源像素保留，二值字形的单个黑点最终扩成包含端点的 `(B+1)×(B+1)` 矩形，方向换为左上坐标后是向左、向下。灰色像素触发的扩张区域变为纯黑，源点仍保留灰度；不能替换成灰度 min/max filter 或半径为 b 的圆形描边。内部4的 measured extents 先上取整至4，内外循环界正好分别等于 scratch 的 W-B/H-B。

## 组合阴影

4072bc..407374 中，S=4×ShadowThickSize。保留原始循环及地址，不能将其简化成普通 `(S,S)` 偏移：

```
for i = H-1 down to S:
  for x = W-1 down to S:
    sy = H-1+S-i
    dy = H-i+1
    if mask[sy][x-S] != 0x00ffffff:
      if mask[dy][x] != 0:
        color[dy][x] = shadowDib
      mask[dy][x] = 0
```

这些 y 为 raw bottom-up 行号，所以左上坐标偏移为 `(S,S-2)`，即默认阴影厚度2时是高分辨率 `(8,6)`，显示像素 `(2,1.5)`。已纯黑的目的 mask 保留原颜色；灰色目的 mask 会被写成阴影颜色再变成黑色。阴影颜色在此处通过交换 R/B 的 DWORD 表达式生成，与409490不同，这个表达式保留高字节。模块如实区分。

s=0 会使原循环存在越界访问，移植 kernel 对每个推导地址做边界裁剪，避免复现内存破坏；这属于明确的安全边界差异。其余有效范围按原顺序执行。

## 接口与限制

`native_glyph_finish(plan, high_color, high_mask)` 接**已经完成 GDI 绘字**的高分辨率完整平面，再执行描边、组合阴影与缩小。Shadow（内部3）的 high_mask 应已包含原版两个 TextOut（前景及偏移阴影）；Plain 为1倍输入，其余为4倍。所有输入/输出采用左上行顺序，DWORD 为 DIB `0x00RRGGBB`；内部访问转换成原始 bottom-up 顺序。返回两个原始平面，不假称一种 RGBA 混合方式与所有原版 blitter 都相同。

`native_glyph_effect_binary(plan, glyph_mask, foregroundCOLORREF)` 接实际 GDI measured extents 大小、未偏移的0/00FFFFFF二值 mask，按现有 FontEffectPlan 构造颜色和透明度 scratch，完成全部内部1..5效果。内部2/3按原 floor(width/4) 截断；内部4按 ceil(width/4)+b；内部5保留4像素输出对齐、奇数厚度的原点截断与特殊阴影偏移。

灰度输入可传入 finish，不会被拒绝。只有从单个 mask 构造彩色绘字的 convenience 接口拒绝灰度：单凭黑字白底 GDI 的结果，没有证据可以反推出 GDI 在任意前景/背景色下的逐通道舍入结果。提供实际彩色 GDI 平面即可走精确 finish；不能将这种限制误读为原版永远只输出二值 mask。FreeType MONO 可作为移植字体来源，但其轮廓、hinting、度量与 Windows 字体不因此相同。

DDraw 分支的405250读取 RGB bitmask，再对各通道16样本求和、右移4位并重新套 mask，未计算第四字节；GlyphRaster内部4的 DDraw 描边通过循环TextOut偏移0..B（包含端点）生成。对二值 glyph 方向/形状相同，对灰度 GDI 字形其重复合成规则与软件路径的纯黑扩张可能不同。本模块没有宣称覆盖该分支。

独立测试覆盖双平面缩小、全4字节floor、原地膨胀方向和包含端点、灰度源谓词、组合阴影特殊偏移/颜色覆盖、二值完整管线、奇数厚度与尺寸、非法灰度 convenience 输入及零阴影边界。使用 clang++ 的 `-Wall -Wextra -Werror -fsanitize=address,undefined` 编译运行通过；不需要 SDL 或字体库。
