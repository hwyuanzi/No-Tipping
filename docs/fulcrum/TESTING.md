# Validation and measured performance

Final strategy SHA-256: `9d9feab4aaf06a630dc65a4ab316692d5d5161f1271d5cffa4ec859e66acba5e`.

## Correctness and maintenance checks

These checks were rerun during the October 3, 2026 maintenance pass. [maintenance_validation.json](../../benchmarks/results/2026-10-03/maintenance_validation.json) records the source hash, environment, commands, and outcomes. Both complete-game wrapper smoke checks (`k=4`, two-second clock, Sample Python, swapped seats) ended normally and were won by Fulcrum. The updated in-process arena also completed two smoke games without runtime failures, and regenerating the historical JSON summary reproduced the retained results exactly.

- Final full repository suite: **27/27 passed**, including the two added submission/protocol tests.
- Independent C++ oracle: **87,334 inventory legality checks**, **56,490 stable subset outcome checks**, and **80 actual removal decisions** passed. Winning positions must return a child independently classified as losing.
- Final C++ oracle also passed with **AddressSanitizer and UndefinedBehaviorSanitizer** enabled.
- Official persistent subprocess protocol was tested with random reachable states for k=1,4,8,15,24, zero-torque boundaries, no-safe-move states, changing board universes, and low clock values. Moves must remain structurally valid and safe whenever a safe move exists.
- All organizer-owned files in the copied folder are byte-identical to the C++ template. The maintenance pass preserved the strategy source hash above and the original engine, runner, templates, samples, and tests. No ZIP packaging is used.
- During initial development, one original startup-marker test failed in a concurrent run and passed in both subsequent full runs. It also passed during this maintenance run. Its two subprocesses sleep before creating markers; the game can end and terminate the inactive process before its marker appears. The supplied test and runner were left unchanged.

## Default 120-second clock

Each matchup swapped the first player. These are observations from small samples, not a statistical guarantee against unknown classmates.

| k | Opponent | Wins | Maximum bot clock used |
|---|---|---|---|
| 15 | Inventory pressure + exact removal through 22 blocks | 2/2 | 62.32 s |
| 24 | Two-ply minimax placement + exact removal through 22 blocks | 2/2 | 94.92 s |
| 24 | Supplied Sample Python, official process runner | 2/2 | 98.28 s |

The first two are fast in-process tests. They charge all Fulcrum strategy wall time but omit protocol parsing overhead. The third uses the organizer's actual `play` function, persistent stdin/stdout, JSON wrapper, and cumulative response clock. All finished by tipping, with no timeout or protocol forfeits. The reference opponents use locally authored policies; they are not other students' competition submissions.

## Five-second stress clock

64 in-process games: k=4,8,15,24, four opponent families, two predetermined seeds, both seats. Fulcrum won **59/64**. Maximum accumulated strategy time: **4.451/5 seconds**.

| Opponent | Wins |
|---|---|
| uniform | 16/16 |
| course_random | 15/16 |
| pressure_exact22 | 15/16 |
| minimax_exact22 | 13/16 |

`uniform` samples all legal actions; `course_random` chooses a random weight and its leftmost safe position; `pressure_exact22` prioritizes inventory mobility pressure and uses exact removal with a 350,000-node cap; `minimax_exact22` examines 24 root placement candidates against all legal one-ply replies and uses that same exact removal solver. Reference computation is not charged to a clock in this harness, so it is a demanding strategy test rather than a timing-equivalent tournament.

The exploratory earlier two-second run scored 55/64. It predates the final immediate-trap filtering change and is retained as development evidence, not pooled with final-version measurements.

## Official protocol at a shorter clock

Twenty official-runner games, 20 seconds per player, k=1,4,8,15,24, versus the supplied Sample Python and Random A, both seats: **17/20 wins**, **15/16 with k>1**. All ended normally by tipping. This includes a loss against Sample Python at k=24; it demonstrates that a smaller clock and a different trajectory can still produce a losing placement endgame. Random A is unseeded, so fresh runs may differ.

For k=1, a complete legal game-tree enumeration proves that player 0 loses against optimal play. The two k=1 first-player losses should not be treated as evidence that every first-player loss is avoidable.

## Results layout and provenance

The performance numbers above are historical measurements from the initial implementation on October 3, 2026; the documentation cleanup did not rerun the full performance matrix. Raw CSV records, complete protocol replays, the summary, exhaustive-verification output, and the `k=1` oracle output are retained under [benchmarks/results/2026-10-03/](../../benchmarks/results/2026-10-03/). `metadata.json` records the measured strategy hash and environment. `arena_initial.csv` is explicitly excluded from the final-version summary.

Fresh experiments belong in `benchmarks/local/`; remove generated scratch files before committing because the upstream `.gitignore` is unchanged. Promote only useful completed runs into a new dated results directory, retaining source hash, compiler/platform, commands, clock settings, and opponent descriptions. New protocol replays embed the strategy source hash; build immediately before measuring so the executable corresponds to that source. For the complete matrix, the report script reads embedded hashes or historical metadata, validates game counts and normal completion, and writes a JSON summary. It preserves historical provenance rather than substituting a later working-tree hash, and does not regenerate maintained documentation.

## Reproduce from repository root

Build and run the correctness checks:

```sh
(cd bots/fulcrum && ./build)
python3 -m unittest discover -s tests -v
mkdir -p benchmarks/local
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic tools/verify_fulcrum.cpp -o benchmarks/local/verify_fulcrum
benchmarks/local/verify_fulcrum
c++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tools/verify_fulcrum.cpp -o benchmarks/local/verify_fulcrum_san
benchmarks/local/verify_fulcrum_san
c++ -std=c++17 -O2 tools/solve_k1.cpp -o benchmarks/local/solve_k1
benchmarks/local/solve_k1
```

The Fulcrum unittest builds with the supplied script and starts the persistent wrapper. It checks safe moves against the independent Python engine on reachable states for `k=1,4,8,15,24`, including zero-torque boundaries, no-safe-move fallbacks, changing board configurations, and short clocks. The C++ oracle independently computes torques and exhaustively classifies small subsets, then checks returned winning moves.

A small official-runner smoke check exercises complete games in both seats:

```sh
python3 tools/benchmark_protocol.py --clock 2 --ks 4 --opponents 'Sample Python' --output benchmarks/local/protocol_smoke.json
```

To reproduce the full performance matrix, keep all output together and allow several minutes of cumulative bot thinking time:

```sh
c++ -std=c++17 -O2 tools/arena_fulcrum.cpp -o benchmarks/local/arena_fulcrum
benchmarks/local/arena_fulcrum 2 5 benchmarks/local/arena_final.csv
benchmarks/local/arena_fulcrum 1 120 benchmarks/local/arena_120_k15.csv 15 2
benchmarks/local/arena_fulcrum 1 120 benchmarks/local/arena_120_k24.csv 24 3
python3 tools/benchmark_protocol.py --clock 20 --ks 1 4 8 15 24 --output benchmarks/local/protocol_20.json
python3 tools/benchmark_protocol.py --clock 120 --ks 24 --opponents 'Sample Python' --output benchmarks/local/protocol_120.json
python3 tools/report_fulcrum.py --results-dir benchmarks/local
```

The arena arguments are seed count, per-player clock, output CSV, optional `k`, and optional opponent index (0: uniform; 1: course random; 2: pressure; 3: minimax). Its default output is `benchmarks/local/arena.csv`. Opponent computation is not clocked in this harness. The report script expects exactly the complete matrix above; it is not intended to summarize partial smoke checks. The standalone `k=1` solver provides a complete small-game result.

To validate and summarize the retained historical measurements without rerunning them:

```sh
python3 tools/report_fulcrum.py
```

This writes `benchmarks/local/benchmark_summary.json` and prints whether the current strategy matches the measured source. The historical source hash excludes the earlier exploratory run. All commands assume the repository root; the protocol script also resolves its relative output paths there. No machine-specific absolute roster file is retained.

Wall-clock cutoff decisions vary with CPU and concurrent load even with fixed pseudo-random seeds. Runtime was tested on arm64 macOS with Apple Clang 21.0.0 and Python 3.14.7 for the test harness. The production bot requires only C++17. A Linux/crunchy5 build remains to be run on that machine; rebuild there rather than copying a macOS executable.
