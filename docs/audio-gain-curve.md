# 原版 SoundVolumeType 音量曲线

原版data01.ykc内default.ini的2605字节配置包含 `SoundVolumeType = 1`；原注释仅为“音量指定方法的设置”，本身没有解释枚举。原INI解释器42d349..42d36e同时写入声音全局56db2c及影片全局4cdb6e，值为首整数非零；无参数将两者清零。两者不是VM的volume bank，也不是slot回滚数据。

## 音频证据

`AudioEffectiveVolume.json`中45e0b0先调用45df20取最终整型音量v，再把`double(v)/1000`传入4608f0。45df20的group整数乘积与每步截断保持不变，曲线转换只发生在最终合并之后，不能分别将每个group转成振幅再相乘。

4608f0证据 `instructions2/AudioGainCurve4608f0.json`，ev_d135a6b98cb8e39b198168035a2bc309ab11e96d7621da99dc366cdfea7778cd。canonical漏掉x87栈表达式，精确算式以同目录`AudioGainCurveAssembly.txt`为证。464f80的log10语义由 `AudioLogAssembly.txt` 中`fldlg2; fyl2x`确认；相关常数的原PE地址：4a38c8=10000.0，4a9498=0.25。令r=v/1000，转换为DirectSound LONG如下（truncate向零）：

* v=0：-10000。
* SoundVolumeType=0：trunc((r-1)*10000)。
* SoundVolumeType!=0：trunc((log10(r*10000)*0.25-1)*10000)。

DirectSound参数单位为百分之一dB，允许范围[-10000,0]，不能放大。SDL float mixer以`10^(attenuation/2000)`转换成振幅，-10000静音端点按静音处理。这是对原DirectSound衰减语义的宿主实现；[微软SetVolume文档](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708939%28v%3Dvs.85%29)说明单位、允许范围及最小值的静音含义。

| 最终整型volume | 类型0，DS LONG | 类型1，DS LONG | 类型1振幅近似 |
|---:|---:|---:|---:|
| 0 | -10000 | -10000 | 0 |
| 1 | -9990附近，保留浮点截断 | -7500 | 0.000178 |
| 100 | -9000 | -2500 | 0.056234 |
| 500 | -5000 | -752 | 0.420727 |
| 1000 | 0 | 0 | 1 |

忽略LONG量化，类型1约等于`r^1.25`，并非线性r。类型0是分贝值插值，500约为振幅0.003162，不能误认为“类型0=线性振幅”。数值公式保留原先乘法、log和减法顺序，未代数化简；这影响边界处浮点转LONG的截断。

原4608f0直接调用DS SetVolume并返回成功/失败，不做clamp。负volume或大于1000通常得到无效DS LONG；调用失败时原buffer保留旧音量。新实现保留“请求音量”以供查询，同时保存最后成功的attenuation，不能把非法数值猜成放大或强制归零。

## 实现与持久化

新增header-only `audio_gain.hpp` 提供native_sound_attenuation、valid判断及PCM gain纯函数，纯函数测试不链接SDL。Audio新增set_volume_type(bool)、volume_type()及applied_attenuation(id)，默认false与原BSS一致；runtime应将EngineConfig.sound_volume_type传入。clear及restore不改变外部INI曲线配置。缓冲与分段流都在最终混音前应用同一曲线，fade仍在原整数volume空间推进，随后映射成振幅。

为保存非法请求后的最后有效buffer音量，Audio blob升级v5，每个channel在fade记录之后增加4字节有符号attenuation。来源加载metadata扫描兼容v1..5；restore继续读取旧v1..4，旧blob没有该缓存时依当前INI曲线计算。资源PCM、stream fragments与cursor/fade/groups保存规则保持原设计。

`audio_gain_tests`覆盖两个模式、精确衰减、静音端点、非法范围及0..1000单调性，ASan/UBSan通过。完整audio_tests以UBSan运行通过，包括真实mixer振幅比、非法请求旧音量保留、v5往返、v4迁移、groups/fade/virtual clock/stream cursor恢复。没有重复运行会挂在macOS系统service初始化的SDL+ASan组合。

## 影片分支的区别

同一个INI键另写4cdb6e，但影片4123df..412413的曲线与声音不同。原汇编显示type0为`trunc(v*10-10000)`，type1为`trunc(log10(v)/3*10000-10000)`，v=0为-10000。忽略量化时，影片type1振幅约为r^(5/3)，不能复用声音r^1.25的helper。当前本次修改仅音频Audio后端；影片宿主应依据这条独立证据接入。
