# Starter template: implement choose_move and return one JSON move per input line.
function choose_move(state)
    # TODO: Parse state and return a JSON move string, e.g. "{\"position\":-3,\"weight\":2}".
    return "{}"
end

for state in eachline(stdin)
    println(choose_move(state))
    flush(stdout)
end
