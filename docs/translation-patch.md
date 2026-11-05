# Translation patch

将补丁命名为 `translation_patch` 并放在游戏目录根部，运行 `play GAME_DIRECTORY` 时会自动检测和加载。仍可通过 `--translation-patch PATH` 指定其他文件；`--original-encoding` 会覆盖补丁声明的编码。

补丁在脚本解析后按脚本名和原始字符串数据偏移替换 UTF-8 字符串常量，不修改资源包。原文不一致或重复记录会报错；未列出的常量保留原文。脚本缓存、剧情、姓名、选项共享此替换路径。应使用独立 `--state-dir`，不要混用不同补丁/编码的旧存档。

使用 `dump-text` 导出的 CSV 填写 `translation` 后运行：

```
python3 tools/build_translation_patch.py translation.csv translation_patch
yukari validate-translation-patch GAME_DIRECTORY translation_patch
```

格式首行为 `YUKARI_TRANSLATION_PATCH_1`。可选的第二行 `@encoding<TAB>CP936` 声明译文编码；旧版无元数据补丁仍可读取。后续每行四列，以 tab 分隔：脚本名 UTF-8 十六进制、原数据偏移十六进制、原文 UTF-8 十六进制、译文 UTF-8 十六进制。不支持空译文。编译前应复核控制符、变量占位符、资源路径及字体名。人名音译需要在完整剧情校对后定稿。

macOS 使用补丁时会优先选择系统中文字体作为文字回退，避免日文字体缺少简体字形；未启用补丁时仍使用原版日文字体路径。补丁译文需能编码为补丁声明或 `--original-encoding` 选择的编码。

生成工具在 CP936 下会将常见不可编码符号转换为兼容写法（例如 `♪ → ～`、`・ → ·`，半角片假名颜文字转换为全角形式）；其余不可编码字符会直接报错。`validate-translation-patch` 也会使用运行时编码器检查译文，并报告失败脚本与字符串偏移，避免运行到相应剧情时才退出。
