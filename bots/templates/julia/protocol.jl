# Organizer-owned, dependency-free JSON adapter for the tournament protocol.
mutable struct JsonParser
    text::String
    index::Int
end
function skipspace(p)
    while p.index <= lastindex(p.text) && p.text[p.index] in (' ', '\n', '\r', '\t'); p.index = nextind(p.text, p.index); end
end
function json_string(p)
    p.text[p.index] == '"' || error("expected JSON string"); p.index = nextind(p.text, p.index); out = IOBuffer()
    while p.index <= lastindex(p.text)
        c = p.text[p.index]; p.index = nextind(p.text, p.index); c == '"' && return String(take!(out))
        if c == '\\'; p.index > lastindex(p.text) && error("bad escape"); e = p.text[p.index]; p.index = nextind(p.text, p.index); write(out, e == 'n' ? '\n' : e == 'r' ? '\r' : e == 't' ? '\t' : e); else write(out, c); end
    end; error("unterminated string")
end
function json_value(p)
    skipspace(p); c = p.text[p.index]
    c == '"' && return json_string(p)
    if c == '{'; p.index = nextind(p.text, p.index); d = Dict{String,Any}(); skipspace(p); p.index <= lastindex(p.text) && p.text[p.index] == '}' && (p.index = nextind(p.text, p.index); return d)
        while true; skipspace(p); k = json_string(p); skipspace(p); p.text[p.index] == ':' || error("expected colon"); p.index = nextind(p.text, p.index); d[k] = json_value(p); skipspace(p); c = p.text[p.index]; c == '}' && (p.index = nextind(p.text, p.index); return d); c == ',' || error("expected comma"); p.index = nextind(p.text, p.index); end
    elseif c == '['; p.index = nextind(p.text, p.index); a = Any[]; skipspace(p); p.index <= lastindex(p.text) && p.text[p.index] == ']' && (p.index = nextind(p.text, p.index); return a)
        while true; push!(a, json_value(p)); skipspace(p); c = p.text[p.index]; c == ']' && (p.index = nextind(p.text, p.index); return a); c == ',' || error("expected comma"); p.index = nextind(p.text, p.index); end
    elseif startswith(p.text[p.index:end], "null"); p.index += 4; return nothing
    elseif startswith(p.text[p.index:end], "true"); p.index += 4; return true
    elseif startswith(p.text[p.index:end], "false"); p.index += 5; return false
    else; start = p.index; while p.index <= lastindex(p.text) && (p.text[p.index] in "-+0123456789.eE"); p.index = nextind(p.text, p.index); end; token = p.text[start:prevind(p.text, p.index)]; return occursin(r"[.eE]", token) ? parse(Float64, token) : parse(Int, token)
    end
end
parse_json(text) = json_value(JsonParser(text, firstindex(text)))
struct Block; position::Int; weight::Int; owner::Int; end
struct GameState
    protocol_version::Int; k::Int; phase::String; player::Int; board::Vector{Block}; remaining::Vector{Vector{Int}}
    torque_left::Int; torque_right::Int; clocks::Vector{Float64}; winner::Int; reason::Union{Nothing,String}; game_id::Union{Nothing,String}; ply::Int; players::Vector{String}
end
struct Move; position::Int; weight::Int; has_weight::Bool; end
function parse_state(text)
    d = parse_json(text); blocks = [Block(Int(x["position"]), Int(x["weight"]), x["owner"] === nothing ? -1 : Int(x["owner"])) for x in d["board"]]
    GameState(Int(d["protocol_version"]), Int(d["k"]), d["phase"], Int(d["player"]), blocks, [Int.(x) for x in d["remaining"]], Int(d["torques"]["left"]), Int(d["torques"]["right"]), Float64.(get(d, "clocks", [0.0, 0.0])), d["winner"] === nothing ? -1 : Int(d["winner"]), d["reason"], get(d, "game_id", nothing), get(d, "ply", -1), String.(get(d, "players", String[])))
end
encode_move(m::Move) = m.has_weight ? "{\"position\":$(m.position),\"weight\":$(m.weight)}" : "{\"position\":$(m.position)}"
