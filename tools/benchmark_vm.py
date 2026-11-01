"""Compare pure VM dispatch/read-history costs on a small deterministic loop."""
import argparse
import json
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tests'))
from runtime_fixtures import Script, archive
with tempfile.TemporaryDirectory(prefix='yukari-vm-benchmark-') as directory:
    game = Path(directory)
    script = Script()
    label, _ = script.control('loop', 0)
    script.code.append(label)
    script.call('=')
    script.call('ScriptJump')
    script.code[-1] = 1
    script.code.append(label)
    (game / 'data01.ykc').write_bytes(archive([('start.yks', script.finish())]))
    result = {}
    for name, binary in [('baseline', args.baseline), ('optimized', args.exe)]:
        samples = []
        for _ in range(7):
            started = time.perf_counter()
            run = subprocess.run([str(binary.resolve()), 'vm', str(game), '--max-steps', '1000000'],
                                 capture_output=True, text=True, timeout=30)
            elapsed = (time.perf_counter() - started) * 1000
            if run.returncode != 3 or 'step-limit steps=1000000' not in run.stdout:
                raise SystemExit(run.stdout + run.stderr)
            samples.append(elapsed)
        result[name] = dict(samples_ms=samples, median_ms=statistics.median(samples))
    result['scope'] = '1,000,000 identical label/expression/jump steps; wall time includes process startup and loading a tiny synthetic archive'
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
