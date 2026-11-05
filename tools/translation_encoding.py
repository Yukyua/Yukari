"""Explicit visual substitutions for characters unavailable in CP936."""

CP936_SUBSTITUTIONS = str.maketrans({
    "♪": "～", "・": "·", "´": "′", "∀": "Ａ", "･": "·",
    "ﾉ": "ノ", "ﾟ": "°", "≪": "《", "≫": "》", "ｼ": "シ",
    "｡": "。", "ｷ": "キ", "ﾘ": "リ", "ｯ": "ッ", "､": "、",
})


def compatible_translation(text, encoding="CP936"):
    if encoding == "CP936":
        text = text.translate(CP936_SUBSTITUTIONS)
    # Never silently discard an unmappable character.
    text.encode({"CP936": "gbk", "CP932": "cp932"}[encoding])
    return text
