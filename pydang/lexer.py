from typing import List
from .utils import index_to_loc
from . import vocab


class Token:
    """ Container of a word which holds the word
        and its location in the module.
    """
    
    lex: str
    loc: tuple
    cls: str
    
    def __init__(self, _w, _l = tuple()) -> None:
        self.lex = _w
        self.loc = _l
        
        if _w in vocab.PUNCTUATION:
            if _w in vocab.CLOSURES:
                self.cls = "closure"
            elif _w in vocab.OPERATORS:
                self.cls = "operator"
            else:
                self.cls = "punct"
        elif _w in vocab.KEYWORDS:
            self.cls = "keyword"
        elif _w[0] == vocab.QUOTE and _w[-1] == vocab.QUOTE:
            self.cls = "literal"
        elif _w[0] in vocab.DIGITS and "." in _w:
            self.cls = "literal"
        elif _w[0] in vocab.DIGITS:
            self.cls = "literal"
        else:
            self.cls = "identifier"
            
        assert self.cls in vocab.TOKEN_TYPES
    
    def __repr__(self) -> str:
        return f"{self.cls}<{self.lex}>"
    
    def __eq__(self, __value: str) -> bool:
        return self.lex == __value 


def tokenize(file: str) -> List[Token]:
    lexicon = []
    current = ""
    
    for index, char in enumerate(file):
        loc = index_to_loc(file, index - len(current))
        
        # comments: ignore line until you see LF or CR
        if len(current) >= 1 and current[0] == vocab.COMMENT:
            if char != vocab.LF and char != vocab.CR:
                continue
            current = ""
            
        # string literals: unless first char of current word is quote and the quote isnt closed
        if len(current) > 1 and current[0] == vocab.QUOTE:
            if char != vocab.QUOTE:
                current += char
            else:
                lexicon.append(Token(current + char, loc))
                current = ""
            continue
            
        if char in vocab.PUNCTUATION:
            # Add new token on LF, CR, TAB, SPACE
            if current:
                lexicon.append(Token(current, loc))
            
            # Add new token on OPERATORS or CLOSURES or DELIM
            if (char in vocab.OPERATORS) or (char in vocab.CLOSURES) or (char == vocab.DELIM):
                lexicon.append(Token(char, index_to_loc(file, index)))
            
            current = ""
        
        # Otherwise consider it part of the same word
        else:
            current += char
            
    return lexicon