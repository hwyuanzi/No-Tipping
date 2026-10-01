import json
import sys
from strategy import choose

for line in sys.stdin:
    print(json.dumps(choose(json.loads(line))), flush=True)
