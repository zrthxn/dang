#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.c"
#include "parser.c"
#include "utils.c"

#ifndef CODEGEN_C_INCLUDED
#define CODEGEN_C_INCLUDED
// --------------------------
// Targets ------------------

TokenStream rippleDeleteTokens(TokenStream *head, uint count) {
  Token *join = *head;
  // join->next->prev = NULL;
  while (count--)
    join = join->next;

  (*head)->next = join->next;
  if (join->next != NULL)
    join->next->prev = *head;

  // join->next = NULL;
  // while (join != NULL) {
  //   free(join);
  //   join = join->prev;
  // }

  return *head;
}

TokenStream replaceToken(TokenStream *head, Token *node) {
  (*head)->prev->next = node;
  (*head)->next->prev = node;
  node->prev = (*head)->prev;
  node->next = (*head)->next;
  *head = node;
  return *head;
}

str syscall_argloc(uint narg) {
  switch (narg) {
  case 0:
    return "rax";
  case 1:
    return "rdi";
  case 2:
    return "rsi";
  case 3:
    return "rdx";
  case 4:
    return "r10";
  case 5:
    return "r8";
  case 6:
    return "r9";
  default:
    CompilerError(fstr("Invalid argument number \"%zu\" for syscall.", narg));
  }
}

str _val_(Token token) {
  switch (token.type) {
  case DeclarationToken:
  case IdentifierToken:
    return fstr("[%s]", token.value.__i.name);

  case MemoryToken:
    return token.value.m;

  case KeywordToken: // TBR
    return fstr("%d", token.value.__k);

  case ProcedureToken: // TBR
    return fstr("%s", token.value.__f.name);

  case OperatorToken: // TBR
    return fstr("OP%d", token.value.__o);

  case LiteralToken: {
    switch (token.value.__l.type) {
    case FloatValue:
    case StringValue:
      return fstr("%s", token.value.__l.value.__s);

    case IntValue:
      return fstr("%d", token.value.__l.value.__i);

    default:
      return "";
    }
  }

  default:
    return "";
  }
}

str _addr_(Token token) {
  switch (token.type) {
  case DeclarationToken:
  case IdentifierToken:
    return fstr("%s", token.value.__i.name);

  case MemoryToken:
    return token.value.m;

  case LiteralToken: {
    switch (token.value.__l.type) {
    case FloatValue:
    case StringValue:
      return token.value.__l.value.__s;

    case IntValue:
      return fstr("%d", token.value.__l.value.__i);

    default:
      return "";
    }
  }

  default:
    return "";
  }
}

bool isTokenOperable(Token *token) {
  switch (token->type) {
  case LiteralToken:
  case MemoryToken:
  case DeclarationToken:
  case IdentifierToken:
    return true;

  default:
    return false;
  }
}

bool isUnaryOperator(Operator op) {
  if (op > _O && op < __UNARY_OPERATIONS)
    return true;
  return false;
}

bool isBinaryOperator(Operator op) {
  if (op > __UNARY_OPERATIONS && op < __BINARY_OPERATIONS)
    return true;
  return false;
}

bool isNnaryOperator(Operator op) {
  if (op > __BINARY_OPERATIONS && op < __NNARY_OPERATIONS)
    return true;
  return false;
}

void resolveOperator(str *ops, Token *operator);

void resolveUnaryOperator(str *ops, Token *operator) {
  if (operator->next->type == OperatorToken)
    resolveOperator(ops, operator->next);

  Token op_arg1 = *operator->next;

  // printf("\n%s\n", *ops);
  // printf("[%s:%x] %s:%x \n", _val_(*operator), operator,
  //       _val_(op_arg1), operator->next
  //       );

  if (!isTokenOperable(&op_arg1))
    CompilerError(
        fstr("Invalid first operand of type %d.", strTokenType(op_arg1.type)));

  switch (operator->value.__o) {
  case BIT_NOT: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    // fline(ops, "xor rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }
  }
}

void resolveBinaryOperator(str *ops, Token *operator) {
  if (operator->next->type == OperatorToken)
    resolveOperator(ops, operator->next);
  if (operator->next->next->type == OperatorToken)
    resolveOperator(ops, operator->next->next);

  Token op_arg1 = *operator->next;
  Token op_arg2 = *operator->next->next;

  // printf("\n%s\n", *ops);
  // printf("[%s:%x] %s:%x | %s:%x \n", _val_(*operator), operator,
  //       _val_(op_arg1), operator->next,
  //       _val_(op_arg2), operator->next->next
  //       );

  if (!isTokenOperable(&op_arg1))
    CompilerError(
        fstr("Invalid first operand of type %d.", strTokenType(op_arg1.type)));
  if (!isTokenOperable(&op_arg2))
    CompilerError(
        fstr("Invalid second operand of type %d.", strTokenType(op_arg2.type)));

  switch (operator->value.__o) {
  case ASSIGN: {
    if (op_arg1.type != IdentifierToken && op_arg1.type != DeclarationToken)
      CompilerError("Assigning to non-identifier");

    Identifier dest = op_arg1.value.__i;

    if (op_arg2.type == IdentifierToken) {
      fline(ops, "mov rsi, %s", _val_(op_arg2));
      fline(ops, "mov qword [%s], rsi", dest.name);
    } else if (op_arg2.type == LiteralToken)
      fline(ops, "mov qword [%s], %s", dest.name, _addr_(op_arg2));
    else if (op_arg2.type == MemoryToken)
      fline(ops, "mov [%s], %s", dest.name, _addr_(op_arg2));
    else
      CompilerError("Assignment should be from literal or variable.");

    rippleDeleteTokens(&operator, 2);
    break;
  }

  case ADD: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    fline(ops, "add rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }

  case SUB: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    fline(ops, "sub rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }

  case MUL: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    fline(ops, "imul rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }

  case BIT_AND: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    fline(ops, "and rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }

  case BIT_OR: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    fline(ops, "or rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }

  case BIT_XOR: {
    fline(ops, "mov rax, %s", _val_(op_arg1));
    fline(ops, "xor rax, %s", _val_(op_arg2));
    wline(ops, "mov rdx, rax");

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, 2);
    break;
  }
  }
}

void resolveNnaryOperator(str *ops, Token *operator) {
  if (operator->next->type == OperatorToken)
    resolveOperator(ops, operator->next);

  Token op_arg1 = *operator->next;

  if (!isTokenOperable(&op_arg1))
    CompilerError(
        fstr("Invalid first operand of type %d.", strTokenType(op_arg1.type)));

  switch (operator->value.__o) {
  case CALL: {
    if (op_arg1.type != IdentifierToken)
      CompilerError("Call to non-function");

    TokenStream head = operator;
    while (head != NULL) {
      head = head->prev;
      if (head->type == ProcedureToken &&
          strcmp(head->value.__f.name, op_arg1.value.__f.name) == 0) {
        memcpy(&op_arg1.value, &(head->value), sizeof(TokenValue));
        break;
      }
    }

    Function fn = op_arg1.value.__f;
    uint nargs = fn.nargs;
    Token *args = op_arg1.next;

    while (nargs > 0) {
      if (args->type == OperatorToken)
        resolveOperator(ops, args);
      fline(ops, "xor rbx, rbx");
      fline(ops, "mov rbx, %s", _val_(*args));
      fline(ops, "push rbx", _addr_(*args));
      args = args->next;
      --nargs;
    }

    fline(ops, "call %s", fn.name);
    fline(ops, "mov rdx, [_rtn%s]", fn.name);

    operator->type = MemoryToken;
    operator->value.m = fstr("rdx");
    rippleDeleteTokens(&operator, fn.nargs);
    break;
  }
  }
}

void resolveOperator(str *ops, Token *operator) {
  if (isUnaryOperator(operator->value.__o))
    return resolveUnaryOperator(ops, operator);
  if (isBinaryOperator(operator->value.__o))
    return resolveBinaryOperator(ops, operator);
  if (isNnaryOperator(operator->value.__o))
    return resolveNnaryOperator(ops, operator);
}

bool isBlockKeyword(Keyword key) {
  switch (key) {
  case FN:
  case IF:
  case WHILE:
  case MACRO:
    return true;

  default:
    return false;
  }
}

void resolveProcedure(str *func, Token *token) {
  Function fn = token->value.__f;
  fline(func, "%s:", fn.name);

  token = token->next;

  for (size_t i = 0; i < fn.nargs; i++) {
    wline(func, "pop rsi");
    fline(func, "pop qword %s", _val_(*token));
    wline(func, "push rsi");
    token = token->next;
  }
    
  uint localBlockDepth = 0;
  while (true) {
    if (token->type == KeywordToken) {
      if (isBlockKeyword(token->value.__k))
        localBlockDepth++;
      else if (token->value.__k == END) {
        if (localBlockDepth > 0)
          localBlockDepth--;
        else
          break;
      }
    } else if (token->type == DeclarationToken)
      strcpy(token->value.__i.name,
             fstr("%s%s", fn.name, token->value.__i.name));

    token = token->next;
  }
}

void clean_codegen(str *code) {
  for (size_t i = 0; i < strlen((*code)); i++)
    if (isalnum((*code)[i]) == 0 && ispunct((*code)[i]) == 0 &&
        isspace((*code)[i]) == 0)
      (*code)[i] = ' ';
}

void codegen(TokenStream _stream_head, str outfile) {
  str head = malloc(sizeof(char));
  str text = malloc(sizeof(char));
  str func = malloc(sizeof(char));
  str data = malloc(sizeof(char));
  str bss = malloc(sizeof(char));

  TokenStream HEAD = _stream_head;

  // Header
  fline(&head, "BITS %d\n", 64);
  wline(&head, "section .text");

  wline(&data, "section .data");
  wline(&bss, "section .bss");

  uint istr = 0, iflt = 0;
  while (HEAD != NULL) {
    Token *token = HEAD;
    HEAD = HEAD->next;

    // Pre-allocate addresses for literals
    if (token->type == LiteralToken) {
      Literal *literal = &token->value.__l;
      switch (literal->type) {
      case StringValue: {
        str str_name = fstr("str%u", istr++);
        fline(&data, "%s: db \"%s\", 0x00", str_name, literal->value.__s);
        literal->value.__s = malloc(strlen(str_name) * sizeof(char));
        strcpy(literal->value.__s, str_name);
        break;
      }

      case FloatValue:

      default:
        break;
      }
    }

    // Reserve memory for variables
    else if (token->type == DeclarationToken) {
      Identifier *iden = &token->value.__i;
      strcpy(iden->name, fstr("_var_%s", iden->name));
      fline(&bss, "%s: resb %d", iden->name, iden->msize);
    }

    // Reserve memory for variables
    else if (token->type == ProcedureToken) {
      Function *function = &(token->value.__f);

      strcpy(function->name, fstr("_fn_%s", function->name));
      str rt_name = fstr("_rtn%s", function->name);
      fline(&bss, "%s: resb %d", rt_name, typeSize(function->type));

      if (token->next->type != ExpressionStartToken)
        continue; // no args

      uint params = 0;
      rippleDeleteTokens(&token, 1); // remove opening paren
      while (token->next->type != ExpressionEndToken) {
        token = token->next;
        if (token->type != DeclarationToken)
          CompilerError(fstr("Invalid %s token in function declaration",
                             strTokenType(token->type)));

        params++;
        Identifier *arg = &token->value.__i;
        strcpy(arg->name, fstr("_var%s_%s", function->name, arg->name));
        fline(&bss, "%s: resb %d", arg->name, arg->msize);
      }

      rippleDeleteTokens(&token, 1); // remove closing paren
      function->nargs = params;
      HEAD = token->next;
    }
  }

  // Iterate through functions
  wline(&text, "global _start");
  wline(&text, "_start:");

  // Iterate through tokens
  HEAD = _stream_head;
  bool isProcedure = false;
  uint blockDepth = 0;
  str *targ;

  printf("---Processed---\n");
  while (HEAD != NULL) {
    Token *token = HEAD;
    HEAD = HEAD->next;
    targ = (isProcedure) ? &func : &text;

    printf("[%x] %s -> %s\n", token, strTokenType(token->type), _val_(*token));

    switch (token->type) {
    case KeywordToken: {
      Keyword key = token->value.__k;
      if (isBlockKeyword(key))
        blockDepth++;

      switch (key) {
      case SYSCALL: {
        uint _syscall_nargs = 0;
        token = token->next; // Move past this keyword
        while (token->value.__k != END) {
          str loc = syscall_argloc(_syscall_nargs++);
          fline(targ, "mov %s, %s", loc, _val_(*token));
          token = token->next;
        }

        wline(targ, "syscall");
        HEAD = token;
        break;
      }

      case END: {
        if (blockDepth > 0)
          blockDepth--;

        if (blockDepth == 0 && isProcedure) {
          wline(targ, "ret");
          isProcedure = false;
        }
        break;
      }

      case INCLUDE:
      case MACRO:
      case IF:
      case ELSE:
      case WHILE:

      default:
        break;
      }

      break;
    }

    case LiteralToken:
      fline(targ, "mov rax, %s", _val_(*token));
      break;

    case OperatorToken:
      resolveOperator(targ, token);
      break;

    case ProcedureToken: {
      resolveProcedure(&func, token);
      isProcedure = true;
      blockDepth++;
      break;
    }
    }
  }

  // Add return 0 at end
  wline(&text, "mov rax, 60");
  wline(&text, "mov rdi, 0");
  wline(&text, "syscall");

  // Temporary clean to handle string corruption
  // @todo find why this is happening
  // clean_codegen(&head);
  // clean_codegen(&data);
  // clean_codegen(&text);
  // clean_codegen(&bss);

  FILE *fout = fopen(outfile, "w");
  if (fout == NULL)
    CompilerError(fstr("Couldn't create \"%s\".", outfile));

  // Write all strings to file
  fprintf(fout, "%s\n", head);
  fprintf(fout, "%s\n", func);
  fprintf(fout, "%s\n\n", text);
  fprintf(fout, "%s\n\n", data);
  fprintf(fout, "%s\n\n", bss);

  fclose(fout);
}

#endif