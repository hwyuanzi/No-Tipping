# Sample uses the same organizer-owned wrapper and typed strategy API as the template.
include(joinpath(@__DIR__, "../../templates/julia/protocol.jl"))
include(joinpath(@__DIR__, "strategy.jl"))
for line in eachline(stdin)
    try
        move = choose_move(parse_state(line))
        move isa Move || error("choose_move must return Move")
        println(encode_move(move)); flush(stdout)
    catch err
        println(stderr, "strategy error: ", sprint(showerror, err)); exit(1)
    end
end
