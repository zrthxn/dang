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

str resolveOperator(TokenStream* stream, uint *length, uint index, size_t *return_index)
{
	Operator op = (*stream)[index].value.__o;
	str ops = "";

	if ((*stream)[index + 1].type == OperatorToken)
		wline(&ops, resolveOperator(stream, length, index + 1, return_index));

	if ((*stream)[index + 2].type == OperatorToken)
		wline(&ops, resolveOperator(stream, length, index + 2, return_index));

	Token _op_arg1 = (*stream)[index + 1];
	Token _op_arg2 = (*stream)[index + 2];

	// printf("\n");
	// for (size_t i = 0; i < (*length); i++)
	// 	printf("\tT%d", (*stream)[i].type);

	switch (op)
	{
	case ASSIGN:
	{
		if (_op_arg1.type != IdentifierToken && _op_arg1.type != DeclarationToken)
			CompilerError("Assigning to non-identifier");

		str opsize = "qword";
		Identifier dest = _op_arg1.value.__i;
		switch (_op_arg2.type)
		{
		case IdentifierToken:
		{
			fline(&ops, "mov rsi, %s", _val_(_op_arg2));
			fline(&ops, "mov %s [%s], rsi", opsize, dest.name);
			break;
		}
		case LiteralToken:
			fline(&ops, "mov %s [%s], %s", opsize, dest.name, _addr_(_op_arg2));
			break;

		case MemoryToken:
			fline(&ops, "mov [%s], %s", dest.name, _addr_(_op_arg2));
			break;

		default:
			CompilerError("Assignment should be from literal or variable.");
		}
		rippleDeleteTokens(stream, length, index, 3);
		*return_index = index;
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

		(*stream)[index].type = MemoryToken;
		(*stream)[index].value.m = fstr("rdx");

		rippleDeleteTokens(stream, length, index + 1, 2);
		*return_index = index;
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

		(*stream)[index].type = MemoryToken;
		(*stream)[index].value.m = fstr("rdx");

		rippleDeleteTokens(stream, length, index + 1, 2);
		*return_index = index;
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
	while (HEAD->next != NULL)
	{
		HEAD = HEAD->next;
		Token *token = HEAD;

		printf("%d ->\n", token->type);

		if (token->type != LiteralToken)
			continue;

		Literal literal = token->value.__l;
		switch (literal.type)
		{
		case StringValue:
		{
			str str_name = fstr("str%u", istr++);
			fline(&data, "%s: db \"%s\", 0x00", str_name, literal.value.__s);

			// literal.value.__s = malloc(strlen(str_name) * sizeof(char));
			// strcpy(literal.value.__s, str_name);
			literal.value.__s = str_name;
			break;
		}

		case FloatValue:

		default: break;
		}
	}

	// Reserve memory for variables
	// wline(&bss, "section .bss");
	// TokenStream HEAD = _stream_head;
	// while (HEAD != NULL)
	// {
	// 	Token *token = HEAD->next;
	// 	if (token->type == DeclarationToken)
	// 	{
	// 		Identifier *iden = &token->value.__i;

	// 		iden->name = fstr("_var_%s", iden->name);
	// 		fline(&bss, "%s: resb %d", iden->name, iden->msize);
	// 	}
	// 	else if (token->type == ProcedureToken)
	// 	{
	// 		Function *function = &token->value.__f;

	// 		function->name = fstr("_fn_%s", function->name);
	// 		str rt_name = fstr("rtn%s", function->name);

	// 		switch (function->type)
	// 		{
	// 		case IntValue:
	// 			fline(&bss, "%s: resb %d", rt_name, sizeof(__int64_t));
	// 			break;
	// 		case FloatValue:
	// 			fline(&bss, "%s: resb %d", rt_name, sizeof(float));
	// 			break;
	// 		case StringValue:
	// 			fline(&bss, "%s: resb %d", rt_name, sizeof(char *));
	// 			break;
	// 		default:
	// 			break;
	// 		}

	// 		uint params = 0;

	// 		if (stream[++_ti].type == ExpressionStartToken)
	// 		{
	// 			while (stream[_ti].type != ExpressionEndToken)
	// 				if (stream[++_ti].type == DeclarationToken)
	// 					params++;

	// 			function->nargs = params;
	// 		}
	// 	}

	// 	HEAD = HEAD->next;
	// }

	// Iterate through functions
	wline(&text, "section .text");
	wline(&text, "global _start");
	wline(&text, "_start:");

	// Iterate through tokens
	HEAD = _stream_head;
	while (HEAD->next != NULL)
	{
		HEAD = HEAD->next;
		Token *token = HEAD;

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
				while (token->value.__k != END) {
					fline(&text, "mov %s, %s", __linux_syscall_argloc(_syscall_nargs++), _val_(*(token->next)));
					token = token->next;
				}

				// for (size_t i = 0; i < _syscall_nargs; i++)
				// 	fline(&text, "mov %s, %s", __linux_syscall_argloc(i), _val_(token[i + 1]));

				// rippleDeleteTokens(&stream, &length, index, _syscall_nargs + 2);
				// return_index = index;

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
			// fline(&text, resolveOperator(&stream, &length, index, &return_index));
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

		// if (return_index != -1)
		// {
		// 	index = return_index - 1;
		// 	return_index = -1;
		// }
	}

	// return 0;
	// wline(&text, "mov rax, 60");
	// wline(&text, "mov rdi, 0");
	// wline(&text, "syscall");

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