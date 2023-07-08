from typing import List
from .utils import index_to_loc


class Vocab:
    LF = '\n'
    CR = '\r'
    TAB = '\t'
    QUOTE = '\"'
    SPACE = ' '
    COMMA = ','
    DELIM = ';'
    COMMENT = '#'
    NULL = '\0'
    
    OPERATORS = {
        '+', 
        '-',
        '*', 
        '/', 
        '=', 
        
        '.', 
        '?', 
        '@', 
        
        '!',  
        '$', 
        '%', 
        '&', 
        '^', 
        '~', 
    }
    
    CLOSURES = {
        '(', 
        ')',
        '[', 
        ']', 
        '{', 
        '}', 
    }

    PUNCTUATION = {
        LF,
        CR,
        TAB,
        SPACE,
        DELIM,
        COMMA,
        NULL,
        
        *OPERATORS,
        *CLOSURES
    }


class Word:
    """ Container of a word which holds the word
        and its location in the module.
    """
    
    lex: str
    loc: tuple
    
    def __init__(self, _w, _l) -> None:
        self.lex = _w
        self.loc = _l
        
    def __repr__(self) -> str:
        return f"Word{self.loc}<{self.lex}>"


def lex(file: str) -> List[Word]:
    lexicon = []
    current = ""
    
    for index, char in enumerate(file):
        loc = index_to_loc(file, index - len(current))
        
        # comments: ignore line until you see LF or CR
        if len(current) >= 1 and current[0] == Vocab.COMMENT:
            if char != Vocab.LF and char != Vocab.CR:
                continue
            current = ""
            
        # string literals: unless first char of current word is quote and the quote isnt closed
        if len(current) > 1 and current[0] == Vocab.QUOTE:
            if current[-1] != Vocab.QUOTE:
                current += char
            else:
                lexicon.append(Word(current, loc))
                current = ""
            continue
            
        if char in Vocab.PUNCTUATION:
            # Add new lex on LF, CR, TAB, SPACE
            if current:
                lexicon.append(Word(current, loc))
            
            # Add new lex on OPERATORS or CLOSURES or DELIM
            if (char in Vocab.OPERATORS) or (char in Vocab.CLOSURES) or (char == Vocab.DELIM):
                lexicon.append(Word(char, index_to_loc(file, index)))
            
            current = ""
        
        # Otherwise consider it part of the same word
        else:
            current += char
            
    return lexicon