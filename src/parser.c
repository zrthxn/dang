#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.c"
#include "utils.c"

#ifndef PARSER_C_INCLUDED
#define PARSER_C_INCLUDED
// --------------------------
// Parser -------------------

typedef enum KeywordTypes {
  _K = 100,
  LET,
  FN,
  IF,
  THEN,
  ELIF,
  ELSE,
  END,
  WHILE,
  DO,
  RETURN,
  INCLUDE,
  SYSCALL,
  MACRO,
  __KEYWORDS_COUNT,
} Keyword;

typedef enum OperatorTypes {
  _O = __KEYWORDS_COUNT,
  // Binary Ops
  ADD, // +
  SUB, // -
  MUL, // *
  DIV, // /
  // MOD, // %

  ASSIGN, // =
  CALL,   // <|

  LOGICAL_GREATER_THAN, // >
  LOGICAL_LESS_THAN,    // <
  LOGICAL_EQUAL,        // ==
  LOGICAL_NOT_EQUAL,    // !=
  LOGICAL_AND,          // &&
  LOGICAL_OR,           // ||
  LOGICAL_XOR,          // ^^

  BIT_SHIFT_LEFT,  // <<
  BIT_SHIFT_RIGHT, // >>

  BIT_AND, // &
  BIT_OR,  // |
  BIT_XOR, // ^

  // Unary Ops
  BIT_NOT,     // !
  LOGICAL_NOT, // !!

  // INCREMENT, // ++
  // DECREMENT, // --
  // DEREF, // *
  // ACCESSOR, // .
  __OPERATIONS_COUNT,
} Operator;

typedef enum ValueTypes {
  _V = __OPERATIONS_COUNT,
  IntValue,
  FloatValue,
  StringValue,
  NullValue,
  __VALUETYPES_COUNT,
} Type;

typedef union {
  int __i;
  float __f;
  str __s;
  void *n;
} LiteralValue;

typedef struct {
  Type type;
  LiteralValue value;
  uint msize;
} Literal;

typedef struct {
  str name;
  Type type;
  uint msize;
} Identifier;

typedef struct {
  str name;
  Type type;
  uint nargs;
} Function;

// Tokenization Types ----
typedef enum {
  _T = __VALUETYPES_COUNT,
  KeywordToken,
  LiteralToken,
  MemoryToken,
  DeclarationToken,
  ProcedureToken,
  IdentifierToken,
  OperatorToken,
  ExpressionStartToken,
  ExpressionEndToken,
  __TOKENTYPES_COUNT
} TokenType;

typedef union {
  Keyword __k;
  Operator __o;
  Identifier __i;
  Function __f;
  Literal __l;
  str m;
  void *n;
} TokenValue;

struct TokenStreamNode {
  TokenType type;
  TokenValue value;

  struct TokenStreamNode *next;
  struct TokenStreamNode *prev;
};

typedef struct TokenStreamNode Token;
typedef Token *TokenStream;

void pushToken(TokenStream *stream, uint *len) {
  TokenStream __new = malloc((++(*len)) * sizeof(Token));
  if ((*len) > 1)
    for (size_t i = 0; i < (*len) - 1; i++)
      __new[i] = (*stream)[i];

  // memcpy(&__new[(*len)-1], el, sizeof(Token));
  __new[(*len) - 1] = (Token){};
  *stream = __new;
}

Token *popToken(TokenStream stream, uint *len) { return &stream[--(*len)]; }

bool isInteger(str word) {
  uint __i = 0;
  if (word[0] == '-')
    __i++;

  while (word[__i] != '\0')
    if (!isdigit(word[__i++]))
      return false;

  // str INT64_MAX_STR = "9223372036854775807";
  // for (size_t i = 0; i < __i; i++) {
  // 	if (INT64_MAX_STR[i] == '\0' || word[i] > INT64_MAX_STR[i]) {
  // 		fprintf(stderr, "\nError: Integer \"%s\" larger than
  // 64-bits.\n", word); 		exit(1);
  // 	}
  // }

  return true;
}

bool isFloat(str word) {
  uint __i = 0;
  if (word[0] == '-')
    __i++;

  uint point = strcspn(word, ".");

  /**
   * @todo Check floatable
   */

  while (word[__i] != '\0')
    if (!isdigit(word[__i]))
      return false;

  return true;
}

Type parseStringType(str type) {
  if (strcmp(type, "int") == 0)
    return IntValue;
  if (strcmp(type, "float") == 0)
    return FloatValue;
  if (strcmp(type, "str") == 0)
    return StringValue;
  /* Fallback */ return IntValue;
}

uint parseTypeSize(str type) {
  if (strcmp(type, "int") == 0)
    return sizeof(__int64_t);
  if (strcmp(type, "float") == 0)
    return sizeof(float);
  if (strcmp(type, "str") == 0)
    return sizeof(char *);
  /* Fallback */ return sizeof(char *);
}

void pushBack(TokenStream *head, Token *node) {
  node->next = NULL;
  node->prev = *head;

  if (*head != NULL)
    (*head)->next = node;

  *head = node;
}

void pushInsertPrevious(TokenStream *head, Token *node) {
  Token *prev = (*head)->prev;
  prev->next = node;

  node->next = *head;
  node->prev = prev;

  (*head)->next = NULL;
  (*head)->prev = node;
}

TokenStream parse(const str filename) {
  /**
   * @brief Parse words into token stream
   */

  uint lexsize = 0;
  str *lexicon;
  lex(filename, &lexicon, &lexsize);

  Token *_stream_head = NULL;

  while (lexsize) {
    const str word = *lexicon;
    const uint len = strlen(word);

    if /* Handle comments */ (word[0] == COMMENT) {
      // Ignore for now
    } else if /* Delimiter */ (strcmp(word, ";") == 0) {
      // Ignore for now, auto splitting
    } else if /* Expression Start */ (strcmp(word, "[") == 0 ||
                                      strcmp(word, "{") == 0 ||
                                      strcmp(word, "(") == 0) {
      // check previous
      Token *tail = malloc(sizeof(Token));
      tail->type = ExpressionStartToken;
      tail->value = (TokenValue)NULL;

      pushBack(&_stream_head, tail);
    } else if /* Expression End */ (strcmp(word, "]") == 0 ||
                                    strcmp(word, "}") == 0 ||
                                    strcmp(word, ")") == 0) {
      // check previous
      Token *tail = malloc(sizeof(Token));
      tail->type = ExpressionEndToken;
      tail->value = (TokenValue)NULL;
			
      pushBack(&_stream_head, tail);
    }
    // Keywords ---------------------------------------------------------------
    else if /* Include other files */ (strcmp(word, "include") == 0) {
      lexicon = &lexicon[1];
      lexsize--;

      str arg = *lexicon;
      setTargetCompiling(arg);

      TokenStream _include_ = parse(arg);
      _include_->prev = _stream_head;
      _stream_head->next = _include_;

      while (_include_->next != NULL)
        _stream_head = _include_->next;
    } else if /* End */ (strcmp(word, "while") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)WHILE;

      pushBack(&_stream_head, tail);
    } else if /* End */ (strcmp(word, "if") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)IF;

      pushBack(&_stream_head, tail);
    } else if /* End */ (strcmp(word, "then") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)THEN;

      pushBack(&_stream_head, tail);
    } else if /* End */ (strcmp(word, "elif") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)ELIF;

      pushBack(&_stream_head, tail);
    } else if /* Else */ (strcmp(word, "else") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)ELSE;

      pushBack(&_stream_head, tail);
    } else if /* Do */ (strcmp(word, "do") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)DO;

      pushBack(&_stream_head, tail);
    } else if /* End */ (strcmp(word, "end") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)END;

      pushBack(&_stream_head, tail);
    } else if /* Return */ (strcmp(word, "return") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)RETURN;

      pushBack(&_stream_head, tail);
    } else if /* Syscall */ (strcmp(word, "syscall") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)SYSCALL;

      pushBack(&_stream_head, tail);
    } else if /* Macro */ (strcmp(word, "macro") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = KeywordToken;
      tail->value = (TokenValue)(Keyword)MACRO;

      pushBack(&_stream_head, tail);
    }
    // Declarations -----------------------------------------------------------
    else if /* Variable declaration */ (strcmp(word, "let") == 0) {
      lexicon = &lexicon[1];
      lexsize--;

      str arg = *lexicon;
      uint split = strcspn(arg, ":");
      if (split == strlen(arg))
        CompilerError(fstr("Untyped variable \"%s\" not supported yet.", arg));

      arg[split++] = '\0';
      str type = &arg[split];

      Token *tail = malloc(sizeof(Token));
      tail->type = DeclarationToken;
      tail->value = (TokenValue)(Identifier){
          .name = fstr(arg),
          .msize = parseTypeSize(type),
          .type = parseStringType(type),
      };

      pushBack(&_stream_head, tail);
    } else if /* Function Declarations */ (strcmp(word, "fn") == 0) {
      lexicon = &lexicon[1];
      lexsize--;

      str arg = *lexicon;
      uint split = strcspn(arg, ":");
      if (split == strlen(arg))
        CompilerError(fstr("Untyped function \"%s\" not supported yet.", arg));

      arg[split++] = '\0';
      str type = &arg[split];

      Token *tail = malloc(sizeof(Token));
      tail->type = ProcedureToken;
      tail->value = (TokenValue)(Function){
          .name = fstr(arg),
          .type = parseStringType(type),
          .nargs = 0,
      };

      pushBack(&_stream_head, tail);
    }
    // Operations -------------------------------------------------------------
    else if /* Assignment Operation */ (strcmp(word, "=") == 0) {
      Token *prev = _stream_head;
      if (prev->type != DeclarationToken && prev->type != IdentifierToken)
        CompilerError(fstr("Assigning to non-identifier \"%d\".", prev->type));

      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)ASSIGN;

      pushInsertPrevious(&_stream_head, tail);
    } else if /* Addition Operation */ (strcmp(word, "+") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)ADD;

      pushInsertPrevious(&_stream_head, tail);
    } else if /* Subtraction Operation */ (strcmp(word, "-") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)SUB;

      pushInsertPrevious(&_stream_head, tail);
    } else if /* Multiplication Operation */ (strcmp(word, "*") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)MUL;

      pushInsertPrevious(&_stream_head, tail);
    } else if /* Division Operation */ (strcmp(word, "/") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)DIV;

      pushInsertPrevious(&_stream_head, tail);
    } else if /* Logical AND Operation */ (strcmp(word, "&&") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)LOGICAL_AND;

      pushInsertPrevious(&_stream_head, tail);
    } else if /* Call Operation */ (strcmp(word, "<|") == 0) {
      Token *prev = _stream_head->prev;
      TokenStream head = _stream_head;

      // check if prev is a function identifier
      bool found = false;
      while (head->prev != NULL) {
        Token *curr = head->prev;
        if (curr->type == ProcedureToken &&
            strcmp(curr->value.__f.name, prev->value.__f.name) == 0) {
          found = true;
          break;
        }
        head = head->prev;
      }

      if (!found)
        CompilerError(
            fstr("Use of un-declared function \"%s\".", prev->value.__f.name));

      Token *tail = malloc(sizeof(Token));
      tail->type = OperatorToken;
      tail->value = (TokenValue)(Operator)CALL;

      pushInsertPrevious(&_stream_head, tail);
    }
    // Literals ---------------------------------------------------------------
    else if /* Int Literals */ (isInteger(word)) {
      Token *tail = malloc(sizeof(Token));
      tail->type = LiteralToken;
      tail->value = (TokenValue)(Literal){
          .type = IntValue,
          .value = (LiteralValue)(int)atoi(word),
          .msize = sizeof(__int64_t),
      };

      pushBack(&_stream_head, tail);
    } else if /* String Literals */ (word[0] == '\"' && word[len - 1] == '\"') {
      str value = &word[1];
      value[len - 2] = '\0';

      Token *tail = malloc(sizeof(Token));
      tail->type = LiteralToken;
      tail->value = (TokenValue)(Literal){
          .type = StringValue,
          .value = (LiteralValue)(str)value,
          .msize = (len - 2) * sizeof(char),
      };

      strcpy(tail->value.__l.value.__s, value);
      pushBack(&_stream_head, tail);
    } else if /* Float Literals */ (isFloat(word)) {
      Token *tail = malloc(sizeof(Token));
      tail->type = LiteralToken;
      tail->value = (TokenValue)(Literal){
          .type = FloatValue,
          .value = (LiteralValue)(float)atof(word),
          .msize = sizeof(float),
      };

      pushBack(&_stream_head, tail);
    } else if /* NULL Literals */ (strcmp(word, "null") == 0) {
      Token *tail = malloc(sizeof(Token));
      tail->type = LiteralToken;
      tail->value = (TokenValue)(Literal){
          .type = NullValue,
          .msize = sizeof(void *),
          .value = (LiteralValue)(int)0,
      };

      pushBack(&_stream_head, tail);
    }
    // Word or Identifier -----------------------------------------------------
    else if (isalpha(word[0]) && strlen(word) <= 32) {
      bool found = false;

      TokenStream head = _stream_head;
      Token curr = *head;
      while (head != NULL && !found) {
        curr = *head;
        head = head->prev;

        if (curr.type == DeclarationToken &&
            strcmp(curr.value.__i.name, word) == 0)
          found = true;
        else if (curr.type == ProcedureToken &&
                 strcmp(curr.value.__f.name, word) == 0)
          found = true;
      }

      if (!found)
        CompilerError(fstr("Un-declared identifier \"%s\".", word));

      Token *tail = malloc(sizeof(Token));
      tail->type = IdentifierToken;
      tail->value = curr.value;
      memcpy(&(tail->value), &curr.value, sizeof(TokenValue));

      pushBack(&_stream_head, tail);
    }

    else /* Something unrecognised was thrown own way */
      CompilerError(fstr("Unknown word \"%s\".", word));

    // Advance to next lex
    lexicon = &lexicon[1];
    lexsize--;
  }

  // Roll pointer back to begenning of stream
  while (_stream_head->prev != NULL)
    _stream_head = _stream_head->prev;

  return _stream_head;
}

#endif