#!/usr/bin/env python3
"""Resume a GPT translation of dump-text JSONL and build translation_patch."""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import threading

JAPANESE = re.compile(r"[\u3040-\u30ff\u3400-\u9fff]")
EXCLUDED_KINDS = {"resource", "script", "font"}
GLOSSARY = (
    "善一=善一；アイノ=艾诺；綾花=绫花；羽美=羽美；いろは=彩华；"
    "水穂=水穗；はつね=初音；柚菜=柚菜；紗希=纱希；鈴乃=铃乃；孝=孝"
)
CORRECTIONS = {
    "当の妹──桜庭水穂（さくらば　みなほ）は、特に気にした素振りもなく俺を見てにっこり微笑む。":
        "而我的亲妹妹──樱庭水穗，却毫不在意地望着我，嫣然一笑。",
    "幼馴染の羽美──瑚乃瀬羽美（このせ　はみ）に、呆れたような眼差しを向けられてしまう。":
        "我的青梅竹马羽美──瑚乃濑羽美，向我投来无奈的目光。",
    "天ノ戸河学園（あまのとがわがくえん）。": "天之户河学园。",
    "自宅のある東側住宅街から駅前を抜け、頬笑町（ほほえみちょう）を縦断する大きな川を越えて歩くこと数分。":
        "从位于东侧住宅区的家出发，穿过车站前，再越过纵贯微笑町的大河，步行数分钟。",
    "八乙女いろは（やおとめ　いろは）。俺達のクラスメイトで、羽美とは付属生のころからの仲の良いお友達だ。":
        "八乙女彩华。她是我们的同班同学，从附属学校时期起就是羽美的好朋友。",
    "荻山鈴乃（おぎやま　すずの）。八乙女と同じく、羽美とは付属生の頃からの友人だ。":
        "荻山铃乃。和八乙女一样，她也是羽美从附属学校时期起的朋友。",
    "犬飼孝（いぬかい　たかし）、俺の付属時代からの悪友だ。":
        "犬饲孝，是我从附属学校时期就认识的损友。",
    "と、担任の芹沢紗希（せりざわ　さき）先生がのんびりとした足取りで教室に入ってきた。":
        "说着，班主任芹泽纱希老师迈着悠闲的步伐走进了教室。",
    "ああ、なるほど。風紀を乱すカップルに、生徒会長の藤井柚菜（ふじい　ゆずな）先輩が怒鳴っていたのか。":
        "哦，原来如此。学生会长藤井柚菜学姐是在训斥扰乱风纪的情侣。",
    "そこに、溜息交じりに副会長の仁科綾花（にしな　あやか）先輩が割って入る。":
        "这时，副会长仁科绫花学姐叹着气插了进来。",
    "「ハグ、抱擁だろ。注釈で『抱き合うにも色々あるよな。前からとか後からとか。どれが一番良いか調べてくれ』って書かれてる」":
        "「是拥抱吧。注释里写着『拥抱也有很多种方式，比如从正面或从背后。请调查哪一种最好』」",
}


def normalize_translation(source, target):
    target = target.replace("っ", "！").replace("・", "·").replace("♪", "～")
    return CORRECTIONS.get(source, target)


def load_rows(path):
    with path.open(encoding="utf-8") as file:
        return [json.loads(line) for line in file if line.strip()]


def candidates(rows):
    result = []
    seen = set()
    for row in rows:
        source = row["source"]
        if source in seen or not JAPANESE.search(source) or EXCLUDED_KINDS.intersection(row["kinds"]):
            continue
        seen.add(source)
        result.append({"source": source, "script": row["script"], "kinds": row["kinds"]})
    return result


def load_progress(path):
    done = {}
    if not path.exists():
        return done
    with path.open(encoding="utf-8") as file:
        for line in file:
            record = json.loads(line)
            done[record["source"]] = normalize_translation(record["source"], record["translation"])
    return done


def translate_batch(index, batch, args):
    payload = [{"id": i, **item} for i, item in enumerate(batch)]
    prompt = f"""Translate every item in the JSON array below from Japanese into polished natural Simplified Chinese for a visual novel.
Return exactly one object for every id, in identical order, using the required schema. Do not omit, merge, summarize, censor, or explain any item.
Preserve the source's quotation marks, punctuation, line breaks, ellipses, placeholders, @ control syntax, Latin tokens, and emotional tone. Translate names and recurring terms consistently. Do not translate file paths or identifiers (they have already been excluded).
Name glossary: {GLOSSARY}.
JSON input:
{json.dumps(payload, ensure_ascii=False, separators=(',', ':'))}
"""
    output = args.work / f"batch-{index:04}.json"
    command = [
        "codex", "exec", "--ephemeral", "--ignore-user-config", "-m", "gpt-5.6-sol",
        "-c", 'model_reasoning_effort="low"', "-s", "read-only", "-C", str(args.repository),
        "--output-schema", str(args.schema), "-o", str(output), "-",
    ]
    for attempt in range(3):
        completed = subprocess.run(command, input=prompt, text=True, stdout=subprocess.DEVNULL,
                                   stderr=subprocess.PIPE, timeout=args.timeout)
        if completed.returncode:
            if attempt == 2:
                raise RuntimeError(f"batch {index} failed: {completed.stderr[-2000:]}")
            continue
        try:
            response = json.loads(output.read_text(encoding="utf-8"))
            translated = response["translations"]
            if [entry["id"] for entry in translated] != list(range(len(batch))):
                raise ValueError("missing or reordered ids")
            values = [normalize_translation(item["source"], entry["translation"])
                      for item, entry in zip(batch, translated)]
            if any(not value for value in values):
                raise ValueError("empty translation")
            return [(item["source"], value, item["script"]) for item, value in zip(batch, values)]
        except (KeyError, ValueError, json.JSONDecodeError) as error:
            if attempt == 2:
                raise RuntimeError(f"batch {index} invalid response: {error}") from error
    raise AssertionError("unreachable")


def hex_text(text):
    return text.encode("utf-8").hex()


def write_patch(rows, translations, path):
    records = []
    covered = 0
    for row in rows:
        target = translations.get(row["source"])
        if target is None or EXCLUDED_KINDS.intersection(row["kinds"]):
            continue
        script, offset = row["id"].rsplit("#str:", 1)
        records.append("\t".join((hex_text(script), offset, hex_text(row["source"]), hex_text(target))))
        covered += 1
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text("YUKARI_TRANSLATION_PATCH_1\n@encoding\tCP936\n" + "\n".join(records) + "\n", encoding="utf-8")
    temporary.replace(path)
    return covered


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--batch-size", type=int, default=600)
    parser.add_argument("--timeout", type=int, default=900)
    options = parser.parse_args()
    options.repository = Path(__file__).resolve().parents[1]
    options.schema = options.repository / "tools/translation_response.schema.json"
    options.output.mkdir(parents=True, exist_ok=True)
    options.work = options.output / ".gpt56-batches"
    options.work.mkdir(exist_ok=True)
    progress = options.output / "gpt56-progress.jsonl"
    rows = load_rows(options.source)
    items = candidates(rows)
    done = load_progress(progress)
    pending = [item for item in items if item["source"] not in done]
    batches = [pending[i:i + options.batch_size] for i in range(0, len(pending), options.batch_size)]
    lock = threading.Lock()
    print(f"unique={len(items)} resumed={len(done)} pending={len(pending)} batches={len(batches)}", flush=True)
    with progress.open("a", encoding="utf-8") as journal:
        with concurrent.futures.ThreadPoolExecutor(max_workers=options.jobs) as pool:
            futures = {pool.submit(translate_batch, i, batch, options): i for i, batch in enumerate(batches)}
            for future in concurrent.futures.as_completed(futures):
                index = futures[future]
                records = future.result()
                with lock:
                    for source, translation, script in records:
                        journal.write(json.dumps({"source": source, "translation": translation,
                                                  "script": script}, ensure_ascii=False) + "\n")
                        done[source] = translation
                    journal.flush()
                    os.fsync(journal.fileno())
                    print(f"batch={index} translated={len(done)}/{len(items)}", flush=True)
    covered = write_patch(rows, done, options.output / "translation_patch")
    report = {
        "status": "complete machine translation; human editing recommended",
        "model": "gpt-5.6-sol",
        "source_rows": len(rows),
        "candidate_rows": sum(1 for row in rows if JAPANESE.search(row["source"]) and
                              not EXCLUDED_KINDS.intersection(row["kinds"])),
        "unique_candidates": len(items),
        "translated_unique": len(done),
        "patch_rows": covered,
        "glossary": GLOSSARY,
    }
    (options.output / "coverage.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                                                   encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
