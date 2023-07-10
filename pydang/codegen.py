from typing import List
from .parser import Statement


def gen_amd64(statements: List[Statement]):
    for line in statements:
        print(line)
    ...