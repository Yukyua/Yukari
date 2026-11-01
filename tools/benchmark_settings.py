"""Opt-in game settings frame budget test on a real macOS Metal display.

Uses a fresh writable state directory; game assets are read-only. Run an
optimized build without other benchmarks/tests running concurrently.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import platform
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir', type=Path, required=True)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True, help='New results directory')
parser.add_argument('--budget-ms', type=float, default=20)
args = parser.parse_args()
root = args.output.resolve()
root.mkdir(parents=True, exist_ok=False)
state = root / 'state'
lines = ['click 200 370', 'menu option', 'wait-ms 1000',
         f'profile settings_idle 240 {args.budget_ms}', 'profile-wait',
         f'profile settings_interactive 1000 {args.budget_ms}']
# The first cycle deliberately uses cold font/effect variants. Do not prewarm
# them or discard their outliers. Later cycles exercise warm redraw/hover.
for _ in range(8):
    for x, y in [(490, 172), (760, 100), (350, 172), (620, 100),
                 (200, 165), (460, 235), (620, 235)]:
        lines += [f'hover {x} {y}', f'click {x} {y}', 'wait-ms 60']
lines += ['profile-wait', f'capture {root / "screen.png"}',
          'click 930 190', 'wait-ms 500',
          f'profile sound_interactive 600 {args.budget_ms}']
for _ in range(6):
    for x, y in [(100, 95), (250, 155), (400, 95), (610, 95), (730, 185), (250, 95)]:
        lines += [f'hover {x} {y}', f'click {x} {y}', 'wait-ms 60']
lines += ['profile-wait', f'capture {root / "sound.png"}',
          f'profile sound_idle 240 {args.budget_ms}', 'profile-wait', 'quit']
# Action paths are whitespace-delimited by the engine.
if any(c.isspace() for c in str(root)):
    parser.error('--output must not contain whitespace')
actions = root / 'settings.actions'
actions.write_text('\n'.join(lines) + '\n')
env = os.environ.copy()
for key in ('SDL_VIDEODRIVER', 'SDL_RENDER_DRIVER', 'SDL_VIDEO_HIGHDPI_DISABLED'):
    env.pop(key, None)
command = [str(args.exe.resolve()), 'play', str(args.game_dir.resolve()),
           '--require-gpu', '--mute', '--seed', '0', '--actions', str(actions),
           '--state-dir', str(state), '--max-ms', '60000']
with (root / 'run.log').open('w') as log:
    result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, env=env, timeout=90)
profiles = {}
expected = {'settings_idle': 240, 'settings_interactive': 1000,
            'sound_interactive': 600, 'sound_idle': 240}
for label, count in expected.items():
    path = state / f'frames-{label}.csv'
    if not path.exists():
        continue
    with path.open() as csv_file:
        rows = list(csv.DictReader(csv_file))
    times = sorted(float(row['frame_ms']) for row in rows)
    import math
    quantile = lambda p: times[math.ceil(p * len(times)) - 1]
    profiles[label] = dict(frames=len(rows), p50_ms=quantile(.5), p95_ms=quantile(.95),
                           p99_ms=quantile(.99), max_ms=max(times),
                           over_budget=sum(t >= args.budget_ms for t in times))
    if len(rows) != count:
        result.returncode = 1
summary = dict(platform=platform.platform(), command=command, budget_ms=args.budget_ms,
               profiles=profiles, exit_code=result.returncode)
(root / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
if result.returncode or len(profiles) != len(expected) or any(p['over_budget'] for p in profiles.values()):
    raise SystemExit('Settings frame budget failed; inspect run.log and CSV files.')
