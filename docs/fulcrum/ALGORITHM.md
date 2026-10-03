# Fulcrum algorithm

Fulcrum aims to improve contest win probability within each player's cumulative response clock. It is a C++17 implementation for the course range `1 <= k <= 24`. Placement search is heuristic; completed removal proofs are exact. There is no proof of optimal placement play or universal superiority.

## Rules and sources

The repository's `notipping/game.py` is authoritative for this implementation:

- Positions run from -30 through 30. The board weighs 3 kg at position 0; supports are at -3 and -1; the initial block is 3 kg at -4.
- Players place only their own remaining weights. During removal, either player may remove any block, including the initial block; the board itself stays.
- Zero torque is stable. Once both inventories are empty, player 0 starts removal.
- When no safe action exists, the bot returns a structurally valid action that inevitably tips the board.

The professor's [public 2026 course page](https://cs.nyu.edu/~shasha/papers/heuristicsindex.html) provides the adversarial-search and experimental context. The [public No Tipping description](https://cs.nyu.edu/~shasha/papers/notipping.html) agrees with the main physical rules. The supplied [course URL](https://cs.nyu.edu/courses/fall26/CSCI-GA.2965-001/) and its No Tipping page redirected to NYU authentication when the bot was developed. Historical strategy and adversarial-search PDF links returned HTTP 403. Those inaccessible materials and unprovided oral lectures were not treated as sources that had been read.

## Stability and move generation

Let `A = -torque_left` and `B = torque_right`. Stability requires both `A >= 0` and `B >= 0`. Adding weight `w` at position `p` updates the margins as follows:

```text
A' = A + w(p + 3)
B' = B - w(p + 1)
```

Removal reverses those updates. A safe placement must satisfy the exact integer interval:

```text
ceil(-A / w) - 3 <= p <= floor(B / w) - 1
```

Intersect that interval with `[-30, 30]` and exclude occupied positions. Occupancy fits in a 61-bit subset of a 64-bit integer; inventories also use bit masks. Explicit mathematical floor and ceiling avoid C++ integer division's truncation toward zero for negative values.

## Placement search

The evaluator considers every remaining weight's safe-position count, mean logarithmic mobility, total action count, and the fraction of weights with no safe position. The same stability margins can be comfortable for a player with small weights and dangerous for one with only large weights. A mild preference for placing heavier weights reduces later inventory pressure; it is an ordering heuristic rather than a fixed opening.

The root enumerates all safe actions and detects immediate wins that leave the opponent unable to place safely. Candidate selection retains highly ranked actions and a representative for every usable weight.

A beam alpha-beta search examines one through four layers. It accepts a deeper iteration only after evaluating all root candidates in that iteration. If a one-layer opponent reply actually leaves Fulcrum without a safe placement, or an exact removal proof shows the resulting board loses, that root action is filtered when an alternative survives. Candidate pruning at deeper layers prevents those evaluations from being treated as general proofs.

Adversarial Monte Carlo tree search then compares candidates with UCT and progressive widening. Each node's outcome is measured from its own acting player's perspective, so opponent nodes choose replies that benefit the opponent. Rollouts mix heavy-weight preference, mobility pressure, and random variation to reduce dependence on one assumed opponent policy.

Simulations continue through both phases. The placement-to-removal transition explicitly sets the acting player to player 0. Small removal endings use an exact solver. On the final placement, the bot also tries to prove the removal outcome of candidate boards directly. A truncated placement candidate list cannot certify that every possible action loses.

## Exact removal search

After placement, block positions and weights stay fixed and both players have identical removal permissions. Within that fixed board universe, a state is determined by its remaining-block subset, without owner labels.

Let `W(S)` mean the player to act can force a win:

```text
No safe removal: W(S) = false.
Some safe child S without block i has W(child) = false: W(S) = true.
Every safe child has W(child) = true: W(S) = false.
```

The solver memoizes proved outcomes with exact subset keys and orders actions by the opponent's reply count. A found losing child proves a winning move immediately. A loss requires proving every safe child wins.

Only completed proofs enter the table. Time or node limits interrupt search rather than turn an unfinished search into a false loss. The table holds at most 1,500,000 entries. It persists across real removal turns in one game, but resets when the game identity or occupied positions' weight configuration changes. Simulated boards use separate tables.

When a large ending cannot be proved within its allocation, MCTS continues with the remaining budget and reuses proved states. If the current board is proved losing, the bot favors moves that give the opponent more opportunities to make a mistake. That preference improves practical chances without changing the exact loss classification.

## Time and complexity

A `steady_clock` deadline follows wall time, matching the organizer's response clock. Budgets depend on remaining time, placement actions, and anticipated removal turns. The bot reserves roughly 3.5% or at least 0.08 seconds, caps placement at 2 seconds per action and removal at 12 seconds, and spends more of a small ending's budget on proof search. Very short clocks fall back to an already generated safe action. Parsing and process scheduling still consume clock time.

Placement has at most `61 * k` actions before occupancy filtering, with additional inventory evaluation and bounded search. Complete removal search has exponential worst-case cost, roughly `O(n * 2^n)`. Caching, action ordering, early proofs, and deadlines reduce practical work; no fixed block-count threshold guarantees completion.

State-dependent pseudo-random seeds make simulated trajectories reproducible, but wall-clock cutoffs can change moves across machines or under different system loads. Full enumeration for `k = 1` proves that player 0 loses against optimal play; even an optimal strategy cannot promise a win in every seat and parameter setting.

## Presentation outline

Explain the two nonnegative stability margins and bit-mask action generation first. Then describe placement as adversarial search over different inventories, using mobility evaluation, alpha-beta ordering, and full-game Monte Carlo rollouts. Finally, explain removal as an impartial subset game where a winning state has a losing child. Distinguish exact completed proofs from heuristic estimates, and use the independent torque oracle, swapped seats, varied `k`, and clock settings in [TESTING.md](TESTING.md) to explain the evidence and its limits.
