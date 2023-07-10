from typing import List
from .lexer import tokenize, Token
from .vocab import *


class Statement:
    tokens: List[Token]
    children: List
    
    def __init__(self, _t, _c = []) -> None:
        self.tokens = _t
        self.children = _c

    def __repr__(self) -> str:
        return repr({
            "tokens": self.tokens,
            "children": self.children,
        })


def parse(filepath: str) -> List[Statement]:
    # Read file
    with open(filepath, 'r') as f:
        code = f.read()
    
    # Lexing
    lexicon = tokenize(code)
        
    # Create logical statements from token stream
    statements = []
    index = 0
    while index < len(lexicon):
        token = lexicon[index]
        # print(token)
        
        # KEYWORDS
        if token.cls == KeywordToken:
            if token.lex == KW_LET:
                assert lexicon[index + 1].cls == IdentifierToken, "Expected Identifier"
                statements.append(Statement([ token, lexicon[index + 1] ]))
                index += 1
                
        # OPERATORS
        if token.cls == OperatorToken:
            if token.lex == EQL:
                # assert lexicon[index + 1].cls == Value, "Lvalue required"
                statements.append(Statement([ token, lexicon[index + 1] ]))
                index += 1
        
        index += 1
                
        
    return statements