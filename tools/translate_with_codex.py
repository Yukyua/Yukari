#!/usr/bin/env python3
"""Resume a GPT translation of dump-text JSONL and build translation_patch."""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import re
import subprocess
import threading
from translation_encoding import compatible_translation

JAPANESE = re.compile(r"[\u3040-\u30ff\u3400-\u9fff]")
CONTROL = re.compile(r"@r\((\d+),([^)]*)\)")
EMPHASIS_CONTROL = re.compile(r"@r\(\d+,・+\)")
TRANSLATABLE_KINDS = {"dialogue", "name", "choice", "chapter", "ui"}
PIT_PREFIX = "yks\\pit\\"


def normalize_translation(_source, target):
    return compatible_translation(target)


def is_translatable(row):
    return (bool(TRANSLATABLE_KINDS.intersection(row["kinds"])) or
            (row["script"].lower().startswith(PIT_PREFIX) and "expression" in row["kinds"]))


def controls(text):
    return [match.group(0) for match in CONTROL.finditer(text)]


def controls_valid(source, target):
    original = list(CONTROL.finditer(source))
    translated = list(CONTROL.finditer(target))
    if len(original) != len(translated):
        return False
    for left, right in zip(original, translated):
        if left.group(1) != right.group(1):
            return False
        if (set(left.group(2)) == {"・"} and
                compatible_translation(left.group(0)) != right.group(0)):
            return False
    return True


def mask_controls(text):
    found = EMPHASIS_CONTROL.findall(text)
    for index, control in enumerate(found):
        text = text.replace(control, f"__YUKARI_CONTROL_{index}__", 1)
    return text, found


def restore_controls(text, found):
    for index, control in enumerate(found):
        placeholder = f"__YUKARI_CONTROL_{index}__"
        if text.count(placeholder) != 1:
            raise ValueError(f"missing or duplicated control placeholder {placeholder}")
        text = text.replace(placeholder, control)
    if "__YUKARI_CONTROL_" in text:
        raise ValueError("unexpected control placeholder")
    return text


def load_rows(path):
    with path.open(encoding="utf-8") as file:
        return [json.loads(line) for line in file if line.strip()]


def candidates(rows):
    result = []
    seen = set()
    for row in rows:
        source = row["source"]
        if source in seen or not JAPANESE.search(source) or not is_translatable(row):
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
            source = record["source"]
            target = normalize_translation(source, record["translation"])
            if controls_valid(source, target):
                done[source] = target
    return done


def translate_batch(index, batch, args):
    masked = [mask_controls(item["source"]) for item in batch]
    payload = [{"id": i, **item, "source": masked[i][0]} for i, item in enumerate(batch)]
    prompt = f"""Translate every item in the JSON array below from Japanese into polished natural Simplified Chinese for a visual novel.
Return exactly one object for every id, in identical order, using the required schema. Do not omit, merge, summarize, censor, or explain any item.
Preserve the source's quotation marks, punctuation, line breaks, ellipses, placeholders, @r control count and numeric first arguments, Latin tokens, and emotional tone. You may translate the textual second argument of @r, but never change a second argument made only of ・ emphasis dots. Translate names and recurring terms consistently. Do not translate file paths or identifiers (they have already been excluded).
Name and terminology glossary (follow it exactly): {args.glossary}.
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
            values = [normalize_translation(item["source"],
                                             restore_controls(entry["translation"], masked[i][1]))
                      for i, (item, entry) in enumerate(zip(batch, translated))]
            if any(not value or not controls_valid(item["source"], value)
                   for item, value in zip(batch, values)):
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
        if target is None or not is_translatable(row):
            continue
        target = compatible_translation(target)
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
    parser.add_argument("--glossary", type=Path,
                        help="UTF-8 file containing game-specific name and terminology mappings")
    options = parser.parse_args()
    options.repository = Path(__file__).resolve().parents[1]
    options.schema = options.repository / "tools/translation_response.schema.json"
    options.output.mkdir(parents=True, exist_ok=True)
    options.work = options.output / ".gpt56-batches"
    options.work.mkdir(exist_ok=True)
    options.glossary = (options.glossary.read_text(encoding="utf-8").strip()
                        if options.glossary else "No game-specific glossary was supplied")
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
                              is_translatable(row)),
        "unique_candidates": len(items),
        "translated_unique": len(done),
        "patch_rows": covered,
        "glossary": options.glossary,
    }
    (options.output / "coverage.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                                                   encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
