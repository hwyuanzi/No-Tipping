# Python strategy template

Edit `strategy.py` and implement `choose_move(state)`. The wrapper parses one
JSON state per turn and serializes the returned dictionary. Do not add stdin,
stdout, JSON, flushing, or process-management code.

Return `{"position": position, "weight": weight}` during placement and
`{"position": position}` during removal. The dictionary contains the same
state fields exposed by the C, C++, and Julia templates.
