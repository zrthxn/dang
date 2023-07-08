#!python
from sys import argv

from pydang import dangcli
from pydang import lexer

 
if __name__ == "__main__":
    # Parse the arguments to the compiler
    args = dangcli.parse_args(argv[1:])
    
    # Read file
    with open(args.filename, 'r') as f:
        code = f.read()
    
    # Lexing
    lexicon = lexer.lex(code)
    for l in lexicon:
        print(l)
    
    # Parsing
    
    # Codegen for platform
    