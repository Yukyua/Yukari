# 参考样本 原始资源审计

审计日期：2026-10-31。范围仅为 `game/data01.ykc` 至 `data05.ykc`；未读取汉化 EXE、CNP 或补丁 DLL。原始包按只读方式打开。

这些结果验证资源解析兼容性，不能据此宣称游戏 VM、动画或音视频已经实现。

## 复现

```sh
python3 tools/audit_resources.py game --cli build/yukari > /tmp/yukari-game-audit.json
```

脚本只输出结构统计、配置、函数名和可定位元数据，不输出剧情文本。`--cli` 对每个 YKS 调用现有 C++ `script-entry`，丢弃反汇编正文，记录错误。

## 包目录

| 包 | 文件字节数 | 索引偏移 | 条目数 | 类型 |
|---|---:|---:|---:|---|
| data01.ykc | 49,884,385 | `0x2F90031` | 572 | .ini: 1, .yks: 571 |
| data02.ykc | 1,006,148,057 | `0x3BF7EE85` | 2,193 | .ykg: 2,193 |
| data03.ykc | 911,880,563 | `0x3658E48F` | 4,261 | .ykg: 4,261 |
| data04.ykc | 630,675,859 | `0x2596B88F` | 2,061 | .ykg: 2,061 |
| data05.ykc | 851,563,098 | `0x32BD0B3A` | 15,656 | .ogg: 15,656 |

五包均为 `YKC001\0\0`，24 字节头，20 字节索引项，所有索引 reserved DWORD 为零。合计 24,743 条目。

data03 与 data04 有 496 个同名 `ykg\ta\...` 条目，已逐项 SHA-256 比较，496 对全部字节一致；随后已从原版 EXE 确认 data04 先于 data03，见 [原版引擎逆向](original-engine.md)。首例 `ykg\ta\ai01ex0001.ykg` 在 data03 的 `0x7CA719F` 与 data04 的 `0x18`，均 277,071 字节，SHA-256 为 `671dc0613622f6d0fc2075e8c2f59359d88595aad5326168a49332d2983ce666`。

## YKS

全部 571 个脚本的加密标记为 1，数据段采用现有解析器的 XOR 0xAA；全部通过 Python 结构审计和 C++ CLI 反汇编。无 CP932 解码错误、未知元素类型、索引越界或顶层字符串常量赋值错误。共有 3,232,413 个代码 DWORD、1,648,077 个索引元素、824,603 次函数调用；最大 locals 头字段为 1,600。

| 元素类型 | 索引项数 | 顶层出现次数 |
|---:|---:|---:|
| 0 | 10,686 | 824,603 |
| 1 | 149,076 | 148,722 |
| 4 | 1,043,467 | 5 |
| 5 | 139,195 | 0 |
| 8 | 200,801 | 126,287 |
| 9 | 16,157 | 15,267 |
| 10 | 88,695 | 87,482 |

类型 7 本样本未出现。顶层类型 4 出现 5 次；解析成功只说明边界可读，执行语义需原版解释器证据。

语言特征：139,195 个 type-5 字符串常量全部严格 CP932 解码成功；69,705 个含非 ASCII 字符，48,481 个含平假名（共 594,148 字符），12,530 个含片假名（共 44,774 字符）。这支持包内文本为日文；没有将共存汉化资源用于验证。统计仅报告字符类别，不保存剧情。

## 启动证据

`data01.ykc:start.yks` 位于包偏移 `0xA45`（2629），长度 3095；85 元素、2 locals。以下偏移为代码区 DWORD 索引，与 `script-entry` 输出一致，并非文件字节偏移。

| word 偏移 | 调用/目的 |
|---|---|
| `0x0C`–`0x21` | 依次调用 SETUP_BOOTCHK、SYSTEM、FORMAT、SOUND、TEXT、OPTION、ETC、LOGO 标签 |
| `0x2B` | ScriptCall `yks\system\OptionSystem.yks` |
| `0x3F` | ScriptCall `yks\system\OptionDefault.yks`（受 GlobalFlag:60 条件控制） |
| `0x4A` | ScriptCall `yks\system\OptionVoice.yks` |
| `0x50`–`0x5C` | createText / createMenu / createScKey / createHistory / createIcon |
| `0x62` | OptionSetting |
| `0x85`–`0x8E` | logo / logo_e / title_ani / title_ani_e（受 GlobalFlag:51 条件控制） |
| `0x9D` | ScriptJump `yks\title\title_main.yks` |

`default.ini` 位于包偏移 `0x18`，2605 字节。明确设置 `WindowSize = 1024, 576`、`SaveDataDirectory = Save\`、`FontBorderThickSize = 3`、`TransitionWaitType = 2`、`SoundVolumeType = 1`。这些值的枚举意义仍需原版代码确认。

已复现资源名大小写阻断：`start.yks` 引用 `yks\system\OptionSystem.yks`，实际包条目为 `yks\system\optionsystem.yks`。修复前 C++ CLI 用前者返回 `entry not found`，用后者正常解析。随后从原版 `0x42A190` 确认 `__stricmp` 比较，Yukari 已修复 ASCII 大小写查找，混合大小写的启动引用已通过验证；未擅自归一化分隔符。

## YKG

全部 8,515 个图像具有 64 字节 `YKG000` 头；头 `0x08` DWORD 为 64，`0x0C`–`0x24` 为零。所有颜色平面签名为 `89 47 4E 50 0D 0A 1A 0A`（GNP）；21 个图像另有同签名 Alpha 平面。245 个图像有帧区，共 3,023 条 32 字节记录。Python 审计无边界或长度错误；另以临时 C++ harness 链接现有 `build/libyukari_core.a`，逐项调用 `Archive::read` 和 `parse_graphic`，8,515 项全部通过，Alpha/帧计数一致。这未测试完整像素解码或 SDL 显示。

当前 `Frame` 字段名保持假设性质：第 6 DWORD 分布为 0:1196、1:1133、2:292、4:402；第 7、8 DWORD 全为零；第 5 DWORD 原始值包括 0、250、3200 等，不能仅凭数值认定时长单位。

可复核样本：

| 类别 | 包:资源 | 包偏移 | 字节数 |
|---|---|---:|---:|
| alpha | `data02.ykc:ykg\effect\eff_orz_01.ykg` | `0x12766B6D` | 69596 |
| animated | `data02.ykc:system.ykg` | `0x18` | 12211 |
| color_only | `data02.ykc:ykg\bg\12th_bg_01a.ykg` | `0x2FCB` | 1367689 |

`system.ykg` 颜色区偏移 64，长度 11443；帧区偏移 11507，长度 704（22 条）。前两条为 `(0,0,32,32,250,2,0,0)`、`(32,0,32,32,250,2,0,0)`。

### 实际颜色平面解码与 SDL 上传

在 `SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software` 下，对以下 data02 条目运行 `build/yukari view-entry ... --once`，均返回 0。该路径完整执行 GNP 签名修正、libpng 像素解码、SDL surface/texture 创建和单帧渲染；dummy 驱动不提供可见窗口，所以这不是画面外观比对。

| 样本 | 解码尺寸 | 结果/限制 |
|---|---|---|
| `ykg\bg\12th_bg_01a.ykg` | 1124×676 | 颜色平面成功 |
| `system.ykg` | 672×32 | 颜色图集成功；22 帧尚未执行 |
| `ykg\effect\eff_orz_01.ykg` | 1024×576 | 颜色平面成功；独立 Alpha 未合成 |
| `ykg\title\title_bace.ykg` | 1024×576 | 标题背景颜色平面成功 |
| `ykg\title\logo\title_moji.ykg` | 464×237 | 标题文字图颜色平面成功 |

可选样本回归脚本不接入常规 CTest，也不保存商业资源。它只列出五包目录、确认 571 个 YKS 与关键资源，对 3 个启动脚本及上述 5 个图像抽样验证，避免反复全量反汇编：

```sh
python3 tests/original_game.py --game-dir /path/to/game --exe build/yukari
```

本机运行 3 项测试全部通过；未传 `--game-dir` 且未设置 `YUKARI_GAME_DIR` 时 3 项全部跳过。原始样本仍为只读。

## 函数调用清单

共 157 个唯一名称。以下为所有 571 脚本中静态出现次数；不代表运行次数。示例位置均为代码 DWORD 偏移，可用审计 JSON 的 `call_examples` 和 CLI 复核。

与根代理从原版 EXE 提取的 `analysis/original-engine/dispatch-candidates.json` 比较：357 个候选名称覆盖全部 157 个脚本调用名称，精确匹配缺项为零。表从 VA `0x4BC3E0` 开始，每项 12 字节；其函数分发用途随后已由原版解释器 `0x4468D0` 的名称比较和间接调用确认，见原版引擎报告。

| 函数 | 次数 | 首个静态示例 | word |
|---|---:|---|---|
| `=` | 229,041 | `data01.ykc:start.yks` | `0x1` |
| `AlreadySkipModeCheck` | 1 | `data01.ykc:yks\system\nowmodeget.yks` | `0x8` |
| `AlreadySkipModeSet` | 3 | `data01.ykc:yks\system\com\alreadyskip.yks` | `0x0` |
| `AreaSetAlreadySkip` | 2 | `data01.ykc:yks\system\createmenu.yks` | `0x68` |
| `AreaSetAutoMode` | 1 | `data01.ykc:yks\system\createmenu.yks` | `0x8E` |
| `AreaSetHistoryExit` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x92` |
| `AreaSetHistoryMode` | 1 | `data01.ykc:yks\system\createmenu.yks` | `0xE6` |
| `AreaSetHistoryNext` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x72` |
| `AreaSetHistoryPageNext` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x82` |
| `AreaSetHistoryPagePrev` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x52` |
| `AreaSetHistoryPageSize` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0xB2` |
| `AreaSetHistoryPrev` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x62` |
| `AreaSetHistoryScrollBarV` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0xA2` |
| `AreaSetIndicate` | 10 | `data01.ykc:yks\system\createmenu.yks` | `0x12C` |
| `AreaSetMouseLeftScript` | 9 | `data01.ykc:yks\direction\gt\fun\push_f.yks` | `0x2F` |
| `AreaSetSkip` | 2 | `data01.ykc:yks\system\createmenu.yks` | `0x66` |
| `AutoModeCancelLKey` | 1 | `data01.ykc:yks\system\optionsetting.yks` | `0xDF` |
| `AutoModeCharactorWaitSet` | 4 | `data01.ykc:yks\system\option\m_game.yks` | `0xA6A` |
| `AutoModeCheck` | 1 | `data01.ykc:yks\system\nowmodeget.yks` | `0xD` |
| `AutoModeSet` | 2 | `data01.ykc:yks\system\com\auto.yks` | `0x0` |
| `AutoSpeedSet` | 2 | `data01.ykc:yks\system\option\m_game.yks` | `0xA5D` |
| `Case` | 11 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0x5B` |
| `CursorAutoHide` | 5 | `data01.ykc:yks\system\option\m_game.yks` | `0x7A5` |
| `DefaultFontEffect` | 13 | `data01.ykc:yks\system\nobel_del.yks` | `0x14` |
| `DefaultFontSet` | 28 | `data01.ykc:yks\system\createfont.yks` | `0xA` |
| `DefaultKeyinSoundStopSet` | 3 | `data01.ykc:yks\system\option\m_game.yks` | `0x76D` |
| `DefaultTextWindowGroup` | 3 | `data01.ykc:yks\system\createtext.yks` | `0x61` |
| `DrawStart` | 149 | `data01.ykc:yks\direction\gt\gt-close.yks` | `0x74` |
| `DrawStop` | 20,459 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0xF` |
| `Effect` | 491 | `data01.ykc:yks\function\eye_catch05.yks` | `0x4A3` |
| `FlagReset` | 1 | `data01.ykc:yks\title\title_main.yks` | `0x24` |
| `FontSet` | 1,211 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0xC` |
| `FullScreenMode` | 2 | `data01.ykc:yks\system\option\m_game.yks` | `0x616` |
| `FullScreenModeGet` | 3 | `data01.ykc:yks\system\option\m_game.yks` | `0x150` |
| `GraphicAddColor` | 3,303 | `data01.ykc:yks\story\aino\aino_01.yks` | `0x1131` |
| `GraphicCreateSaveData` | 20 | `data01.ykc:yks\system\option\m_load.yks` | `0x388` |
| `GraphicDelete` | 45,547 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x33` |
| `GraphicFilter` | 2 | `data01.ykc:yks\story\ayaka\ayaka_01.yks` | `0x2EB1` |
| `GraphicHide` | 87,852 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x11` |
| `GraphicLoad` | 25,238 | `data01.ykc:start.yks` | `0xAD` |
| `GraphicLoadHide` | 367 | `data01.ykc:yks\direction\gt\fun\push_f.yks` | `0x54` |
| `GraphicMonochrome` | 1 | `data01.ykc:yks\story\ayaka\ayaka_13.yks` | `0x627A` |
| `GraphicMove` | 22 | `data01.ykc:yks\function\msg_effect.yks` | `0xB3` |
| `GraphicRefresh` | 6 | `data01.ykc:yks\system\option\m_game.yks` | `0x6C3` |
| `GraphicScroll` | 641 | `data01.ykc:yks\direction\gt\fun\fun_001_2.yks` | `0x18` |
| `GraphicScrollWait` | 560 | `data01.ykc:yks\direction\gt\fun\fun_001_2.yks` | `0x1E` |
| `GraphicSepia` | 6 | `data01.ykc:yks\story\ayaka\ayaka_05.yks` | `0x37C1` |
| `GraphicShow` | 23,010 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x48` |
| `GraphicSubColor` | 692 | `data01.ykc:yks\story\aino\aino_01.yks` | `0x62AF` |
| `GraphicTextRect` | 4 | `data01.ykc:yks\system\createtext.yks` | `0x3B` |
| `GraphicTransmittance` | 318 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x25` |
| `GraphicTransmittanceScroll` | 518 | `data01.ykc:yks\direction\gt\gt_loopsprite.yks` | `0x33` |
| `GraphicTransmittanceScrollWait` | 291 | `data01.ykc:yks\direction\gt\loopsprite_hide.yks` | `0xF` |
| `HistoryExitToNext` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x1E` |
| `HistoryReset` | 94 | `data01.ykc:yks\etc\prlogue_skip.yks` | `0x4D` |
| `HistorySoundGroupSet` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x3B` |
| `HistorySoundIconSet` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x4D` |
| `HistorySoundNumberSet` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x4A` |
| `HistoryTextColor` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x30` |
| `HistoryToVoiceContinue` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x21` |
| `HistoryWindowSet` | 1 | `data01.ykc:yks\system\createhistory.yks` | `0x34` |
| `IconSetAlreadySkip` | 1 | `data01.ykc:yks\system\createicon.yks` | `0x13` |
| `IconSetAutoMode` | 1 | `data01.ykc:yks\system\createicon.yks` | `0x1A` |
| `IconSetSkip` | 1 | `data01.ykc:yks\system\createicon.yks` | `0xC` |
| `Index` | 94 | `data01.ykc:yks\extra\cg\cg_main.yks` | `0x7` |
| `IndicateCase` | 93 | `data01.ykc:yks\map\love_02.yks` | `0x12B` |
| `InputDialog` | 1 | `data01.ykc:yks\system\option\m_yesno.yks` | `0x236` |
| `InputDialogGetText` | 1 | `data01.ykc:yks\system\option\m_yesno.yks` | `0x249` |
| `KeySetScript` | 8 | `data01.ykc:yks\system\createsckey.yks` | `0x0` |
| `KeyTableCase` | 1 | `data01.ykc:yks\title\title_main.yks` | `0x77` |
| `KeyWait` | 45,038 | `data01.ykc:yks\direction\mail\mail-write\w_aino_07.yks` | `0x108` |
| `KeyWaitFunctionSet` | 1 | `data01.ykc:yks\title\title_main.yks` | `0x1F` |
| `KeyWaitIcon` | 9 | `data01.ykc:start.yks` | `0xA7` |
| `KeyWaitIconPositionSet` | 3 | `data01.ykc:yks\system\createtext.yks` | `0x68` |
| `LF` | 4,272 | `data01.ykc:yks\story\aino\aino_03.yks` | `0x285` |
| `LoadEnableMode` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x19` |
| `MouseCase` | 95 | `data01.ykc:yks\etc\select_gt.yks` | `0x81` |
| `MouseCase2` | 213 | `data01.ykc:yks\etc\select_gt.yks` | `0xA9` |
| `MouseKeyCase` | 48 | `data01.ykc:yks\etc\select_gt.yks` | `0x92` |
| `MouseSelect` | 26 | `data01.ykc:yks\etc\select_gt.yks` | `0x7E` |
| `MouseSelectFunctionSet` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x29` |
| `MouseXPointGet` | 23 | `data01.ykc:yks\system\option\m_game.yks` | `0x8FE` |
| `MoviePlay` | 1 | `data01.ykc:yks\etc\playmovie.yks` | `0x8` |
| `MovieVolumeSet` | 1 | `data01.ykc:yks\etc\playmovie.yks` | `0x5` |
| `PF` | 44,845 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0xF6` |
| `RGB` | 12,980 | `data01.ykc:yks\story\aino\aino_01.yks` | `0x112C` |
| `Random` | 9 | `data01.ykc:yks\function\eye_catch01.yks` | `0x65` |
| `Restart` | 2 | `data01.ykc:yks\system\option\m_yesno.yks` | `0x288` |
| `Return` | 346 | `data01.ykc:start.yks` | `0x28` |
| `RubyFontSet` | 2 | `data01.ykc:yks\system\createtext.yks` | `0x5E` |
| `SaveDataCheck` | 6 | `data01.ykc:yks\function\nextjump.yks` | `0xAA` |
| `SaveDataCommentChange` | 1 | `data01.ykc:yks\system\option\m_save.yks` | `0x790` |
| `SaveDataCopy` | 1 | `data01.ykc:yks\system\option\m_save.yks` | `0x794` |
| `SaveDataDisplaySetToComment1` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x55` |
| `SaveDataDisplaySetToComment2` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x59` |
| `SaveDataDisplaySetToDate` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x4B` |
| `SaveDataDisplaySetToFont` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x40` |
| `SaveDataDisplaySetToGraphic` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x45` |
| `SaveDataLoad` | 6 | `data01.ykc:yks\function\nextjump.yks` | `0xDA` |
| `SaveDataLoadToCallScript` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x37` |
| `SaveDataSave` | 22 | `data01.ykc:yks\etc\select_gt.yks` | `0x29` |
| `SaveEnableMode` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x16` |
| `ScriptCall` | 8,159 | `data01.ykc:start.yks` | `0xC` |
| `ScriptExit` | 1 | `data01.ykc:yks\system\option\m_yesno.yks` | `0x2BE` |
| `ScriptJump` | 2,019 | `data01.ykc:start.yks` | `0x24` |
| `ScriptSkipMode` | 29 | `data01.ykc:start.yks` | `0xA4` |
| `ScriptSkipReset` | 37 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0x3` |
| `ScriptSubCall` | 12 | `data01.ykc:yks\etc\select_gt.yks` | `0xA2` |
| `Select` | 4 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0x58` |
| `SelectFunctionSet` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x26` |
| `SelectMode` | 3 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0x5` |
| `SelectPosition` | 5 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0x4D` |
| `SelectTTE` | 1 | `data01.ykc:yks\function\select_rtc.yks` | `0x168` |
| `SelectTransitionSet` | 1 | `data01.ykc:yks\system\optionsystem.yks` | `0x1F` |
| `SkipModeCheck` | 1 | `data01.ykc:yks\system\nowmodeget.yks` | `0x3` |
| `SkipModeSet` | 2 | `data01.ykc:yks\system\com\skip.yks` | `0xF` |
| `SoundDeleteToFade` | 1,387 | `data01.ykc:yks\etc\select_gt.yks` | `0xFD` |
| `SoundGroupAutomodeSkipSet` | 13 | `data01.ykc:yks\system\optionvoice.yks` | `0xA2` |
| `SoundGroupSet` | 18 | `data01.ykc:yks\system\optionvoice.yks` | `0x0` |
| `SoundGroupVolumeSet` | 62 | `data01.ykc:yks\system\option\vo_volume.yks` | `0x11D` |
| `SoundLoad` | 216 | `data01.ykc:start.yks` | `0x68` |
| `SoundLoadStream` | 1,500 | `data01.ykc:yks\etc\select_gt.yks` | `0x3E` |
| `SoundPlay` | 16,919 | `data01.ykc:yks\direction\gt\fun\fun_003.yks` | `0x29` |
| `SoundPlayLoop` | 1,993 | `data01.ykc:yks\etc\select_gt.yks` | `0x42` |
| `SoundPlayWait` | 246 | `data01.ykc:yks\etc\select_gt.yks` | `0x23` |
| `SoundStop` | 3,718 | `data01.ykc:yks\etc\select_gt.yks` | `0x3B` |
| `SoundStopALL` | 23 | `data01.ykc:yks\extra\cg\cg_main_e.yks` | `0x0` |
| `SoundVolume` | 781 | `data01.ykc:yks\direction\gt\fun\fun_003.yks` | `0x2D` |
| `SoundVolumeToFade` | 193 | `data01.ykc:yks\roll\roll_aino.yks` | `0x5A9` |
| `SpriteCreate` | 23,871 | `data01.ykc:yks\direction\gt\fun\push_f.yks` | `0x5E` |
| `SpriteDelete` | 41,487 | `data01.ykc:yks\extra\cg\cg_mode_ay.yks` | `0x32E` |
| `SpriteMotionWait` | 10 | `data01.ykc:yks\direction\mail\call_02.yks` | `0x26` |
| `SpriteMove` | 52 | `data01.ykc:yks\system\option\m_game.yks` | `0x934` |
| `StrOut` | 46,340 | `data01.ykc:yks\direction\mail\mail-write\w_aino_07.yks` | `0x105` |
| `StrOutNW` | 43 | `data01.ykc:yks\story\aino\aino_03.yks` | `0x1A3D` |
| `StrOutNWC` | 22,935 | `data01.ykc:yks\story\aino\aino_01.yks` | `0xA3` |
| `SubMenuEnableMode` | 10 | `data01.ykc:start.yks` | `0xAA` |
| `SubMenuScript` | 11 | `data01.ykc:yks\system\nobel_del.yks` | `0x3E` |
| `SystemDataWrite` | 1 | `data01.ykc:yks\title\title_main.yks` | `0x22` |
| `SystemKeySetHistoryMode` | 1 | `data01.ykc:yks\system\createsckey.yks` | `0x18` |
| `SystemKeySetWindowErase` | 1 | `data01.ykc:yks\system\createsckey.yks` | `0x1C` |
| `TTE` | 4 | `data01.ykc:yks\function\select_rtc.yks` | `0x231` |
| `TextBorderColor` | 4,496 | `data01.ykc:yks\direction\mail\selectmail.yks` | `0x54` |
| `TextColor` | 4,492 | `data01.ykc:yks\story\aino\gt\aino_02_gt.yks` | `0x43` |
| `TextSpeedSet` | 2 | `data01.ykc:yks\system\option\m_game.yks` | `0x9E0` |
| `TextWindowSet` | 3 | `data01.ykc:yks\system\createtext.yks` | `0x18` |
| `TimeInterrupt` | 13 | `data01.ykc:yks\function\select_rtc.yks` | `0x16C` |
| `TimerGet` | 6 | `data01.ykc:yks\roll\roll_aino.yks` | `0x68` |
| `TimerStart` | 6 | `data01.ykc:yks\roll\roll_aino.yks` | `0x41` |
| `TimerWait` | 12 | `data01.ykc:yks\roll\roll_aino.yks` | `0x5A5` |
| `Transition` | 19,533 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x14` |
| `TransitionModeSet` | 5 | `data01.ykc:yks\system\option\m_game.yks` | `0x688` |
| `UserTransition` | 730 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x29` |
| `Wait` | 1,261 | `data01.ykc:yks\direction\gt\fun\fun_001.yks` | `0x2E` |
| `WindowMessageCase` | 1 | `data01.ykc:yks\system\option\m_game.yks` | `0x5EC` |
| `WindowNameSet` | 686 | `data01.ykc:yks\extra\cg\cg_main.yks` | `0x4` |
| `if` | 73,122 | `data01.ykc:start.yks` | `0x37` |
