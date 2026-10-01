# Julia example with no package dependencies. The simple regex helpers parse the
# fixed protocol fields used by this sample; larger bots can use JSON3 or JSON.
function integer_field(text, key)
    m = match(Regex("\\\"" * key * "\\\"\\s*:\\s*(-?[0-9]+)"), text)
    isnothing(m) && error("missing field: " * key)
    return parse(Int, m.captures[1])
end

function parse_board(text)
    section = match(r"\"board\"\s*:\s*\[(.*?)\]"s, text)
    isnothing(section) && error("missing board")
    board = NamedTuple{(:position, :weight), Tuple{Int, Int}}[]
    for item in eachmatch(r"\{([^{}]*)\}", section.captures[1])
        object = item.captures[1]
        push!(board, (position=integer_field(object, "position"),
                      weight=integer_field(object, "weight")))
    end
    return board
end

function remaining_for(text, player)
    section = match(r"\"remaining\"\s*:\s*\[\s*\[([^\]]*)\]\s*,\s*\[([^\]]*)\]\s*\]"s, text)
    isnothing(section) && error("missing remaining weights")
    return parse.(Int, filter(value -> !isempty(value), strip.(split(section.captures[player + 1], ','))))
end

stable(left, right) = left <= 0 && right >= 0

function choose(text)
    player = integer_field(text, "player")
    left = integer_field(text, "left")
    right = integer_field(text, "right")
    board = parse_board(text)
    occupied = Set(block.position for block in board)

    if occursin(r"\"phase\"\s*:\s*\"add\"", text)
        for weight in remaining_for(text, player)
            for position in -30:30
                position in occupied && continue
                if stable(left - weight * (position + 3), right - weight * (position + 1))
                    return "{\"position\":$position,\"weight\":$weight}"
                end
            end
        end
        weight = minimum(remaining_for(text, player))
        position = first(p for p in -30:30 if !(p in occupied))
        return "{\"position\":$position,\"weight\":$weight}"
    end

    for block in board
        if stable(left + block.weight * (block.position + 3),
                  right + block.weight * (block.position + 1))
            return "{\"position\":$(block.position)}"
        end
    end
    return "{\"position\":$(board[1].position)}"
end

for input in eachline(stdin)
    println(choose(input))
    flush(stdout)
end
