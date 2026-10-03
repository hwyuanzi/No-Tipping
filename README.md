# No Tipping

This is the infrastructure for Heuristic Problem Solving game: No Tipping (as detailed [here](https://cs.nyu.edu/courses/fall26/CSCI-GA.2965-001/notipping.html)).

Runs on macOS and Linux with Python 3.8+. The runner uses only the Python standard library. The site has been tested locally and on NYU's crunchy5 server.

## System Overview: Getting started

C and C++ demos build themselves on first use when `g++` and `gcc` are installed. To keep compilation outside a bot’s game clock, prebuild them before starting a tournament. Julia must be installed and available as `julia` to run that example.

```sh
(cd bots/samples/cpp && ./build)
(cd bots/samples/c && ./build)
python3 -m notipping --serve
```

Open [http://127.0.0.1:8000](http://127.0.0.1:8000) to view the game in browser. 

The default `bots.json` includes two random Python bots and sample bots written in Python, C++, C, and Julia. This file maintains the tournament roster and is what will be adjusted to include your bot profiles.

### Playing a tournament

- Set **Weights per player (k)** to any positive integer less than or equal to 24 and **Clock per player** (default: 120 seconds). The interface will display a red warning when `2k < 50` is not satisfied, but it does not strictly block you from continuing. 
- There are two play modes: (1) **All bot pairings** plays every unique bot-versus-bot matchup (2) **Choose two bots** lets you choose a pair of bots to play 2 rounds against. 
- You also have the option to customize the gameplay speed. During the tournament, we will run "As Is", which will be the real-time gameplay. The moves will be made as soon as your bot returns them. All other speed options there are more so for demo purposes and as a resource if you want to watch the gameplay as you build out your strategies. 
- Click **Start tournament** (or **Add games to tournament**, once a tournament has begun) to start a game between two bots. Each game will automatically play two rounds, switching who goes first. 
- Bots are started before gameplay and remain running between moves. Each player’s clock measures the time spent waiting for that bot to produce a move. 
- During a game, players will kick things off in the "Placement" phase, where they take turns placing blocks on any valid spot on the board. Once all blocks are placed, players will then enter "Removal" phase, where they take turns removing <u>any</u> block from the board, including the original block initially placed at index -4. 
- After every move, the torque for the board will be recalculated. The equation is detailed in the "Rules and limits" section below. Thsi is how the tipping will be determined.
- After Game 1 finishes, click **Ready for round 2 — roles are switched**. After Game 2, the popup reports the pairing winner or a tie.
- Once a game has started, settings cannot be changed until you finish both rounds or you select **New tournament / Reset scores**.
- Wins accumulate across every game added to the tournament. Use **New tournament / Reset scores** to start over.
- Click **Stop tournament** to quit a run early. Any completed games and scores stay visible; click **New tournament / Reset scores** afterward to choose new settings and start again.

The selected `k` and clock are fixed once a tournament starts. Each player's clock runs only while their bot is responding and carries across all of that player's turns in one game. Results save after each game and restore when the server restarts with the same roster.

Select a game to replay it. Use Play, Back, Next, or the slider; expand **Move-by-move log** to browse moves from every completed game in the tournament and jump to a move. A tipping move identifies the player who tipped the board and animates the scale.

Below are 3 screenshots showing the UI. The first shows the browser during the "Placement" phase. The second shows the board during the "Removal" phase. Notice the different block labels. The third shows the board once it has been tipped. 

![Gameplay during the "Placement" Phase](docs/screenshots/placement-phase.png "This is what it looks like during the "Placement" phase. The blocks will be split into **Not Yet Placed** and **On Board**.")

![Gameplay during the "Removal" Phase](docs/screenshots/removal-phase.png ""This is what it looks like during the "Removal" phase. The blocks will be split into **On Board** and **Removed**.")

![Game ends when the board is tipped](docs/screenshots/tipped-board.png ""This is what it looks like when the board is tipped and the game ends. The winner will be announced with confetti (and maybe kit kats).")

### Run on crunchy5

Clone this repository to crunchy5. Be sure to build C/C++ bots on crunchy5 because local macOS binaries will not run on Linux. Also ensure that all the necessary runtime and dependencies are installed there.

Start the website on crunchy5:

```sh
python3 -m notipping --serve --port 8000
```

On your laptop, run this command in a second terminal and leave it running while you use the site:

```sh
ssh -J YOUR_USERNAME@access.cims.nyu.edu \
  -L 8000:localhost:8000 \
  YOUR_USERNAME@crunchy5.cims.nyu.edu
```

With the SSH connection open, visit **http://localhost:8000** in your laptop's browser. The server listens only on crunchy5's localhost; use `--port` and the matching forwarded port if 8000 is taken.

## Instructions for my peers: Making a bot

### 1. Create a folder

Put your strategy in a folder under `bots/`, for example `bots/shela-bot/`. Copy the template folder for your language and edit only its student-owned strategy file: `strategy.py`, `strategy.cpp`, `strategy.c`, or `strategy.jl`. Include any data or resources it needs. Please email myself (Shela) with your preferred bot name, preferred emoji and color (as a 6-digit hexadecimal), language/runtime version, and any setup notes.

### 2. Implement `choose_move(state)`

The wrapper will start your strategy, read one complete JSON state per line, calls `choose_move(state)`, and writes one JSON move per line. Do not implement stdin/stdout handling or print directly to stdout. Send debugging output to stderr. **In-memory variables persist during one game**; a new process starts for the next game. Please let me know if this needs to be changed.

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

Return a placement such as `{"position":-3,"weight":2}` or a removal such as `{"position":-4}`. The game engine is the authority: invalid moves are rejected, and tipping, timeout, invalid JSON, excess output, launch failure, and nonzero exit are forfeits.

### 3. Build your bot profile

I (Shela) will add your bot profiles to `bots.json`. For example:

```json
{"name":"Shela's Bot","icon":"🚀","color":"#79bcff","cwd":"bots/shela-bot","command":["{python}","main.py"]}
```

As mentioned, the `icon` and `color` fields are optional. Feel free to send me your preferred emoji and a six-digit hex color (such as `#79bcff` - you can use [color-hex.com](https://www.color-hex.com/)). I will add those preferences to this manifest entry. If either is omitted, the runner assigns a random default. It'd be best to keep your preferences distinct so bots are easy to tell apart. 

The `cwd` path is relative to `bots.json`. 

Please let me know if you have any custom dependencies ahead of time.

### 4. Testing (Optional)

There is a `tests/` folder at your disposal with automated tests for the game engine, tournament runner, server validation, and sample bots.

The test suite is organized into three files:

- `tests/test_game.py` tests game rules, bot execution, and tournament behavior.
- `tests/test_server.py` tests validation of server and game settings.
- `tests/test_samples.py` tests the sample bots, bot communication protocols, and bot identity configuration.

Some tests are skipped automatically when optional tools, such as C++, CMake, or Julia, are not installed.


To tun the full test suite from the repository root:

```bash
python -m unittest discover -s tests -v
```

### 5. Hand-Off

Please email me your complete bot folder by **Wednesday, October 7th**, with a preferred bot name, runtime version, build/setup commands, and optional emoji/color preferences. Include a short README if setup takes more than one step.

The bundled random bot and all four language samples use the same wrapper architecture. Working language demos are in `bots/samples/`; starter templates are in `bots/templates/`. C and C++ demos auto-build on first launch, though I will prebuild them before a tournament. Julia requires Julia to be installed (which I've done both locally and on crunchy5).

## Rules and limits

- Each player has one weight of every size from 1 to `k`. The course requires `2k < 50` (so `k` must be at most 24). The interface accepts any positive integer `k`; if `k` is 25 or greater, it displays a red warning that the course requirement is not met, but still lets you continue. The board has 60 open positions, so values above `k=30` cannot fit all players' weights during placement.
- The board is at position 0 and weighs 3 kg. Supports are at -3 and -1. The initial 3 kg block is at -4. Each position can hold at most one block; both support positions can hold weights.
- Players alternate placing weights until both players have placed all of theirs. Only then does player 0 start removing blocks. Either player may remove any placed block, including the opponent's or the initial block. The board itself cannot be removed. In other words, you cannot remove any block while placement is still underway.
- Torque is calculated independently about each support. For a support at position `s`, the game uses:

  ```text
  τ(s) = -3 × (0 − s) − Σ [ wᵢ × (pᵢ − s) ]
  ```

  Here, `s` is the support position (`-3` or `-1`), the `3` is the board's weight at position `0`, `pᵢ` is a block's position, and `wᵢ` is that block's weight. The game omits the shared gravitational acceleration factor because it would multiply every term equally. With the game's clockwise-negative sign convention:

  ```text
  τ_left  = τ(-3) ≤ 0
  τ_right = τ(-1) ≥ 0
  ```

  Both inequalities must hold for the board to be stable; zero torque is stable. A move that makes either inequality false tips the board and loses immediately. This is the one-dimensional lever-arm form of `τ = r × F`; [OpenStax University Physics explains torque and the lever arm](https://openstax.org/books/university-physics-volume-1/pages/10-6-torque).

  For the initial board and block, the values are:

  ```text
  τ(-3) = -3 × (0 − (-3)) − 3 × (-4 − (-3)) = -9 + 3 = -6
  τ(-1) = -3 × (0 − (-1)) − 3 × (-4 − (-1)) = -3 + 9 =  6
  ```

  Therefore the initial position is stable because `-6 ≤ 0` and `6 ≥ 0`.
- Each player gets 120 seconds per game by default. All bot processes start before the first turn, so bot startup does not count against either player's clock. Strategy execution and move-response time do count against the active player's clock. The browser and command line can change the clock.
- Standard output and standard error are each limited to **65,536 bytes per move**. This is an output limit, not a source-file limit. Invalid JSON or moves, launch failures, nonzero exits, excess output, and timeout forfeit the game.
- There is no application-enforced CPU or memory quota beyond the clock. The process stays alive for the full game, so in-memory variables persist between that bot’s turns; it is stopped when the game ends.

## Run without the website

```sh
python3 -m notipping --bots bots.json --k 24 --clock 120 --output results.json
```

Each unordered bot pair plays twice, swapping who goes first. A win earns one point; the two-game pairing may end tied. The output JSON saves every game. Starting a new browser tournament resets the previous scores; completed results are saved as each game finishes.

## Tests

```sh
python3 -m unittest discover -s tests -v
```

Note: The Julia sample test is skipped if Julia is not installed.
