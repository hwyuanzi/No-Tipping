# C++ strategy template

Implement `choose_move` in `strategy.cpp`. The argument is one complete JSON
state string and the return value must be exactly one JSON move object. Build
with:

```sh
g++ -std=c++17 -O2 runner.cpp strategy.cpp -o bot
```

The organizer launches `bot`; do not add another stdin/stdout loop.
