"""Opt-in real Metal/HiDPI NEW GAME VM/render benchmark using read-only assets."""
import argparse
import csv
import json
import math
import os
from pathlib import Path
import platform
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--baseline', type=Path, help='Optional saved binary built with the same optimization level')
parser.add_argument('--output', type=Path, required=True, help='New results directory without whitespace')
args = parser.parse_args()
root = args.output.resolve()
if any(c.isspace() for c in str(root)):
    parser.error('--output must not contain whitespace')
root.mkdir(parents=True, exist_ok=False)
env = os.environ.copy()
for key in ('SDL_VIDEODRIVER', 'SDL_RENDER_DRIVER', 'SDL_VIDEO_HIGHDPI_DISABLED'):
    env.pop(key, None)
profiles = {}
for label, exe in [('baseline', args.baseline), ('optimized', args.exe)]:
    if exe is None:
        continue
    out = root / label
    out.mkdir()
    state = out / 'state'
    actions = out / 'newgame.actions'
    actions.write_text('profile newgame 1600 0\nclick 200 370\nwait-page 1\nprofile-wait\n'
                       f'capture {out / "first-page.png"}\nquit\n')
    command = [str(exe.resolve()), 'play', str(args.game_dir.resolve()), '--require-gpu', '--mute',
               '--seed', '0', '--actions', str(actions), '--state-dir', str(state), '--max-ms', '60000']
    with (out / 'run.log').open('w') as log:
        subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=90, check=True)
    with (state / 'frames-newgame.csv').open() as csv_file:
        rows = list(csv.DictReader(csv_file))
    if len(rows) != 1600:
        raise SystemExit('Incomplete frame capture')
    times = sorted(float(row['frame_ms']) for row in rows)
    log_text = (out / 'run.log').read_text()
    if 'TEXT_READY page=1' not in log_text:
        raise SystemExit('First story page was not reached')
    host = {}
    for name, count, total, maximum in re.findall(
            r'HOST_PROFILE name=(\S+) calls=(\d+) total_ms=(\S+) max_ms=(\S+)', log_text):
        host[name] = dict(calls=int(count), total_ms=float(total), max_ms=float(maximum))
    profiles[label] = dict(command=command, frames=len(rows),
                           p99_ms=times[math.ceil(.99*len(times))-1], max_ms=max(times),
                           vm_steps=sum(int(row['vm_steps']) for row in rows),
                           vm_inclusive_ms=sum(float(row['vm_ms']) for row in rows),
                           gpu_operations=sum(int(row['gpu_operations']) for row in rows), host=host)
summary = dict(platform=platform.platform(), scope='Click NEW GAME through first story page; 1600 complete real-time frames', profiles=profiles)
(root / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
