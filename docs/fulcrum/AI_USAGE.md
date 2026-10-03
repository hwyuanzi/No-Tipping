# AI assistance record

The [public course page](https://cs.nyu.edu/~shasha/papers/heuristicsindex.html) permits large language models and asks students to explain the prompt, result, and how it was used.

## Initial request

The original request was in Chinese. Its English translation is:

> Based on the course at https://cs.nyu.edu/courses/fall26/CSCI-GA.2965-001/, especially the No Tipping class, and this repository's contest requirements, create an algorithm and complete submission that maximize the chance of achieving the best score and beating every opponent. C++ may be used.

The user also supplied organizer instructions: copy the entire language template, preserve the folder structure, modify only `strategy.cpp`, retain the wrapper/header/build files, provide the entire copied folder, and include a preferred name/emoji/color, compiler version, setup instructions, and dependencies.

## Generated result and use

Codex read the repository README, authoritative game engine, protocol wrapper, and starter files, and consulted the professor's accessible public course and No Tipping pages. It generated the C++ strategy, independent exhaustive-verification harness, local benchmark opponents, persistent-protocol tests, documentation, and an initial submission package workflow.

The strategy combines exact torque intervals, inventory-aware placement evaluation, beam alpha-beta search, adversarial Monte Carlo tree search, and memoized exact impartial-game removal search. `bots/fulcrum/strategy.cpp` is the AI-assisted implementation; the organizer's wrapper, header, build script, and copied template README remain unchanged.

[ALGORITHM.md](ALGORITHM.md) describes mechanics, assumptions, limitations, and a presentation outline. [TESTING.md](TESTING.md) and the dated machine-readable results record validation and actual opponent/clock settings. Exact removal proofs are distinguished from search estimates. No hidden opponent data, paid inference service, network dependency, or external game API is used at runtime.

## Repository maintenance request

The follow-up request asked Codex to prepare Fulcrum for long-term GitHub maintenance: retain the C++17 logic and organizer protocol, make documentation English, organize it as a root README and three documents under `docs/`, remove unnecessary generated files, retain useful benchmark/test tools and evidence, verify execution and tests, and avoid ZIP creation or pushing to GitHub.

Codex reorganized and translated documentation, moved historical measurements into `benchmarks/results/2026-10-03/`, removed obsolete packaging outputs and tooling, and updated benchmark/report paths and provenance handling. The strategy source was preserved byte-for-byte. Build, protocol, move-validity, test-suite, and independent-oracle checks were rerun as described in [TESTING.md](TESTING.md).

## Upstream restoration request

The user subsequently requested undoing changes to the organizer repository. All existing organizer-owned files were restored to `upstream/main`, including the root README, `.gitignore`, and original screenshot metadata. Fulcrum documentation was moved into `docs/fulcrum/`, keeping its setup instructions separate from the original README. Only new Fulcrum source, profiles, documentation, tests, tools, and benchmark evidence remain. No commit or push was made.
