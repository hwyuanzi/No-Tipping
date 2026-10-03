# No Tipping — Fulcrum

Fulcrum is a C++17 contest bot built on the supplied No Tipping infrastructure. It combines inventory-aware placement search with memoized exact removal search and adversarial Monte Carlo tree search. The bot uses only the C++ standard library; the organizer's Python runner requires Python 3.8+.

## Fulcrum quick start

Build on the machine that will run the contest, before starting its game clock:

```sh
(cd bots/fulcrum && ./build)
python3 -m notipping --bots bots.fulcrum.json --serve
```

Open [http://127.0.0.1:8000](http://127.0.0.1:8000) and choose Fulcrum and an opponent. The full roster includes Julia, C, and C++ samples; install their runtimes or select the Python samples. For a small two-seat protocol check:

```sh
python3 tools/benchmark_protocol.py --clock 2 --ks 4 --opponents 'Sample Python'
```

To use GCC explicitly, run `(cd bots/fulcrum && CXX=g++ ./build)`. The organizer launches `./bot` with working directory `bots/fulcrum`; launching it does not compile it. Rebuild locally on Linux rather than copying a macOS executable. Fulcrum was tested on arm64 macOS with Apple Clang 21.0.0; Linux/crunchy5 execution remains unverified.

| Setting | Value |
| --- | --- |
| Bot name | Fulcrum |
| Emoji | ⚖️ |
| Block color | `#19A7CE` |
| Language | C++17 |
| Runtime dependencies | C++ standard library |
| Build dependencies | C++17 compiler and POSIX shell |
| Roster entry | [fulcrum.profile.json](../../fulcrum.profile.json) |
| Local roster | [bots.fulcrum.json](../../bots.fulcrum.json) |

Copy the entire `bots/fulcrum/` folder when handing the bot to the organizer. Its `runner.cpp`, `strategy.hpp`, `build`, and template `README.md` remain byte-identical to `bots/templates/cpp/`. Strategy changes belong in `strategy.cpp`. The wrapper owns JSON parsing, move serialization, stdout flushing, error reporting, and process lifecycle. No external API, network service, model, or data file is required at runtime.

## Documentation and maintenance

- [Algorithm](ALGORITHM.md): stability equations, search, time allocation, and limitations.
- [Testing](TESTING.md): checks, benchmark interpretation, and reproduction commands.
- [AI usage](AI_USAGE.md): prompt translation, generated work, and disclosure.

```text
docs/fulcrum/README.md      Fulcrum setup and metadata
bots/fulcrum/              Complete source bot: strategy + unchanged template files
bots/templates/            Organizer language templates
bots/samples/, bots/random/ Organizer reference bots
bots.json                  Original organizer roster
bots.fulcrum.json           Organizer roster plus Fulcrum
fulcrum.profile.json        Standalone portable roster entry
notipping/, web/           Original engine, runner, server, and browser UI
docs/fulcrum/             Fulcrum algorithm, testing, and AI usage
tests/                    Original tests plus Fulcrum protocol tests
tools/                    Verification, benchmark, and reporting scripts
benchmarks/results/2026-10-03/  Curated historical results and source provenance
benchmarks/local/         Local builds, logs, and replay outputs
```

From the repository root:

```sh
python3 -m unittest discover -s tests -v
```

Keep reviewed benchmark evidence in a dated directory under `benchmarks/results/`, with source provenance and documented settings. Local experiments default to `benchmarks/local/`. Remove generated executables and scratch outputs before committing; the original upstream `.gitignore` is preserved. Maintained documentation is edited directly; the report script generates JSON summaries without overwriting it. Historical results include 59/64 wins at a five-second clock and 6/6 across three two-seat matchups at 120 seconds. These measurements do not establish optimal play or guarantee wins against unknown opponents.

## Upstream preservation

All organizer-owned files match `upstream/main`, including the root README, `.gitignore`, engine, tests, templates, samples, infrastructure, and original documentation. Fulcrum documentation is kept separately in `docs/fulcrum/`; the new bot, tests, tools, profiles, and benchmark evidence are additions only.
