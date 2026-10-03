# Sample student strategy using the same typed API as bots/templates/julia.
occupied(state, p) = any(block -> block.position == p, state.board)
stable_after_add(state, p, w) = state.torque_left - w * (p + 3) <= 0 && state.torque_right - w * (p + 1) >= 0
function stable_after_remove(state, p)
    block = only(filter(x -> x.position == p, state.board))
    state.torque_left + block.weight * (p + 3) <= 0 && state.torque_right + block.weight * (p + 1) >= 0
end
function choose_move(state::GameState)
    if state.phase == "add"
        for w in state.remaining[state.player + 1], p in -30:30
            !occupied(state, p) && stable_after_add(state, p, w) && return Move(p, w, true)
        end
        for p in -30:30
            !occupied(state, p) && return Move(p, first(state.remaining[state.player + 1]), true)
        end
    else
        for block in state.board
            stable_after_remove(state, block.position) && return Move(block.position, 0, false)
        end
        !isempty(state.board) && return Move(first(state.board).position, 0, false)
    end
    error("no legal move available")
end
