from typing import List
from .vocab import *
from .lexer import tokenize, Token
from .utils import Scope


class Statement:
    """ Statements are single operations that do only one thing. 
        Each statement must be an operation. Like "add 2,5". 
        
        A statement represents an operation. It doesn't necessarily change the stack. 
        It depends on the operation to decide what to do.
    """
    op: Token
    tokens: List[Token]
    
    def __init__(self, op, tokens = []) -> None:
        self.op = op
        self.tokens = tokens if type(tokens) == list else [tokens]
        
    def __repr__(self) -> str:
        return f"Statement<{self.op.lex}>({self.tokens})"
    
    
class Container:
    """ Containers are boxes of tokens, statements or other containers.
        They can be named or anonymous.
        
        A container represents a subroutine which has a return value.
    """
    id: Token
    statements: List[Statement]
    
    def __init__(self, statements, id = "") -> None:
        assert type(statements) == list
        self.statements = statements
        self.id = id
        
    def __repr__(self) -> str:
        return f"<{self.id}<{repr(self.statements)}>>"
    
    def __getitem__(self, index):
        return self.statements[index]
        
    def push(self, statements: Statement | List[Statement]):
        if type(statements) == list:
            self.statements.extend(statements)
        else:
            self.statements.append(statements)

    
def resolve_block(stream: List[Token], scope: Scope = None) -> Container:
    """ Creates a new scope and resolves some 
        tokens into container of statements.
    """
    
    EOFIX = len(stream) - 1
    SCOPE = Scope.from_parent(scope) if scope else Scope()
    OPS = Container([])
    
    # Create logical OPS from token stream
    index = 0
    while index <= EOFIX:
        token = stream[index]
        
        # KEYWORDS
        if token.cls == KeywordToken:
            if token == KW_LET:
                # Expect: (let) <identifier>; 
                # Expect: (let) <identifier> = <value>;
                assert (identifier := stream[index + 1]).cls == IdentifierToken, "Expected Identifier"
                OPS.push(Statement(token, identifier))
                SCOPE.push(identifier.lex, identifier)
                index += 1
                
        
        # OPERATIONS
        elif token.cls == OperatorToken:
            if token == ASSIGN:
                # Expect: <SP:destination> (=) <value>;
                vix = stream.index(DELIM, index + 1)
                value = resolve_block(stream[index + 1 : vix + 1], SCOPE)
                OPS.push(Statement(token, value))
                index = vix
                
            elif token == ADD:
                # Expect: <SP:operand> (+) <value>;
                vix = stream.index(DELIM, index + 1)
                value_block = stream[index + 1 : vix]
                if len(value_block) == 1 and (value_block[0].cls in [IdentifierToken, LiteralToken]):
                    value = value_block[0]
                else:
                    value = resolve_block(stream[index + 1 : vix + 1], SCOPE)
                OPS.push(Statement(token, value))
                index = vix
                
            
        # LITERALS
        elif token.cls == LiteralToken:
            OPS.push(token)
        
        
        # IDENTIFIERS
        elif token.cls == IdentifierToken:
            assert token in SCOPE, f"Unknown variable {token.lex}"
            OPS.push(token)
        
        
        index += 1
        
    return OPS


def parse(filepath: str) -> List[Statement]:
    # Read file
    with open(filepath, 'r') as f:
        code = f.read()
    
    # Lexing and Parsing
    lexicon = tokenize(code)
    # resolve imports/includes
    
    return resolve_block(lexicon)
    