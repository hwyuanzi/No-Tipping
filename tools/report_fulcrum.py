#!/usr/bin/env python3
"""Summarize the documented benchmark matrix without rewriting maintained docs."""
import argparse
from collections import defaultdict
import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read_arena(directory, name, expected):
    with (directory / name).open() as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != expected or any(row['reason'] != 'tipping' for row in rows):
        raise ValueError(f'{name}: expected {expected} completed games without runtime failures')
    return rows


def read_protocol(directory, name, expected, clock):
    result = json.loads((directory / name).read_text())
    games = result['games']
    if result['clock'] != clock or len(games) != expected or any(game['reason'] != 'tipping' for game in games):
        raise ValueError(f'{name}: expected {expected} completed games at a {clock}-second clock')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--results-dir', type=Path, default=ROOT / 'benchmarks/results/2026-10-03')
    parser.add_argument('--output', type=Path, default=ROOT / 'benchmarks/local/benchmark_summary.json')
    args = parser.parse_args()
    rows = read_arena(args.results_dir, 'arena_final.csv', 64)
    fast120 = read_arena(args.results_dir, 'arena_120_k15.csv', 2) + read_arena(args.results_dir, 'arena_120_k24.csv', 2)
    protocol20 = read_protocol(args.results_dir, 'protocol_20.json', 20, 20)
    protocol120 = read_protocol(args.results_dir, 'protocol_120.json', 2, 120)
    # Historical replays predate embedded provenance. Never relabel their results
    # with the hash of a later strategy just because the working tree changed.
    metadata_path = args.results_dir / 'metadata.json'
    metadata = json.loads(metadata_path.read_text()) if metadata_path.exists() else {}
    hashes = {result.get('strategy_sha256', metadata.get('strategy_sha256')) for result in (protocol20, protocol120)}
    if None in hashes or len(hashes) != 1:
        raise ValueError('Missing or inconsistent strategy provenance; provide metadata.json for older replays')
    strategy_hash = hashes.pop()
    if metadata.get('strategy_sha256', strategy_hash) != strategy_hash:
        raise ValueError('Replay provenance does not match metadata.json')
    groups = defaultdict(list)
    for row in rows:
        groups[row['opponent']].append(row)
    short = protocol20['games']
    full = protocol120['games']
    summary = {
        'strategy_sha256': strategy_hash,
        'arena_5_seconds': {
            'games': len(rows), 'wins': sum(int(row['win']) for row in rows),
            'max_our_seconds': max(float(row['our_seconds']) for row in rows),
            'by_opponent': {name: {'wins': sum(int(row['win']) for row in items), 'games': len(items)} for name, items in groups.items()},
        },
        'default_120_seconds': {
            'reference_games': len(fast120), 'reference_wins': sum(int(row['win']) for row in fast120),
            'protocol_games': len(full), 'protocol_wins': sum(game['winner'] == 'Fulcrum' for game in full),
        },
        'protocol_20_seconds': {
            'games': len(short), 'wins': sum(game['winner'] == 'Fulcrum' for game in short),
            'wins_excluding_k1': sum(game['winner'] == 'Fulcrum' for game in short if game['k'] > 1),
            'games_excluding_k1': sum(game['k'] > 1 for game in short),
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, indent=2) + '\n')
    current_hash = hashlib.sha256((ROOT / 'bots/fulcrum/strategy.cpp').read_bytes()).hexdigest()
    print(json.dumps(summary, indent=2))
    print('Current strategy matches measured source:', current_hash == strategy_hash)


if __name__ == '__main__':
    main()
