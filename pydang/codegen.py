from typing import List
from .lexer import Token
from .parser import Statement, Container


def gen_amd64(statements: Container | List[Statement]):
    ASM = str()
    
    for line in statements:
        print(line)

        # STATEMENTS
        if type(line) == Statement:
            ...
        
        # CONTAINERS
        elif type(line) == Container:
            ...
        
        # LITERALS and IDENTIFIERS
        elif type(line) == Token:
            ...