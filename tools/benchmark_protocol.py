#!/usr/bin/env python3
"""Official runner benchmark; both seats, real cumulative clocks, persistent stdin JSON."""
import argparse
import json
from pathlib import Path
import sys
import hashlib
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from notipping.runner import load_bots, play

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--clock', type=float, default=20)
    parser.add_argument('--ks', type=int, nargs='+', default=[1, 4, 8, 15, 24])
    parser.add_argument('--opponents', nargs='+', default=['Sample Python', 'Random A'])
    parser.add_argument('--output', default='benchmarks/local/protocol_results.json')
    args = parser.parse_args()
    manifest = json.loads((ROOT / 'bots.json').read_text())
    manifest.append({'name': 'Fulcrum', 'icon': '⚖️', 'color': '#19A7CE', 'cwd': 'bots/fulcrum', 'command': ['./bot']})
    output = Path(args.output)
    if not output.is_absolute():
        output = ROOT / output
    output.parent.mkdir(parents=True, exist_ok=True)
    strategy_hash = hashlib.sha256((ROOT / 'bots/fulcrum/strategy.cpp').read_bytes()).hexdigest()
    # Resolve paths before loading a temporary manifest; no local paths are committed.
    for bot in manifest:
        bot['cwd'] = str(ROOT / bot['cwd'])
    with tempfile.TemporaryDirectory(prefix='fulcrum-roster-') as temporary:
        path = Path(temporary) / 'bots.json'
        path.write_text(json.dumps(manifest, indent=2) + '\n')
        bots = {b['name']: b for b in load_bots(path)}
    games = []
    for k in args.ks:
        for other in args.opponents:
            for seat in (0, 1):
                ordered = [bots['Fulcrum'], bots[other]] if seat == 0 else [bots[other], bots['Fulcrum']]
                result = play(ordered, k, args.clock, f'protocol-{k}-{other}-{seat}')
                result['k'] = k
                games.append(result)
                print(f"k={k}, seat={seat}, versus {other}: {result['winner']}, {result['reason']}", flush=True)
                output.write_text(json.dumps({'strategy_sha256': strategy_hash, 'clock': args.clock, 'games': games}, indent=2) + '\n')
                if result['reason'] != 'tipping':
                    raise RuntimeError('Protocol or runtime failure: ' + result['reason'])
    print('Wins:', sum(g['winner'] == 'Fulcrum' for g in games), '/', len(games), flush=True)

if __name__ == '__main__':
    main()
