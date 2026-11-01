# 全脚本文本导出

```sh
./build/yukari dump-text game translation/original-engine
```

命令读取所有有效原版 YKS 脚本中的 **type-5 字符串常量**，包括未运行的路线、未引用常量、空串和技术字符串。无需启动游戏、SDL 窗口或执行 VM。YKS 的 XOR 解密只发生在内存，文本按现有解析器的 CP932 转成 UTF-8。

输出目录必须尚不存在，且位于源游戏目录之外；已存在的译稿不会被覆盖。解析或写入失败会报告对应脚本并清理本次新建的输出目录，避免把不完整结果当成全量导出。

## 文件

| 文件 | 用途 |
| --- | --- |
| `translation.csv` | UTF-8 BOM，可用表格软件或 CSV 工具编辑；译文填入 `translation` 列 |
| `texts.jsonl` | UTF-8、无 BOM，每行一条完整记录，适合脚本和翻译工具处理 |
| `manifest.json` | 格式版本、编码、脚本清单、来源包、加密标记、指纹及统计 |
| `README.txt` | 字段与编辑约定 |

两个文本文件包含相同记录、相同顺序。CSV 对引号和跨行字段作标准转义，不能按物理行简单分割。推荐 Python 用 `open(..., encoding='utf-8-sig', newline='')` 和 `csv.DictReader` 读取。

每条记录按 **脚本名 + 解密数据段偏移** 生成稳定 `id`。相同文字位于不同偏移时分别保留，以支持不同语境；多个元素共享同一偏移时合为一条，并保留所有 `elements` 和 `uses`。

JSONL 字段：

- `id`、`archive`、`script`：定位译文记录和原版来源。
- `data_offset`：解密数据段内字节偏移；`file_offset`：YKS 内偏移；`archive_offset`：YKC 内偏移。
- `elements`：引用此常量的元素索引。
- `uses`：每个引用的函数名、指令 `word`、参数 `argument` 和元素索引。参数索引从 0 开始；word 是 DWORD 指令位置，不是文件字节偏移。
- `kinds`：按调用位置提供 `dialogue`、`name`、`choice`、`chapter`、`ui`、`font`、`script`、`resource`、`expression`、`other`、`unused` 提示。共享常量可同时有多种用途；分类是提示，未识别用途也照常导出。
- `source`、`translation`：完整原文与空译文列。换行、Ruby、`@` 控制码等保持原样，不拼接或改写。
- `source_hex`：不含结尾 NUL 的原始解密 CP932 字节，保留重复编码及私有字符的原字节依据。
- `script_hash`：整个原始 YKS 的 FNV-1a64 指纹，与 manifest 一致；用于定位原始版本，不是加密认证。

先按 `kinds` 筛选剧情、姓名、选项、章节和界面内容，再检查 `expression`、`other` 与 `unused`。资源路径、脚本路径和字体名保留是为了全量覆盖，不应当作普通剧情直接翻译。保留 ID、原文和定位列；本功能目前只负责导出，**尚不提供译文回写或重新打包**。

## 范围

使用与运行时相同的原版包优先级：`data04 → data01 → data02 → data03 → data05 → … → data09`，ASCII 大小写不敏感，首个同名资源生效。被更高优先级覆盖的旧脚本不重复导出；不会挂载 CNP、汉化 EXE 或补丁 DLL。

这是静态脚本文本表，不是一次运行的字幕记录。变量、拼接和分支的字符串片段全部保留，运行时才生成的最终句子不做推断；图像中的烘焙文字、视频画面文字和 EXE 内部硬编码提示不属于 YKS 字符串表，也不做 OCR。

## game 验证

本次实际输出在 `translation/original-engine/`：571 个有效脚本、139,195 条字符串。已逐条核对 CSV / JSONL 的 ID、原文、原始字节、引用和分类一致，manifest 统计一致。该目录被 `.gitignore` 排除，避免把本地游戏全文提交进源代码仓库。

合成测试覆盖：加密/未加密脚本、大小写及包优先级、共享偏移与重复原文、未引用/空常量、引号/换行/制表符/控制码、原始字节和两级文件偏移、确定性输出、拒绝覆盖、符号链接指向源目录，以及损坏脚本的失败清理。测试同时检查源资源的哈希没有变化。
