# 字体效果、厚度与颜色

本记录仅使用原版GlyphRaster405cd0、GlyphColor40a140、TextGlyphPlace415280及原357命令handler。运行时当前`script effect2→shadow / effect3→border`的**类别没有错**；原消费者内部编号比脚本编号大一，不能混为同一个枚举。

## 编号与继承

DefaultFontEffect(43a8f0)将脚本参数加1写入56c36c（ev_a1d6b503b99656e87170a567a4dd76a140fada07f06afa7365c5ad9b7d47e1e7）。GlyphRaster先读sprite+54；若小于1，则使用56c36c，再switch内部值：

| 脚本DefaultFontEffect参数 | 内部效果 | 分支行为 |
|---:|---:|---|
| 0 | 1 | 普通、1倍栅格 |
| 1 | 2 | 普通、4倍栅格后下采样，无阴影/描边 |
| 2 | 3 | 4倍栅格阴影，使用ShadowThickSize |
| 3 | 4 | 4倍栅格描边，使用BorderThickSize |
| 4 | 5 | 4倍栅格组合描边与阴影，特殊双缓冲路径 |
| 其它 | 非2..5 | 默认普通、1倍栅格 |

负script值仍先做32位加1；其结果可能小于1，但当它作为全局default时不会再次继承别的默认值，而是switch默认普通分支。字体效果不会影响`TextColor`颜色选择方式。

42b92f把esi置2，42bb18/1e将其分别写入config+140d0/+140d4：原ShadowThickSize和BorderThickSize默认均为2。原default.ini明确将BorderThickSize覆盖成3。初始化56c36c(config+13bac)也为内部2，即脚本口径1的普通超采样。证据保存在`instructions2/FontThicknessDefaultsAssembly.txt`。

## 颜色规则

GlyphColor40a140创建glyph时写入：+50前景、+54内部effect、+58阴影、+5c描边。416760将text layer+100/+104/+108/+10c传入415280，再传入40a140。GlyphRaster405cd0最开始分别将+58==-1替换成56c370、+5c==-1替换成56c374。它没有“颜色为-1就禁用效果”的条件；颜色sentinel与effect启用是不同状态。

DefaultTextShadowColor/BorderColor及TextShadowColor/BorderColor setter将超出0..FFFFFF的输入归一为-1，前者写全局，后者写layer继承值。若全局本身也为-1，GlyphRaster仍把该DWORD交给填色/GDI路径，不能据此声称原版主动关闭mask生成；具体无效COLORREF的结果与原GDI状态相关。新plan保留raw颜色，不以-1改effect种类。

前景+50任何非零高字节都触发低24位调色板索引读取，不只是FF高字节。GlyphRaster临时替换为调色板值，栅格后恢复原sprite字段。SDL宿主应使用自己的palette resolver，不丢失原始索引。颜色均按原COLORREF低字节R、次字节G、再B解释；effect5软件路径将此交换为DIB像素排列，不能将其当新颜色配置。

## 厚度、尺寸与原点

以下坐标为原GDI左上原点，hi表示4倍栅格像素；W/H是GetTextExtentPoint32A在对应supersample下的测量值。

* 内部1/默认：canvas=(W,H)，supersample=1。
* 内部2：canvas=(floor(W/4),floor(H/4))；glyph原点(0,0)。
* 内部3：令s=ShadowThickSize；canvas=(floor(W/4)+s,floor(H/4)+s)，前景原点(0,0)，阴影mask平移(4s,4s)hi，即显示像素(s,s)。s不是模糊半径。
* 内部4：令b=BorderThickSize；canvas=(ceil(W/4)+b,ceil(H/4)+b)。前景原点(2b,2b)hi，即(b/2,b/2)显示像素；mask先在(4b,0)hi画字，然后矩形扩张向左4b、向下4b。故b是总扩张尺寸，居中半径b/2；原配置b=3对应1.5显示像素半径，不是固定1或3整数像素。直接把它塞给整数border_width会近似化，而且移动ink原点/扩展bitmap也必须考虑。
* 内部5：令b=BorderThickSize，t=4*trunc(b/2)。canvas每轴为align_up_4(floor(measure/4)+4b)。前景在(2t,3t)hi，描边mask起始字形在(3t,2t)hi，再按内部4同样矩形扩张。奇数b时mask中心与前景并非通过简单b/2平移完全重合，不能随意修正其整数截断。

内部5的shadow后处理是逐像素读写，4072bc读取ShadowThickSize，按原bottom-up DIB行号平移；换成GDI左上坐标后为(4s,4s-2)hi，即(s,s-0.5)显示像素。不能把组合分支直接等价成“描边后再偏移(s,s)”而宣称逐像素复刻。对应原汇编在`FontBorderCombinedAssembly.txt`；`font_effect.hpp`保留前景原点、mask原点、左右/下扩张和shadow平移这些独立参数，宿主可用4倍mask实现。

`native_glyph_effect.cpp` 现已实现软件 DIB 的白底黑字 mask 后处理、颜色平面和 4x 下采样；`native_font.cpp` 可读取实际 GDI 双平面缓存，缺失样本时采用明确标记的 FreeType MONO 来源。严格模式拒绝缺失采样。plan 本身仍不提供 Windows GDI，FreeType 来源不声称与其相同，详见 [管线](native-glyph-pipeline.md) 与 [像素核心](native-glyph-kernel.md)。415280在font cell_width>0时仍按配置cell推进（DBCS全宽，SBCS半宽）；只有cell_width<1时按效果扩展后的bitmap宽推进。不能把canvas padding无条件加进所有文本层的字距。

## 独立解析模块

新增header-only `font_effect.hpp`：

* font_effect_from_script：保留32位加1。
* resolve_font_effect：按内部effect和继承颜色生成FontEffectPlan，默认native effect2、两个厚度2。
* FontEffectPlan.canvas：按原每个分支的floor/ceil/alignment计算bitmap输出尺寸。
* resolve_glyph_foreground：保留任意非零高字节的palette间接取色。

font_effect_tests验证脚本/内部编号、覆盖与继承、3/4/5分支坐标、奇数描边、尺寸取整、无效效果退普通及颜色调色板，ASan/UBSan通过。已接入 CMake 测试，runtime 消费 effect 类别、继承颜色和两个 thick 配置；整数 TextStyle 仍不足以完整表达上述 4 倍坐标。当前 FreeType 描边将半径取整，组合效果仍采用常规边框加阴影，不能宣称上述原 mask 算法已逐像素实现。
