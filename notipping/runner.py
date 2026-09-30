import argparse
import colorsys
import json
import os
import re
from pathlib import Path
import secrets
import signal
import subprocess
import sys
import tempfile
import time
from itertools import combinations
from .game import Game, IllegalMove

OUTPUT_LIMIT = 65536
BOT_ICONS = (
    '🐙', '🤖', '🎲', '🐍', '🦊', '🚀', '🐸', '🐼', '🦉', '🐝', '🐬', '🦄',
    '🐢', '🦖', '🦋', '🐳', '🐧', '🐱', '🐯', '🦁', '🌟', '🔮', '🛸', '🍀',
    '⚡', '🎯', '🧩', '🦜', '🐨', '🦈', '🌈', '🎨',
)


def _random_bot_color(rng, used):
    for hue in rng.sample(range(360), 360):
        rgb = colorsys.hls_to_rgb(hue / 360, 0.68, 0.72)
        color = '#%02x%02x%02x' % tuple(round(channel * 255) for channel in rgb)
        if color.lower() not in used:
            return color
    raise ValueError('Too many bots to assign distinct colors')


def load_bots(path):
    path = Path(path).resolve()
    entries = json.loads(path.read_text())
    if not isinstance(entries, list) or len(entries) < 2:
        raise ValueError('Bot manifest must contain at least two bots')
    names = set()
    rng = secrets.SystemRandom()
    used_icons = {bot['icon'] for bot in entries if isinstance(bot, dict) and bot.get('icon')}
    used_colors = {bot['color'].lower() for bot in entries
                   if isinstance(bot, dict) and isinstance(bot.get('color'), str)}
    for index, bot in enumerate(entries):
        if not isinstance(bot.get('name'), str) or not bot['name'] or bot['name'] in names:
            raise ValueError('Each bot needs a unique nonempty name')
        names.add(bot['name'])
        if 'icon' not in bot:
            choices = [icon for icon in BOT_ICONS if icon not in used_icons]
            bot['icon'] = rng.choice(choices or BOT_ICONS)
            used_icons.add(bot['icon'])
        if 'color' not in bot:
            bot['color'] = _random_bot_color(rng, used_colors)
            used_colors.add(bot['color'].lower())
        if not isinstance(bot['icon'], str) or not bot['icon'] or len(bot['icon']) > 32:
            raise ValueError('Each bot icon must be a short nonempty string')
        if not isinstance(bot['color'], str) or not re.fullmatch(r'#[0-9a-fA-F]{6}', bot['color']):
            raise ValueError('Each bot color must be a six-digit hex color such as #79bcff')
        command = bot.get('command')
        if not isinstance(command, list) or not command or not all(isinstance(x, str) for x in command):
            raise ValueError('command must be a nonempty array of strings')
        bot['cwd'] = str((path.parent / bot.get('cwd', '.')).resolve())
        if not Path(bot['cwd']).is_dir():
            raise ValueError('Bot folder does not exist: ' + bot['cwd'])
        bot['command'] = [sys.executable if x == '{python}' else x for x in command]
    return entries


def get_move(bot, state, timeout):
    # Files avoid unbounded pipe buffering. The launch directory contains all bot files.
    with tempfile.TemporaryFile() as inp, tempfile.TemporaryFile() as out, tempfile.TemporaryFile() as err:
        inp.write((json.dumps(state) + '\n').encode())
        inp.seek(0)
        proc = subprocess.Popen(bot['command'], cwd=bot['cwd'], stdin=inp,
                                stdout=out, stderr=err, start_new_session=True)
        deadline = time.monotonic() + timeout
        try:
            while proc.poll() is None:
                if time.monotonic() >= deadline:
                    raise ValueError('move timed out')
                if os.fstat(out.fileno()).st_size > OUTPUT_LIMIT or os.fstat(err.fileno()).st_size > OUTPUT_LIMIT:
                    raise ValueError('bot output exceeded 64 KiB per stream')
                time.sleep(0.01)
            if time.monotonic() > deadline:
                raise ValueError('clock expired')
            if proc.returncode != 0:
                raise ValueError('bot exited with status ' + str(proc.returncode))
            if os.fstat(out.fileno()).st_size > OUTPUT_LIMIT or os.fstat(err.fileno()).st_size > OUTPUT_LIMIT:
                raise ValueError('bot output exceeded 64 KiB per stream')
            out.seek(0)
            return json.loads(out.read(OUTPUT_LIMIT).decode())
        finally:
            # Clean up child processes as well, even if the main bot already exited.
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            proc.wait()


def play(bots, k, clock_seconds, game_id, on_progress=None):
    game = Game(k)
    clocks = [float(clock_seconds), float(clock_seconds)]
    frames = []
    players = [b['name'] for b in bots]

    def frame(move=None):
        state = game.state()
        state['clocks'] = list(clocks)
        state['player_info'] = [{'name': b['name'], 'icon': b['icon'], 'color': b['color']} for b in bots]
        if move is not None:
            state['move'] = move
        frames.append(state)
        return state

    frame()
    while game.winner is None:
        player = game.turn
        state = game.state()
        state.update(game_id=game_id, ply=len(frames), players=players,
                     clocks=list(clocks))
        active_since = time.time()
        if on_progress:
            on_progress({'players': players, 'frames': list(frames), 'clocks': list(clocks),
                         'active_player': player, 'active_since': active_since,
                         'game_id': game_id, 'player_info': [{'name': b['name'], 'icon': b['icon'], 'color': b['color']} for b in bots]})
        move = None
        started = time.monotonic()
        try:
            if clocks[player] <= 0:
                raise ValueError('clock expired')
            move = get_move(bots[player], state, clocks[player])
            elapsed = time.monotonic() - started
            clocks[player] = max(0.0, clocks[player] - elapsed)
            game.apply(move)
        except (OSError, ValueError, IllegalMove) as exc:
            elapsed = time.monotonic() - started
            clocks[player] = max(0.0, clocks[player] - elapsed)
            if 'clock expired' in str(exc) or clocks[player] <= 0:
                clocks[player] = 0.0
                game.forfeit('time')
            else:
                game.forfeit(str(exc))
        current = frame(move)
        if on_progress:
            on_progress({'players': players, 'frames': list(frames), 'clocks': list(clocks),
                         'active_player': None, 'active_since': None,
                         'game_id': game_id, 'player_info': [{'name': b['name'], 'icon': b['icon'], 'color': b['color']} for b in bots]})
    return {'players': players, 'winner': bots[game.winner]['name'],
            'reason': game.reason, 'clock_seconds': clock_seconds, 'game_id': game_id,
            'player_info': [{'name': b['name'], 'icon': b['icon'], 'color': b['color']} for b in bots],
            'frames': frames}


def tournament(bots, k=15, clock_seconds=120, on_progress=None, pairing=None,
               game_id_start=1, on_game_complete=None):
    if clock_seconds <= 0:
        raise ValueError('clock_seconds must be positive')
    Game(k)
    if pairing is None:
        chosen_bots = bots
        pairs = list(combinations(bots, 2))
    else:
        if len(pairing) != 2 or pairing[0] == pairing[1]:
            raise ValueError('Choose two distinct bots')
        by_name = {b['name']: b for b in bots}
        if any(name not in by_name for name in pairing):
            raise ValueError('Selected bot was not found')
        chosen_bots = [by_name[name] for name in pairing]
        pairs = [(chosen_bots[0], chosen_bots[1])]
    games = []
    scores = {b['name']: 0 for b in chosen_bots}
    for pair_index, (a, b) in enumerate(pairs):
        pair_id = str(game_id_start + len(games))
        for round_number, ordered in enumerate(([a, b], [b, a]), start=1):
            result = play(ordered, k, clock_seconds, str(game_id_start + len(games)), on_progress)
            result.update(pairing_id=pair_id, round_number=round_number,
                          pairing_number=pair_index + 1, pairing_count=len(pairs),
                          pairing_bots=[a['name'], b['name']])
            games.append(result)
            scores[result['winner']] += 1
            if on_game_complete:
                on_game_complete(result, pair_index, round_number, len(pairs))
    return {'k': k, 'clock_seconds': clock_seconds,
            'bots': [{'name': b['name'], 'icon': b['icon'], 'color': b['color']} for b in chosen_bots],
            'scores': scores, 'games': games}


def main():
    parser = argparse.ArgumentParser(description='No Tipping tournament runner')
    parser.add_argument('--bots', default='bots.json')
    parser.add_argument('--k', type=int, default=15)
    parser.add_argument('--clock', type=float, default=120, help='total seconds per player per game')
    parser.add_argument('--output', default='results.json')
    parser.add_argument('--serve', action='store_true')
    parser.add_argument('--port', type=int, default=8000)
    args = parser.parse_args()
    bots = load_bots(args.bots)
    if args.serve:
        from .server import serve
        serve(bots, args)
    else:
        result = tournament(bots, args.k, args.clock)
        Path(args.output).write_text(json.dumps(result, indent=2))
        print(json.dumps(result['scores'], indent=2))

if __name__ == '__main__':
    main()
