# No Tipping

A Python standard-library tournament runner and browser replay interface for macOS and Linux, including NYU crunchy5. Requires Python 3.8+; no pip packages or Node installation required. The project has been tested locally; crunchy5 access and installed runtimes have not been verified.

## Run locally

From this folder:

```sh
python3 -m notipping --serve
```

Open http://127.0.0.1:8000 and set **Weights per player (k)** to any integer from **1 to 24**, then click **Run tournament**. You can choose a different k before each tournament without restarting the server. The value is locked while a tournament runs and applies to both games of every pairing. `--k` sets the initial browser default; each saved result records the k used. The default clock is **120 seconds per player per game**, shared across all placement and removal turns. Only the player whose bot is responding loses time. If that player’s remaining time reaches zero, they lose on time. Set **Clock per player** before starting a tournament to change the limit. The active game board and both player clocks appear during the tournament; the running player’s clock updates continuously while their bot thinks. Choose **All bot pairings** for a full round robin, or **Choose two bots** to add one matchup. Either choice adds games to the same active tournament and standings; every pair plays twice, swapping who goes first. The first game result appears in a popup; click **Ready for round 2 — roles are switched** to start the swapped game. After Game 2, a result popup reports the matchup winner or tie and offers the next matchup when applicable. Each game and the cumulative standings save as it finishes. Use **New tournament / Reset scores** to clear the current results and begin again. After the first game finishes, k and clock settings stay fixed for that tournament; starting a new one unlocks them. The replay selector and controls are available as games finish. When a move tips the board, the replay names the player who tipped it and animates the scale around the support. Tournament standings count wins across every game in the run and appear beside the replay on wide screens. Expand **Move-by-move log** to see each placement/removal with player, position, weight, and resulting torque; select a move to jump the replay to that state. Use Play, Next, Back, or the slider to inspect the board, inventories, clocks, and result.

For a terminal-only competition:

```sh
python3 -m notipping --bots bots.json --k 15 --clock 120 --output results.json
```

Each unordered pair plays exactly twice, with first player swapped. A win earns one point; equal win counts remain tied. Every game is saved in the output JSON. In the browser, the result file is updated as each game completes and a matching saved tournament is restored when the server restarts. Starting a new tournament clears the previous output file and begins a fresh tally. A terminal-only invocation writes its tournament result to the specified output path.

## Run on crunchy5

Copy the project and bot folders to your account, then run the same command there. Build compiled bots on the target machine: macOS executables generally cannot run on Linux. Each bot's runtime and dependencies must be available on the machine where it executes.

To use the browser UI remotely, run `python3 -m notipping --serve` on crunchy5, and in a separate terminal on your laptop forward its port:

```sh
ssh -L 8000:127.0.0.1:8000 YOUR_USERNAME@YOUR_CRUNCHY5_HOSTNAME
```

Then visit http://127.0.0.1:8000 locally. Substitute the SSH hostname and any gateway options supplied by NYU. The server binds only to loopback. Use `--port` and matching forwarding ports if 8000 is occupied.

## Getting started: classmates

### 1. Prepare your bot folder

Gather the complete project: source files, required data/resources, and any dependency or build instructions. **There is no submission file-size limit, file-count limit, or single-file requirement in this runner.** Transfer-service quotas, disk capacity, and university policies still apply separately. Include the language/runtime version and the exact build and launch commands. Native C/C++ programs must be built on the machine where the competition runs.

The organizer needs your bot's display name and launch details. The interface assigns each bot a random emoji and color automatically, so there is nothing to choose or configure for its visual identity.

### 2. Read the game state and return one move

The runner starts a fresh process for each move. Read one JSON object from standard input and print exactly one JSON move object to standard output, then exit. Write diagnostics to standard error. During placement, return a position and an available weight, for example `{"position":-3,"weight":2}`. During removal, return an occupied position, for example `{"position":-4}`. Positions range from −30 to 30. The state tells you the phase, current player, board, remaining weights, torque, clocks, and game ID. Your program must not rely on in-memory state surviving between turns.

Both games in a matchup count toward the same tournament standings, and the second game swaps who goes first. The state’s `player` value and `players` order identify each game's turn order; do not assume your bot is always player 0.

### 3. Test your bot

Build and launch your bot on a sample placement state and a sample removal state. Check that it prints one valid JSON move and nothing else to stdout. The examples below and `bots/samples/` show the protocol.

The runner is language-agnostic: any command-line program that can read the protocol JSON from stdin and write one move JSON object to stdout can be used. This includes C, C++, Julia, Python, Java, Rust, and other installed runtimes. Commands are configured as an argument array and launched directly without a shell. `cwd` is the bot folder, resolved relative to the manifest; use portable paths. `{python}` resolves to the runner's Python interpreter.

The organizer will add your bot to `bots.json` using a manifest entry like this:

```json
{"name":"Alice's Bot", "cwd":"bots/alice", "command":["{python}", "main.py"]}
```

The organizer runs any build/setup steps separately before the competition; the runner does not build native code or install packages automatically. Avoid requiring internet access. Submissions run with the organizer account's filesystem and network permissions, so do not read or write outside your bot folder.

### 4. Hand off your submission

Send me the full bot folder by Wednesday, along with your bot name, runtime version, and any build or setup commands. You do not need to pick an icon or color; I’ll let the system assign those randomly. Include a short README if the project needs more than one setup step.

### Sample bots

Working protocol examples are in `bots/samples/`, and `sample-bots.json` lists one bot per class language. The Python and Julia examples need no extra packages. C and C++ examples need to be compiled before use; build them on the machine where the competition will run so the executable matches that host:

```sh
(cd bots/samples/cpp && g++ -std=c++17 -O2 bot.cpp -o bot)
(cd bots/samples/c && gcc -std=c11 -O2 bot.c -o bot)
python3 -m notipping --bots sample-bots.json --serve
```

Make sure `julia` is installed and on `PATH` to include the Julia example. The runner starts a fresh process for each move, so language startup and package initialization count against that bot's clock. Compile native bots and precompile any packages before game day. The Julia runtime is not installed in this development environment, so its example is included in the test suite but skipped here; it uses only Julia's built-in regex and I/O support.

**A new process launches for each move**, with the bot folder as its working directory. Read one JSON object from stdin, print exactly one JSON move to stdout, then exit. Send diagnostic messages to stderr. Imports and startup count against the active player’s remaining clock. Use game state as your source of truth; in-memory variables do not persist across turns. If storing files, namespace them by game and player, and clear stale data between tournament runs (game IDs restart at 1 each run).

Input example (first turn with k=2):

```json
{"protocol_version":1,"k":2,"phase":"add","player":0,"board":[{"position":-4,"weight":3,"owner":null}],"remaining":[[1,2],[1,2]],"torques":{"left":-6,"right":6},"clocks":[120,120],"winner":null,"reason":null,"game_id":"1","ply":1,"players":["Alice","Bob"]}
```

Player 0 always goes first in the current game. Player 1 goes second. Ownership uses these indexes; `null` denotes the initial block. `remaining` contains the unused weights of each player. During `add`, respond with:

```json
{"position":-3,"weight":2}
```

The `clocks` array reports the seconds remaining for player 0 and player 1 at the start of your turn.

During `remove`, respond with:

```json
{"position":-4}
```

The bundled `bots/random/bot.py` imports `strategy.py`, demonstrating a multi-file submission. It chooses a random remaining weight and its leftmost safe position during placement, and a random safe removal during removal. If the selected weight has no safe placement, it makes a tipping placement; if no safe removal exists, it makes a tipping removal.

## Rules and limits

Course rules come from the supplied No Tipping PDF. The organizer additionally requires two games per pairing. Standings accumulate across all game batches added to the same tournament, including both all-pairings and selected-matchup runs. Starting a new tournament resets the scores. The following operational defaults are implementation choices and can be reviewed before distributing the contract:

- `--k 15` by default, configurable from 1 through 24 (the course requires 2k < 50).
- `--clock 120`: 120 seconds on each player’s game clock, configurable in the browser or CLI. Time carries across every turn in that game and pauses while the opponent acts.
- Stdout and stderr may each contain at most **65,536 bytes per move**. This is an output limit, not a source-file or submission-size limit. Output is checked approximately every 10 ms, so a process can briefly exceed the limit before termination.
- Invalid JSON, invalid moves, failed launches, nonzero exits, excess output, and running out of clock time forfeit the game. The opponent receives one win; the tournament continues.
- No application-enforced memory or CPU quota beyond the player clock. The UI refreshes the active clock while a tournament is running. Bots execute with the organizer account's filesystem/network permissions; this runner is not a security sandbox. Use an appropriately restricted account or isolated environment for code you do not trust. Process groups are terminated at the end of every move on macOS/Linux.

The board spans integer positions -30 through 30 inclusive and weighs 3 kg at center 0. Supports are -3 and -1; both positions accept weights. The initial 3 kg block occupies -4. Only one block may occupy a position. Each player owns one of each weight 1 through k.

Placement alternates until both inventories are empty, then player 0 begins removal. Either player can remove any placed block, including the opponent's or the initial block. The board itself cannot be removed. No voluntary early removal is allowed.

Torque about support s is `-3*(0-s) - sum(weight*(position-s))`, using clockwise-negative convention and omitting the common gravitational factor. Stability requires left torque <= 0 and right torque >= 0. Zero torque is allowed. A tipping move loses immediately. The bare board is unstable, so a game cannot end in an empty-board draw.

## Verify

```sh
python3 -m unittest discover -s tests -v
```

Tests cover torque, support placements, occupancy, phase transition, removal ownership, initial-block removal, tipping, swapped starts, multi-file bots, and failed/slow/noisy bots.
