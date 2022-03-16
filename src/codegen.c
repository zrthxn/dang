#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.c"
#include "lexer.c"
#include "parser.c"

#ifndef CODEGEN_C_INCLUDED
#define CODEGEN_C_INCLUDED
// --------------------------
// Targets ------------------

TokenStream rippleDeleteTokens(TokenStream *head, uint count)
{
	TokenStream *join = head;
	while (count--) *join = (*join)->next;

	(*head)->next = (*join)->next;
	if ((*join)->next != NULL)
		(*join)->next->prev = *head;

	return (*head)->next;
}

str __linux_syscall_argloc(uint narg)
{
	switch (narg)
	{
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
		return "";
	}
}

str _val_(Token token)
{
	switch (token.type)
	{
	case DeclarationToken:
		return fstr("[%s]", token.value.__i.name);
	case IdentifierToken:
		return fstr("[_var_%s]", token.value.__i.name);

	case MemoryToken:
		return token.value.m;

	case KeywordToken:
		return fstr("%d", token.value.__k);

	case OperatorToken:
		return fstr("OP%d", token.value.__o);

	case LiteralToken:
	{
		switch (token.value.__l.type)
		{
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

str _addr_(Token token)
{
	switch (token.type)
	{
	case DeclarationToken:
		return fstr("%s", token.value.__i.name);
	case IdentifierToken:
		return fstr("_var_%s", token.value.__i.name);

	case MemoryToken:
		return token.value.m;

	case LiteralToken:
	{
		switch (token.value.__l.type)
		{
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

str resolveOperator(Token *operator, TokenStream *head)
{
	// List of operation instructions
	str ops = "";

	if (operator->next->type == OperatorToken)
		wline(&ops, resolveOperator(operator->next, head));

	if (operator->next->next->type == OperatorToken)
		wline(&ops, resolveOperator(operator->next->next, head));

	Token _op_arg1 = *(operator->next);
	Token _op_arg2 = *(operator->next->next);
	
	switch (operator->value.__o)
	{
	case ASSIGN:
	{
		if (_op_arg1.type != IdentifierToken && _op_arg1.type != DeclarationToken)
			CompilerError("Assigning to non-identifier");

		str opsize = "qword";
		Identifier dest = _op_arg1.value.__i;

		if (_op_arg2.type == IdentifierToken) {
			fline(&ops, "mov rsi, %s", _val_(_op_arg2));
			fline(&ops, "mov %s [%s], rsi", opsize, dest.name);
		}
		else if (_op_arg2.type == LiteralToken)
			fline(&ops, "mov %s [%s], %s", opsize, dest.name, _addr_(_op_arg2));
		else if (_op_arg2.type == MemoryToken)
			fline(&ops, "mov [%s], %s", dest.name, _addr_(_op_arg2));
		else
			CompilerError("Assignment should be from literal or variable.");

		*head = rippleDeleteTokens(&operator, 2);
		break;
	}

	case CALL:
	{
		if (_op_arg1.type != IdentifierToken)
			CompilerError("Call to non-function");

		// printf("\nHere\n");
		// // Function fn = _op_arg1.value.__f;
		// // // Identifier fn = _op_arg1.value.__i;
		// // // fline(&ops, "call %s", fn.name);
		// printf("call %x", _op_arg1.value.__f);
		break;
	}

	case ADD:
	{
		if (_op_arg1.type != IdentifierToken && _op_arg1.type != LiteralToken && _op_arg1.type != MemoryToken)
			CompilerError("Illegal operation, first operand.");
		if (_op_arg2.type != IdentifierToken && _op_arg2.type != LiteralToken && _op_arg2.type != MemoryToken)
			CompilerError("Illegal operation, second operand.");

		fline(&ops, "mov rax, %s", _val_(_op_arg1));
		fline(&ops, "add rax, %s", _val_(_op_arg2));
		wline(&ops, "mov rdx, rax");

		(*head)->type = MemoryToken;
		(*head)->value.m = fstr("rdx");

		*head = rippleDeleteTokens(&operator, 2);
		break;
	}

	case SUB:
	{
		if (_op_arg1.type != IdentifierToken && _op_arg1.type != LiteralToken && _op_arg1.type != MemoryToken)
			CompilerError("Illegal operation, first operand.");
		if (_op_arg2.type != IdentifierToken && _op_arg2.type != LiteralToken && _op_arg2.type != MemoryToken)
			CompilerError("Illegal operation, second operand.");

		fline(&ops, "mov rax, %s", _val_(_op_arg1));
		fline(&ops, "sub rax, %s", _val_(_op_arg2));
		wline(&ops, "mov rdx, rax");

		(*head)->type = MemoryToken;
		(*head)->value.m = fstr("rdx");

		*head = rippleDeleteTokens(&operator, 2);
		break;
	}

	default: break;
	}

	return ops;
}

void clean_codegen(str *code)
{
	for (size_t i = 0; i < strlen((*code)); i++)
		if (isalnum((*code)[i]) == 0 && ispunct((*code)[i]) == 0 && isspace((*code)[i]) == 0)
			(*code)[i] = ' ';
}

void codegen(TokenStream _stream_head, str outfile)
{
	FILE *fout = fopen(outfile, "w");
	if (fout == NULL)
		CompilerError(fstr("Couldn't create \"%s\".", outfile));

	str head = "";
	str text = "";
	str data = "";
	str bss = "";

	TokenStream HEAD = _stream_head;

	// Header
	fline(&head, "BITS %d\n", 64);

	// Pre-allocate addresses for literals
	wline(&data, "section .data");
	uint istr = 0, iflt = 0;
	while (HEAD != NULL)
	{
		Token *token = HEAD;
		HEAD = HEAD->next;

		printf("%d -> %s\n", token->type, _val_(*token));

		if (token->type != LiteralToken)
			continue;

		Literal *literal = &token->value.__l;
		switch (literal->type)
		{
		case StringValue:
		{
			str str_name = fstr("str%u", istr++);
			fline(&data, "%s: db \"%s\", 0x00", str_name, literal->value.__s);

			literal->value.__s = malloc(strlen(str_name) * sizeof(char));
			strcpy(literal->value.__s, str_name);
			// literal.value.__s = str_name;
			break;
		}

		case FloatValue:

		default: break;
		}
	}

	// Reserve memory for variables
	wline(&bss, "section .bss");
	HEAD = _stream_head;
	while (HEAD != NULL)
	{
		Token *token = HEAD;
		HEAD = HEAD->next;

		if (token->type == DeclarationToken)
		{
			Identifier *iden = &token->value.__i;
			iden->name = fstr("_var_%s", iden->name);
			fline(&bss, "%s: resb %d", iden->name, iden->msize);
		}
		// else if (token->type == ProcedureToken)
		// {
		// 	Function *function = &token->value.__f;

		// 	function->name = fstr("_fn_%s", function->name);
		// 	str rt_name = fstr("rtn%s", function->name);

		// 	switch (function->type)
		// 	{
		// 	case IntValue:
		// 		fline(&bss, "%s: resb %d", rt_name, sizeof(__int64_t));
		// 		break;
		// 	case FloatValue:
		// 		fline(&bss, "%s: resb %d", rt_name, sizeof(float));
		// 		break;
		// 	case StringValue:
		// 		fline(&bss, "%s: resb %d", rt_name, sizeof(char *));
		// 		break;
		// 	default:
		// 		break;
		// 	}

		// 	// uint params = 0;

		// 	// if (stream[++_ti].type == ExpressionStartToken)
		// 	// {
		// 	// 	while (stream[_ti].type != ExpressionEndToken)
		// 	// 		if (stream[++_ti].type == DeclarationToken)
		// 	// 			params++;

		// 	// 	function->nargs = params;
		// 	// }
		// }
	}

	// Iterate through functions
	wline(&text, "section .text");
	wline(&text, "global _start");
	wline(&text, "_start:");

	// Iterate through tokens
	HEAD = _stream_head;
	while (HEAD != NULL)
	{
		Token *token = HEAD;
		HEAD = HEAD->next;

		switch (token->type)
		{
		case KeywordToken:
		{
			Keyword value = token->value.__k;
			switch (value)
			{
			case SYSCALL:
			{
				uint _syscall_nargs = 0;
				token = token->next; // Move past the SYSCALL keyword

				while (token->value.__k != END) {
					fline(&text, "mov %s, %s", __linux_syscall_argloc(_syscall_nargs++), _val_(*token));
					token = token->next;
				}

				wline(&text, "syscall");
				HEAD = token;
				break;
			}

			default:
				break;
			}
			break;
		}

		case OperatorToken:
			wline(&text, resolveOperator(token, &HEAD));
			break;

		case LiteralToken:
			fline(&text, "mov rax, %s", _val_(*token));
			break;

		case ProcedureToken:
		{
			Function fn = (*token).value.__f;
			fline(&text, "%s:", fn.name);

			break;
		}

		case ExpressionStartToken:
			/* code */
			break;

		case ExpressionEndToken:
			/* code */
			break;
		}
	}

	// return 0;
	wline(&text, "mov rax, 60");
	wline(&text, "mov rdi, 0");
	wline(&text, "syscall");

	// Temporary clean to handle string corruption
	// @todo find why this is happening
	clean_codegen(&head);
	clean_codegen(&data);
	clean_codegen(&text);
	clean_codegen(&bss);

	// Write all strings to file
	fprintf(fout, "%s", head);
	fprintf(fout, "%s\n", data);
	fprintf(fout, "%s\n", bss);
	fprintf(fout, "%s\n", text);

	fclose(fout);
}

#endif