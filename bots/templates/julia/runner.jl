# Organizer-owned wrapper. Students edit strategy.jl only.
include("strategy.jl")
for line in eachline(stdin)
    try
        move = choose_move(line)
        move isa AbstractString || error("choose_move must return a JSON string")
        println(move); flush(stdout)
    catch err
        println(stderr, "strategy error: ", sprint(showerror, err)); exit(1)
    end
end
