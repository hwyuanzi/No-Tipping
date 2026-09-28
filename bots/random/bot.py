import json
import sys
from strategy import choose
print(json.dumps(choose(json.loads(sys.stdin.readline()))))
