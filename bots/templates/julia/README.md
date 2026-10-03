# Julia strategy template

Implement `choose_move(state::GameState)::Move` in `strategy.jl`. The wrapper
passes a native `GameState` and serializes the native `Move` result. The
organizer launches `runner.jl`, which owns stdin/stdout, flushing, and errors.

This template deliberately has no package dependency; the organizer-owned
`protocol.jl` contains the protocol adapter.
