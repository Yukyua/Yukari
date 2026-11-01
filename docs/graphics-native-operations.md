# 原版图形滤镜与变换后端

2026-10-31，仅对照原版 `game.exe`。本文补充 [GPU 绘制](gpu-rendering.md)，不将 Windows 原版静态分析描述为运行观察。`Scene` 只提供绘制后端；脚本参数、滚动时序、父层/精灵状态和存档由 runtime 接线。

## Scene 接口

| API | 已实现的语义 |
|---|---|
| `set_add_color(id,colorref)` / `set_sub_color(id,colorref)` | COLORREF 低字节为 R；每项替换原值，不累加。0 和 `0xffffffff` 禁用。按原图先饱和加、再饱和减。 |
| `set_sepia(id,bool)` | 使用原版亮度加偏移公式，未使用常见 sepia 矩阵。 |
| `set_monochrome(id,bool)` | `(3R+6G+B)/10`，整数截断。 |
| `set_negaposi(id,bool)` | RGB 分量各为 `255-channel`。 |
| `create_filter(id,mode)` / `filter_type(id)` | 替换为 GraphicFilter 特殊层，mode1..4 保留为 type10001..10004；在 z 顺序中作用于此前已合成画面，不修改单张源图。 |
| `set_scale(id,x_per_mille,y_per_mille)` / `set_scale_center(id,x,y)` | 1000 为 1 倍，负值可镜像；中心为层本地像素坐标，默认(0,0)。 |
| `set_rotate(id,angle_units)` / `set_rotate_center(id,x,y)` | 2000 为一周，500 为顺时针90度；旋转中心独立于缩放中心，默认(0,0)。先缩放，再旋转，最后加世界位置。 |
| `set_image_type(id,type)` / `image_type(id)` | 图片类型1..3普通绘制，4/5保留 LoadADD/SUB 类型。GraphicFilter 的 `image_type()` 返回10001..10004。 |

同一资源共享不可变 CPU RGBA 与 GPU 图像。单图滤镜从原始像素重算，顺序为 add → sub → sepia → monochrome → negaposi，保留 Alpha。一个图层的滤镜不修改同资源的其他层；`copy()` 以源层当时的图像为基底，后续源层滤镜变化不污染副本。变换使用 SDL GPU 几何顶点，不读回像素。

Sepia 的原版表达式为：

```
Y = .299*R/255 + .587*G/255 + .114*B/255
R' = clamp(Y + .078512)
G' = clamp((Y + .031304) - .039984)
B' = clamp(Y - .16125199)
```

各分量限于0..1，再乘255并按原版浮点转整数舍入。尚未建立跨 x87 / 当前 CPU 浮点环境所有舍入边界的穷举等价证明。

## 明确保留原版后端差别

原版 software 和 GPU 路径并非同一套像素结果。Yukari 按当前实际 renderer 对应其分支，未把软件语义擅自套到 Metal 上。

- GraphicFilter type10001/2/3 在 software 上分别将**累积画面**做 sepia / monochrome / negaposi；绘制在其后的图层不受影响。原版这些 helper 在 GPU 全局开启时直接返回，因此 Yukari 加速路径同样保留特殊层但不改变像素。这是保留已观察到的原版分支，不是吞掉 unsupported 异常。
- type10004 在原版层遍历中直接落到后续精灵循环，没有像素处理；Yukari 保留这个类型。该结论来自整个分派与汇合路径，不能从命令名猜第四种滤镜。
- LoadADD/SUB 的原版 GPU helper 使用 SRCALPHA / INVSRCALPHA，没有 image type4/5 的特殊分派；Yukari Metal 对这两种类型也使用普通 Alpha。
- software type4/5 的透明 mask 为白时跳过、黑时复制源图；中间 mask 值，ADD 为 `min(D+S,255)`，SUB 为 `max(D+S-255,0)`，**不是 `D-S`**。RGBA 后端以相应 opacity0/255/中间值选择分支。原版中间值分支不读全局透过率参数；整层完全透明仍由更外层跳过。独立 YKG mask 的颜色到 Alpha 归一化仍受 [已有 mask 证据边界](title-graphics-semantics.md) 限制。

software 特殊合成只在 software renderer 中执行并计入 readback 指标；Metal 没有默认 CPU 回退。普通图层、变换、单图资源滤镜及快照不因此增加 GPU 读回。

## 依据与局限

REA 原版导出位于 `analysis/original-engine/instructions/`：`AddColorCore.json`、`SubColor.json`、`Sepia.json`、`Monochrome.json`、`Filter.json`、`Scale*.json`、`Rotate*.json`。像素文件名不可当作语义：`FilterPixels.json` 对应0x408D30（实际sepia），`SepiaPixels.json` 对应0x408F90（实际mono），`MonoPixels.json` 对应0x409090（实际negative）。单图重算顺序来自已有 `title/internal-0x416a00.json`。

另外保存了只读 Apple LLVM objdump 定点证据，**不是 REA Evidence**。每份 JSON 含完整指令字节输出、明确起止地址、命令、原版 SHA-256：

- `color-setters.objdump.json`：0x416F40..0x416FD4，确认四项字段写入和重算调用。
- `layer-constructor.objdump.json`：0x412950..0x4129F5，零初始化及 add/sub=-1 缺省。
- `transform-dirty-bounds.objdump.json`：0x4130D0..0x413351，脏区边界计算。
- `transform-render.objdump.json` / `rotation-z-matrix.objdump.json` / `transform-constants.bytes.json`：0x413440 的平移/缩放/旋转矩阵顺序、angle/1000×pi 和常数字节。采样、边缘覆盖和旧 D3D 半像素习惯未建立逐像素等价证明。
- `filter-layer-dispatch.objdump.json`：整个0x413D90..0x414F0F，特殊层分派及 type10004 汇合。
- `software-image-blend.objdump.json`：0x40BB60 的 type4/5 饱和运算、mask端点分支。
- `gpu-image-draw.objdump.json` / `gpu-quad-draw.objdump.json`：0x40D290→0x40B000，GPU普通Alpha状态。

`render_tests` 在 software 与实际 Metal 上验证颜色打包/饱和/重算顺序、滤镜独立性、四种特殊层与 z 顺序、GPU跳过/无读回、缩放中心/镜像、旋转单位/方向/组合顺序、变换Alpha和两种后端的 ADD/SUB 分支。`text_tests` 回归通过。这些测试验证上述接口和像素规则；未执行 Windows 原版，也不涵盖所有图层滤镜/变换的剧情组合与保存恢复。

## Transition and Effect

`Scene::render_transition(old, mode, progress, user_mask={}, rect=nullptr, sampling={})`
composes the new layers and applies the original backend's old-frame operation,
without presenting. A nonempty resource selects UserTransition1/2; otherwise
mode is Transition1..32. `TransitionSampling` supplies the actual effective
helper duration, native quality (`DAT_56c32c`) and timing mode (`DAT_56c860`).
Progress selects a native loop phase, rather than specifying a wall-clock delay;
the runtime owns waits, disabled-transition policy and interruption. Random CPU
modes19/20 require `set_transition_random(callback)` sharing the original CRT
stream with the VM. Each transition shuffles once, using the native full-range
swap loop rather than a different shuffle algorithm.

Evidence: `instructions/TransitionCore.json`, `UserTransitionCore.json`,
`TransitionMode1.json` through `TransitionMode32.json`, `MaskTransition1.json`,
`MaskTransition2.json`; timed fade uses
`instructions2/TransitionFadeTimed.json` (0x41cc90). All refer to the original
EXE. CPU paths use the observed pixel-copy, staggered-stripe, signed integer
fade, diagonal ramp, random block, quadrant, split-reveal and sliding-snapshot
operations. Software snapshots/readback are restricted to that backend.

The accelerated branches differ materially from those CPU operations:

- Modes1/3..14 generate the original full-canvas blue-byte mask, then draw an
  old snapshot. The 4x4 order and numeric constants come from original data at
  0x4a3990/0x4a3a10 and 0x4a3a90. The vertical gradients5/6 divide by canvas
  **width**, as the original does. Mode2 fades the old snapshot.
- Modes15..28 do CPU/DIB writes and allocate no old GPU layer. On the original
  accelerated branch they expose the new GPU scene, rather than reproducing
  those software styles. Modes29..32 translate the old GPU snapshot.
- UserTransition1 and2 install the mask's low DIB byte (blue) as texture alpha
  via 0x40d1c0 →0x40ae00. Both use the 0x40b270 drawing kernel. That kernel
  inherits texture-stage COLOR/ALPHA state, while ordinary image drawing at
  0x40b000 installs `ALPHAOP=MODULATE`, texture alpha and diffuse alpha. Yukari
  models this observed ordinary-image stage state: old opacity is blue-mask
  alpha multiplied by `truncate(255 - float(native_transmittance)*0.255)`.
  This is an explicit stage-inheritance boundary: equivalence with a frame
  whose previous D3D stage was configured differently has not been established.
  It is not the CPU RGB-average hard threshold/ramp algorithm.
- Accelerated snapshot geometry spans the whole canvas even when RECT narrows
  the helper's loop. RECT controls its loop extent; it does not crop the old
  GPU texture. CPU direct fade/mask loops advance from the base DIB pointer
  without a RECT.top offset; they modify the bottom `bottom-top` rows. Other
  CPU blitters retain their own explicit rectangle/source coordinates.

SDL geometry submits constant-alpha runs sampling the existing old GPU texture;
accelerated transition rendering performs no screen readback or CPU color
compositing. Geometry batches have a bounded vertex count. The tests cover all
32 backend dispatches, masks, integer midpoint colors, reveal direction,
quadrant asymmetry, rectangle addressing, completion and GPU readback counts.
The phase calculations preserve helper step sizes supplied by sampling; exact
original presentation scheduling still belongs to the runtime.

`effect_steps(mode)`, `effect_offset(mode, amplitude, step)`,
`effect_step_ms(mode, duration, whole_duration, accelerated)` and
`render_effect(frame, mode, amplitude, step)` implement Effect1..6. Modes1..4
have four ±amplitude steps, with vertical, horizontal and the two diagonal
orientations. Modes5/6 share the original twelve-pair table;5 first divides
amplitude by two. Completed steps restore the origin. Timing follows
`Effect2/3/4/6.json`, plus `story/EffectShake1/5.json`; accelerated helpers retain
the original minimum15ms sleep. Global disabling is a runtime policy, not a
renderer no-op.

`transition_timing(mode, sampling, user_mask, rect)` returns
`TransitionTiming{steps, step_ms, total_ms, total_duration}` from the same native
phase planner used by CPU rendering. For example native helper input40,
quality2, timing0 yields Transition2's16 stages ×20ms =320ms. Deadline-based
modes report their nominal duration; the original conditional presentation,
late-frame skips and OS scheduling can change actual completion time. Unlike
Effect, the Transition GPU helpers have no universal15ms minimum sleep.
`instructions2/DIBPixelsBase.json` confirms the positive DIB height and raw
`this+8` pixels base underlying the direct-blend RECT addressing described above.

`positioned_text(id, runs, absolute_clip)` accepts `PositionedText{text,x,y,style}`.
Each x/y is the absolute character-cell top-left; no border inset or automatic
wrap is inserted. Explicit newlines inside a run use its own line height. Runs
retain individual font, size, color, outline and shadow, so a native glyph/save
model can supply one character per run without reflowing earlier text. One
transparent texture covers the clip rectangle, with same-size updates reusing
that texture. FreeType remains the portable rasterizer, not a claim of GDI glyph
pixel equivalence. Face/size/glyph coverage caching is shared by measure_text,
text and positioned_text; RenderStats exposes cache hits and rasterizations.


`blit(destination,x,y,source,rect,kernel)` implements GraphicCopy/Copy2's
CPU image operation using `instructions/ImageCompositeCore.json` (0x40bb60),
not a framebuffer screenshot. Display alpha, position, crop and transforms
do not participate. Clipping retains source/destination alignment; overlapping
self-copies preserve the native forward DIB traversal. Destination mask bytes
remain unchanged. Raw separate RGB masks are retained for this operation.
The scalar and MMX branches have distinct native integer arithmetic, selectable
with `NativePixelKernel`; tests preserve the scalar channel crossing and MMX
16-bit multiplication/saturation rather than replacing either with SDL alpha.
Type3 uses the first raw DIB pixel as its color key. Type4/5 preserve their
transparent/opaque endpoint branches and intermediate saturated ADD/SUB.
After editing the CPU color plane, Yukari uploads the changed destination to
its GPU texture. This refresh is an explicit portability implementation
decision: stale original GPU pixels have not been observed, and absence of an
identified invalidation call alone is not evidence for emulating stale data.

`swap_layers(a,b)` exchanges complete native node state between list slots,
including image, alpha, position, crop and transforms. Each list slot retains
its ordering identity. An absent destination receives the existing node; two
absent IDs are a no-op. The GraphicSwap handler (0x4363b0) exchanges linked-list
positions and node IDs, rather than merely exchanging bitmap pointers.

`cache_prefix(through_id)` implements GraphicCache's software prefix snapshot
from `instructions2/GraphicCacheCore.json` (0x415170). Layers with native IDs up
to the cutoff are composed once; subsequent frames draw that snapshot before
the still-live higher layers. Prefix edits/deletions do not alter the snapshot.
`clear_prefix_cache()` implements 0x412d30 and removes this state. Accelerated
rendering follows the original explicit GPU return and creates no cache. This
cache is renderer state, not resource-name prefetch; a renderer rebuild must
clear it independently of deleting ordinary layers. Tests exercise both
backend branches, frozen/live contents, node swapping, copy arithmetic and
GPU texture refresh without external game assets.


`set_raster(id,enabled,amplitude,frequency,phase)` supplies the native quality2
row displacement from `instructions2/RasterRowAssembly.txt`: each row uses
`truncate(sin((truncate(phase)+row)/source_height*frequency*3.141592)*amplitude)`.
Nonidentity scale or rotation suppresses raster, matching the dispatch order.
Rows retain ordinary source alpha and clipping; fractional phase truncates
before row addition. Runtime owns enabling quality and advancing phase/amplitude.
Software and Metal tests cover the phase arithmetic, disabling and scale exclusion.

Snow/Sakura research: `GraphicParticleReset.json` (0x415140) walks all layers
and calls0x413400, clearing layer+0xbc and releasing the layer+0x64 cached
resource state. It does not clear the +0x140 sprite-instance array or advance
particles; 0x415990 clears that array.
The scheduler, spawn and updates are in `LayerMotionTick.json` (0x417780),
especially0x417d74..0x4182c8. `ParticleDispatchAssembly.txt` records the original
bytes/addresses independently of REA. Snow and Sakura share the ten layer-ID
slots at0x4bc150 and interval/countdown globals0x4cdb70/0x4cdb88; mode at0x4cdb74
selects their distinct spawn rules. Lower initialization/list/random helpers
require additional evidence before a renderer particle model is implemented.


`ParticleModel` implements Snow/Sakura scheduling independently of SDL. Its
source evidence is `LayerMotionTick.json`, the two original handlers, and
`ParticleCapacity4130b0`, `ParticleAppend412dc0`, `ParticleRemove415a70`,
`ParticleClear415990`, `RandomRange429e10`, and `SpriteGlyphIndex40a5e0`.
Configuration preserves existing instances and their original resource/group
references. Snow's region top is not used for spawn height: sprites start at
negative sprite height. Its horizontal velocity deliberately retains the
original possible division by zero, and float-to-int conversion uses x87's
INT_MIN indefinite result. Sakura uses inclusive random intervals generated
from two original CRT draws with rejection, not modulo on one draw. Wind is
shared, updated once per selected layer when a spawn interval expires, and
retained across reconfiguration. Movement consumes raw milliseconds plus the
collection accumulator, independently of the graphics speed multiplier.

SnowEffect() stops emission while existing sprites continue moving;
SakuraEffect() deletes configured nonnegative layer IDs. Capacity checking and
append have different bounds: append's strict count+1<1000 admits999 sprites,
then rejected allocations still consume their native spawn random draws.
Removal compacts the list and the original index increments, so the following
sprite receives animation but skips position integration for that tick. Sprite
frame actions/durations use the shared native SpriteMotion model. The caller
provides original sprite groups and the CRT random stream; absent providers
fail explicitly. Unsafe zero periods and unrepresentable random intervals are
rejected rather than reproducing an infinite original loop. Tests cover these
branches without game assets or external random state.


Own-save persistence APIs:

- `ParticleModel::snapshot()` / `restore(bytes, ResourceLoader)` preserve full
  configuration, countdown, wind, layer-enabled state and existing instances.
  Each instance retains original resource name/group, frame/mode/deadline,
  animation clock, serial and all IEEE double coordinate/velocity bits. The
  loader returns original `SpriteGroups`, deduplicated by name during restore.
  External CRT random state remains owned by runtime and is not advanced or
  reseeded by restoring. Input validation precedes committing the new model.
- `Scene::layer_pixels(id)` / `restore_layer_pixels(id,bytes)` preserve the
  current CPU RGBA plane, raw separate RGB mask, native image type and distinct
  original color/mask backup. PNG payloads retain transparent pixel RGB. This
  avoids losing GraphicCopy edits when a save previously recorded only resource
  names. Load the resource and apply saved color-effect flags first, then restore
  this payload last; later color-effect changes still use the saved original
  backup. Layer position, crop, alpha and transforms remain runtime metadata.
  Restoration requires matching dimensions, creates independent GPU textures
  and leaves the layer untouched on malformed payloads. It uses CPU image
  buffers directly, without GPU screenshot readback. GraphicFilter nodes have
  no bitmap payload and are rejected by this image-only API.

Original native save configuration offsets, confirmed directly at0x43f3fa:

| Native structure offset | Global fields |
| --- | --- |
| +0x89330..+0x8933c | RECT left/top/right/bottom from0x4cdb78..84 |
| +0x89340 | effect mode from0x4cdb74 |
| +0x89344..+0x89368 | ten layer IDs from0x4bc150 |
| +0x8936c | spawn period from0x4cdb70 |
| +0x89370 | spawn countdown from0x4cdb88 |

This segment does not write wind0x4cdb8c. These native offsets are evidence
about the original container, distinct from Yukari's versioned full model
payload. `ParticleDispatchAssembly.txt` includes the corresponding raw bytes.


`original_layer_state.hpp/cpp` maps the confirmed0x140 native layer header
without overwriting unrelated fields. Evidence is `story/LayerWriter.json`
(0x415c00), `story/LayerReader.json` (0x416fe0), the original Scale/ScaleCenter/
Rotate/RotateCenter handlers, and the color setters. Header offsets are relative
to the serialized record, whose native in-memory backing starts at layer+0x12d0.

| Header offset | Native layer field | Meaning / representation |
| --- | --- | --- |
| 0x0c | +0x00 | image type, int32 including special filter types |
| 0x20 / 0x24 | +0xd0 / +0xd8 | x/y truncated to int32 by native x87 helper |
| 0x28 | +0xf8 | visible byte |
| 0x74 | +0x1108 or +0x1118 | current transmittance, or active tween target, truncated to int32 |
| 0x88 | +0x10ec | scroll mode int32 |
| 0x90 / 0x98 | +0x10f0 / +0x10f8 | scroll x/y velocity, float64 |
| 0xa0 | +0x1132 | scale enabled byte |
| 0xa8 / 0xb0 | +0x1138 / +0x1140 | current scale x/y ratios, float64 |
| 0xb8 / 0xbc | +0x1148 / +0x114c | scale center x/y int32 |
| 0xc0 | +0x1150 | scale tween enabled byte |
| 0xc8 / 0xd0 | +0x1158 / +0x1160 | scale x/y velocity, float64 |
| 0xd8 / 0xe0 | +0x1168 / +0x1170 | scale x/y targets, float64 |
| 0xe8 | +0x11f0 | angle in native2000-per-turn units, float64 |
| 0xf0 / 0xf4 | +0x11f8 / +0x11fc | rotation center x/y int32 |
| 0xf8 | +0x1200 | rotation mode int32 |
| 0x100 | +0x1208 | angular velocity, float64 |
| 0x108 | +0x1210 | rotation target int32 |
| 0x115 | +0x1280 | raster enabled byte |
| 0x118 / 0x11c / 0x120 | +0x1284 / +0x1288 / +0x128c | raster target/frequency/period int32 |
| 0x124 / 0x128 | +0x12bc / +0x12c0 | add/sub COLORREF uint32 |
| 0x12c / 0x12d / 0x12e | +0x12c4 / +0x12c5 / +0x12c6 | sepia/mono/negative bytes |

`decode_original_layer_state(header)` retains raw flag bytes and IEEE double
bits. `encode_original_layer_state(state,preserved_header)` writes only these
confirmed fields, preserving resource/sprite descriptors, unknown data and
adjacent padding. `capture_motion()` truncates position exactly as the native
writer; `apply_motion(existing)` restores serialized motion while retaining
existing scroll bounds: native+0x1120..+0x112c are absent from both writer and
reader. The native reader resets raster phase/remaining/tween-velocity
(+0x1290/+0x1298/+0x12a0) to zero, so this bridge does too. Native save does not
retain fractional x/y or an active transmittance tween: the reader restores
integer opacity and zeros its velocity/target. Raw assembly at415dda confirms
that the writer selects+0x1118 target whenever+0x1110 velocity is nonzero;
otherwise it selects+0x1108 current. `original_saved_transmittance()` preserves
this branch, omitted by the canonical pseudocode. `NativeLayerWriterAssembly.txt`
contains the original instructions. Yukari's own MotionModel snapshot
is separate and retains its full floating and transient state.

GraphicCopy persistence evidence:415c00 writes the layer header, resource and
mask names, sprite names, per-instance animation arrays and0x7c sprite records.
It writes no DIB color or alpha plane and has no bitmap-payload descriptor.
Thus a native container cannot independently recover edited GraphicCopy pixels.
416fe0 has an early same-resource load-elision condition, but later calls416a00
unconditionally;416a00 reloads a nonempty resource via416860 before applying
color effects. `instructions2/LayerImageLoader416860.json` confirms that call chain.
`title/internal-0x40f570.json` completes it:40f570 first releases the current
image via40e340, reads the original archive/file bytes and decodes them through
40f4e0. A resource-backed layer is therefore reconstructed from original bytes
and saved color settings on native load; its GraphicCopy-edited CPU pixels are
not restored from the native save, including in the same process. No-resource
dynamic bitmaps have no saved pixel payload either, but their complete restore
behavior is not inferred from this resource-backed chain. Yukari's own YUKAPIXL
payload explicitly carries modified planes; it must not be inserted as an
invented native field.

`original_layer_state_tests` checks explicit field offsets, raw byte flags,
float64/NaN bit preservation, unknown-byte round trips, MotionModel conversion,
unsaved scroll bounds and native raster reset behavior under ASan/UBSan.
