# C strategy template

Implement `choose_move` in `strategy.c`. The `state` argument is one complete
JSON line and `output` must receive exactly one JSON move object. Build with:

```sh
cc -std=c11 -O2 runner.c strategy.c -o bot
```

The organizer launches `bot`; do not add another stdin/stdout loop.
