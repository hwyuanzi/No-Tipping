# No Tipping

Tournament runner and replay website for the No Tipping game (as detailed here: https://cs.nyu.edu/courses/fall26/CSCI-GA.2965-001/notipping.html).

Runs on macOS and Linux with Python 3.8+. The runner uses only the Python standard library. The site has been tested locally and on NYU's crunchy5 server.

## Getting started

C and C++ demos build themselves on first use when `g++` and `gcc` are installed. To keep compilation outside a bot’s game clock, prebuild them before starting a tournament. Julia must be installed and available as `julia` to run that example.

```sh
(cd bots/samples/cpp && ./build)
(cd bots/samples/c && ./build)
python3 -m notipping --serve
```

Open [http://127.0.0.1:8000](http://127.0.0.1:8000). The default `bots.json` includes two random Python bots and sample bots written in Python, C++, C, and Julia. Edit that file to change who appears in the tournament. The sample source folders can stay in place when you replace the roster.

### Play a tournament

- Set **Weights per player (k)** to any positive integer and **Clock per player** (default: 120 seconds). The interface accepts values that break the course requirement and displays a red warning when `2k < 50` is not satisfied; it does not prevent you from continuing.
- Choose **All bot pairings** to play every unique bot-versus-bot matchup, or **Choose two bots** to play only the selected pair. Click **Run tournament**; each pairing plays twice, switching who goes first.
- After Game 1, click **Ready for round 2 — roles are switched**. After Game 2, the popup reports the pairing winner or a tie.
- Use **Live move pace** to pause briefly after each move so you can follow the board and live move list. This viewing pause does not use either bot’s clock; **Fast** runs without a pause.
- Wins accumulate across every game added to the tournament. Use **New tournament / Reset scores** to start over.
- Click **Stop tournament** to quit a run early. Any completed games and scores stay visible; click **New tournament / Reset scores** afterward to choose new settings and start again.

The selected `k` and clock are fixed once a tournament starts. Each player's clock runs only while their bot is responding and carries across all of that player's turns in one game. Results save after each game and restore when the server restarts with the same roster.

Select a game to replay it. Use Play, Back, Next, or the slider; expand **Move-by-move log** to browse moves from every completed game in the tournament and jump to a move. A tipping move identifies the player who tipped the board and animates the scale.

### Screenshots

These screenshots use a one-weight demo matchup. The first shows the stable starting board during replay; the second shows the board after a move tips it.

![Stable No Tipping board in replay](docs/screenshots/stable-board.png)

![No Tipping board after it tips](docs/screenshots/tipped-board.png)

### Run on crunchy5

Copy the project and bot folders to crunchy5. Build C/C++ bots on crunchy5 because local macOS binaries will not run on Linux. Make sure every bot's runtime and dependencies are installed there.

Start the website on crunchy5:

```sh
python3 -m notipping --serve
```

On your laptop, run this command in a second terminal and leave it running while you use the site:

```sh
ssh -J YOUR_USERNAME@access.cims.nyu.edu \
  -L 8000:localhost:8000 \
  YOUR_USERNAME@crunchy5.cims.nyu.edu
```

With the SSH connection open, visit **http://localhost:8000** in your laptop's browser. The server listens only on crunchy5's localhost; use `--port` and the matching forwarded port if 8000 is taken.

## Make a bot

### 1. Create a folder

Put your strategy in a folder under `bots/`, for example `bots/shela-bot/`. Copy the template folder for your language and edit only its student-owned strategy file: `strategy.py`, `strategy.cpp`, `strategy.c`, or `strategy.jl`. Include any data or resources it needs. Tell the organizer your bot name, optional icon/color, language/runtime version, and any setup notes.

### 2. Implement `choose_move(state)`

The organizer-owned wrapper starts your strategy, reads one complete JSON state per line, calls `choose_move(state)`, and writes one JSON move per line. Do not implement stdin/stdout handling or print directly to stdout. Send debugging output to stderr. **In-memory variables persist during one game**; a new process starts for the next game.

During placement (`phase` is `add`), return an available weight and position:

```json
{"position":-3,"weight":2}
```

During removal (`phase` is `remove`), return an occupied position:

```json
{"position":-4}
```

The state includes `protocol_version`, `k`, `phase`, `player`, `board`, `remaining`, `torques`, `clocks`, `winner`, `reason`, and, when available, `game_id`, `ply`, and `players`. Positions range from -30 through 30. `player` indexes `players`; `remaining` and `clocks` use the same order. Each board item has `position`, `weight`, and `owner` (`null` for the initial block).

Example input on the first turn with `k=2`:

```json
{"protocol_version":1,"k":2,"phase":"add","player":0,"board":[{"position":-4,"weight":3,"owner":null}],"remaining":[[1,2],[1,2]],"torques":{"left":-6,"right":6},"clocks":[120,120],"winner":null,"reason":null,"game_id":"1","ply":1,"players":["Alice","Bob"]}
```

Return a placement such as `{"position":-3,"weight":2}` or a removal such as `{"position":-4}`. The game engine remains authoritative: invalid moves are rejected, and tipping, timeout, invalid JSON, excess output, launch failure, and nonzero exit are forfeits. Python, C++, C, and Julia now all have organizer-owned wrappers and native strategy state/move types. The C/C++/Julia adapters use dependency-free protocol parsers, so students do not write JSON or process-management code.

### 3. Add it to the roster

The organizer adds one entry to `bots.json`; students do not edit the roster or provide an execution command. The `cwd` path is relative to `bots.json`. For example:

```json
{"name":"Shela's Bot","icon":"🚀","color":"#79bcff","cwd":"bots/shela-bot","command":["{python}","main.py"]}
```

The `icon` and `color` fields are optional. Students can send you their preferred emoji and a six-digit hex color (such as `#79bcff`) with their bot; add those preferences to the manifest entry. If either is omitted, the runner assigns a random default. Keep preferences distinct so bots are easy to tell apart. Students do not need to edit `bots.json` themselves.

For compiled or other-language bots, the organizer sets the build and execution command. Avoid requiring internet access or unapproved dependencies.

### 4. Test and hand it off

Test your bot on both an `add` state and a `remove` state. Confirm it prints one valid move and no other text to standard output. Send the complete bot folder by **Wednesday, October 7th**, with its name, runtime version, build/setup commands, and optional emoji/color preferences. Include a short README if setup takes more than one step.

The bundled random bot and all four language samples use the same wrapper architecture. Working language demos are in `bots/samples/`; starter templates are in `bots/templates/`. C and C++ demos auto-build on first launch, but prebuild them before a tournament. Julia requires Julia to be installed on the machine running the tournament.

## Rules and limits

- Each player has one weight of every size from 1 to `k`. The course requires `2k < 50` (so `k` must be at most 24). The interface accepts any positive integer `k`; if `k` is 25 or greater, it displays a red warning that the course requirement is not met, but still lets you continue. The board has 60 open positions, so values above `k=30` cannot fit all players' weights during placement.
- The board is at position 0 and weighs 3 kg. Supports are at -3 and -1. The initial 3 kg block is at -4. Each position can hold at most one block; both support positions can hold weights.
- Players alternate placing weights until both players have placed all of theirs. Only then does player 0 start removing blocks. Either player may remove any placed block, including the opponent's or the initial block. The board itself cannot be removed. In other words, you cannot remove any block while placement is still underway.
- Torque about support `s` is `-3*(0-s) - sum(weight*(position-s))`. This is the one-dimensional lever-arm form of the standard torque equation, `τ = r × F`, summed over the board and blocks; the game omits the shared gravitational acceleration factor and uses clockwise-negative signs. [OpenStax University Physics explains torque and the lever arm](https://openstax.org/books/university-physics-volume-1/pages/10-6-torque). Stability requires left torque `<= 0` and right torque `>= 0`; zero is stable. A move that tips the board loses immediately.
- Each player gets 120 seconds per game by default. Bot startup and strategy execution count against that player's clock. The browser and command line can change the clock.
- Standard output and standard error are each limited to **65,536 bytes per move**. This is an output limit, not a source-file limit. Invalid JSON or moves, launch failures, nonzero exits, excess output, and timeout forfeit the game.
- There is no application-enforced CPU or memory quota beyond the clock. Bots run with the organizer account's filesystem and network permissions; this is not a security sandbox. Submit code you trust, and keep file access within your bot folder. The process stays alive for the full game, so in-memory variables persist between that bot’s turns; it is stopped when the game ends.

## Run without the website

```sh
python3 -m notipping --bots bots.json --k 15 --clock 120 --output results.json
```

Each unordered bot pair plays twice, swapping who goes first. A win earns one point; the two-game pairing may end tied. The output JSON saves every game. Starting a new browser tournament resets the previous scores; completed results are saved as each game finishes.

## Tests

```sh
python3 -m unittest discover -s tests -v
```

The Julia sample test is skipped if Julia is not installed.
