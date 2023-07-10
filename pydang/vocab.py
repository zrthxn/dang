"""
Module containing symbol sets
"""


QUOTE = '\"'
"""Symbol enclosing a string literal"""

COMMENT = '#'

NULL = '\0'

DIGITS = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
}

OPERATORS = {
    ADD := '+', 
    SUB := '-',
    MUL := '*', 
    DIV := '/', 
    EQL := '=', 
    
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
    PAREN_OPN := '(', 
    PAREN_END := ')',
    
    BOXBR_OPN := '[', 
    BOXBR_END := ']', 
    
    BRACE_OPN := '{', 
    BRACE_END := '}', 
}


PUNCTUATION = {
    LF := '\n',
    CR := '\r',
    TAB := '\t',
    
    SPACE := ' ',
    COMMA := ',',
    DELIM := ';',
    
    *OPERATORS,
    *CLOSURES
}


KEYWORDS = {
    KW_LET      := "let",
    KW_FN       := "fn",
    KW_IF       := "if",
    KW_THEN     := "then",
    KW_ELIF     := "elif",
    KW_ELSE     := "else",
    KW_END      := "end",
    KW_WHILE    := "while",
    KW_DO       := "do",
    KW_RETURN   := "return",
    KW_INCLUDE  := "include",
    KW_MACRO    := "macro",
    KW_NULL     := "null",
    KW_ASM      := "asm",
}


TOKEN_TYPES = {
    KeywordToken := "keyword",
    IdentifierToken := "identifier",
    OperatorToken := "operator",
    ClosureToken := "closure",
    LiteralToken := "literal",
    PunctToken := "punct",
}
