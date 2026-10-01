# Julia strategy template

Implement `choose_move(state)` in `strategy.jl`. The state is one complete
JSON string and the function returns one JSON move string. The organizer
launches `runner.jl`, which owns stdin/stdout, flushing, and errors.

This template deliberately has no package dependency. Use an organizer-
approved JSON package only if the tournament environment provides it.
