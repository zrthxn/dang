#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum
{
	false,
	true
} bool;
typedef unsigned int uint;
typedef char* str;

#define LF '\n'
#define CR '\r'
#define TAB '\t'
#define SPACE ' '
#define COMMENT '#'

// Stack of module paths being compiled
str* __TARGETS__;

bool isTargetCompiling(str module)
{
	/**
	 * @brief Check if a given module has been included
	 * Will be useful to check for circular dependencies
	 */

	uint __ti = 0;
	while (__TARGETS__[__ti] != NULL)
		if (strcmp(__TARGETS__[__ti++], module) == 0)
			return true;

	return false;
}

void setTargetCompiling(str module)
{
	/**
	 * @brief Set a given module has been included
	 */

	if (isTargetCompiling(module))
	{
		fprintf(stderr, "\nError: Circular dependency, \"%s\" dependends on a module that is using it.\n", module);
		exit(1);
	}

	uint __ti = 0;
	str *targets = malloc(sizeof(str));

	targets[__ti] = malloc(strlen(module) * sizeof(char));
	strcpy(targets[__ti], module);

	while (__TARGETS__[__ti] != NULL)
	{
		targets[__ti + 1] = malloc(strlen(__TARGETS__[__ti]) * sizeof(char));
		strcpy(targets[__ti + 1], __TARGETS__[__ti]);
		free(__TARGETS__[__ti]);
		targets[(++__ti) + 1] = NULL;
	}
	free(__TARGETS__);
	__TARGETS__ = targets;
}

void printall(str* array, size_t size)
{
	for (size_t i = 0; i < size; i++)
		printf("%s\n", array[i]);
}

void CompilerError(str message) {
	fprintf(stderr, "\nError: %s\n", message);
	exit(1);
}

str fstr(str ln, ...) {
	va_list args;
	va_start(args, ln);

	uint _LINE_MAXSIZE = strlen(ln) + 128;
	str line = malloc(_LINE_MAXSIZE * sizeof(char));
	int flen = vsnprintf(line, _LINE_MAXSIZE * sizeof(char), ln, args);

	if (flen > _LINE_MAXSIZE)
		CompilerError("Formattted string too large.");

	va_end(args);

	str new;
	if (flen > 0) {
		new = malloc(flen * sizeof(char));
		strcpy(new, line);
	}

	return new;
}

// --------------------------
// Lexer --------------------

void readTargetFile(str filename, str* buffer, uint* size)
{
	/**
	 * @brief Read file into buffer as string
	 */

	FILE* f_ptr = fopen(filename, "r");

	if (f_ptr == NULL)
		CompilerError(fstr("Couldn't open file \"%s\"", filename));

	// Find size of file
	fseek(f_ptr, 0L, SEEK_END);
	*size = ftell(f_ptr);
	fseek(f_ptr, 0L, SEEK_SET);

	// Allocate that many bytes and read
	*buffer = malloc((*size) * sizeof(char));

	uint index = 0;
	while (index < *size)
	{
		char c = fgetc(f_ptr);
		if (c != EOF)
			(*buffer)[index++] = c;
		else
		{
			if (index != *size)
				fprintf(stderr, "\nEncountered EOF after %u bytes.\n", index);
			break;
		}
	}

	fclose(f_ptr);
}

void lex(const str filename, str** lexicon, uint* lexsize)
{
	/**
	 * @brief Lex file into logical words
	 */

	str buffer;
	uint size;

	readTargetFile(filename, &buffer, &size);
	printf("\b\b\b, %d bytes.\n", size);

	str* __lexicon;
	size_t length = 0;
	str current = "";
	uint __ci = 0;

	while (size)
	{
		const char c = *buffer;

		if (
				// Add new lex on LF, CR, TAB and SPACE,
				(c != LF && c != CR && c != TAB && c != SPACE) ||

				// unless first char of current word is quote
				// and the quote isnt closed yet; for strings
				(current[0] == '\"' && (__ci > 0 ? current[__ci - 1] != '\"' : true)) ||

				// Unless handling comments: ignore line until you see LF or CR
				(current[0] == COMMENT && c != LF && c != CR))
		{
			char new[++__ci];
			for (size_t i = 0; i < __ci - 1; i++)
				new[i] = current[i];

			new[__ci - 1] = c;
			new[__ci] = '\0';

			current = malloc((__ci + 1) * sizeof(char));
			strcpy(current, new);
		}
		/**
		 * @todo add conditions for condensed expressions
		 * like `var bye:int = (23+45)*3/4`
		 */
		// Time to add a new lex
		else if (strcmp(current, "") != 0)
		{
			if (current[0] == COMMENT)
			{
				current = malloc(sizeof(char));
				strcpy(current, "");
				__ci = 0;
				continue;
			}

			str __lexes[++length];
			// Save previous entries
			if (length > 1)
				for (size_t i = 0; i < length - 1; i++)
				{
					__lexes[i] = malloc(strlen(__lexicon[i]) * sizeof(char));
					strcpy(__lexes[i], __lexicon[i]);
				}

			// Add new entry
			__lexes[length - 1] = malloc((__ci + 1) * sizeof(char));
			strcpy(__lexes[length - 1], current);

			// Copy all to new location
			__lexicon = malloc((length) * sizeof(str));
			for (size_t i = 0; i < length; i++)
			{
				__lexicon[i] = malloc(strlen(__lexes[i]) * sizeof(char));
				strcpy(__lexicon[i], __lexes[i]);
			}

			current = malloc(sizeof(char));
			strcpy(current, "");
			__ci = 0;
		}

		buffer = &buffer[1];
		size--;
	}

	*lexsize = length;
	*lexicon = malloc((*lexsize) * sizeof(str *));
	for (size_t i = 0; i < *lexsize; i++)
	{
		(*lexicon)[i] = malloc(strlen(__lexicon[i]) * sizeof(char));
		strcpy((*lexicon)[i], __lexicon[i]);
	}
}

// --------------------------
// Parser -------------------

typedef enum
{
	_K = 100,
	VAR,
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
	__KEYWORDS_COUNT,
} Keyword;

typedef enum
{
	_O = __KEYWORDS_COUNT,
	// Binary Ops
	ADD, // +
	SUB, // -
	MUL, // *
	DIV, // /
	// MOD, // %

	ASSIGN, // =
	CALL,		// <|

	LOGICAL_GREATER_THAN, // >
	LOGICAL_LESS_THAN,		// <
	LOGICAL_EQUAL,				// ==
	LOGICAL_NOT_EQUAL,		// !=
	LOGICAL_AND,					// &&
	LOGICAL_OR,						// ||
	LOGICAL_XOR,					// ^^

	BIT_SHIFT_LEFT,	 // <<
	BIT_SHIFT_RIGHT, // >>

	BIT_AND, // &
	BIT_OR,	 // |
	BIT_XOR, // ^

	// Unary Ops
	BIT_NOT,		 // !
	LOGICAL_NOT, // !!

	// INCREMENT, // ++
	// DECREMENT, // --
	// DEREF, // *
	// ACCESSOR, // .
	__OPERATIONS_COUNT,
} Operator;

typedef enum
{
	_V = __OPERATIONS_COUNT,
	IntValue,
	FloatValue,
	StringValue,
	NullValue,
	__VALUETYPES_COUNT,
} ValueType;

typedef union
{
	int __i;
	float __f;
	str __s;
	void* n;
} LiteralValue;

typedef struct
{
	ValueType type;
	LiteralValue value;
	uint msize;
} Literal;

typedef struct
{
	str name;
	ValueType type;
	uint msize;
} Identifier;

typedef struct
{
	str name;
	ValueType type;
	uint nargs;
} Function;

// Tokenization Types ----
typedef enum
{
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

typedef union
{
	Keyword __k;
	Operator __o;
	Identifier __i;
	Function __f;
	Literal __l;
	str m;
	void* n;
} TokenValue;

typedef struct
{
	TokenType type;
	TokenValue value;
} Token;

typedef Token* TokenStream;

void pushToken(TokenStream* stream, uint* len)
{
	TokenStream __new = malloc((++(*len)) * sizeof(Token));
	if ((*len) > 1)
		for (size_t i = 0; i < (*len) - 1; i++)
			__new[i] = (*stream)[i];

	// memcpy(&__new[(*len)-1], el, sizeof(Token));
	__new[(*len) - 1] = (Token){};
	*stream = __new;
}

Token* popToken(TokenStream stream, uint* len)
{
	return &stream[--(*len)];
}

void rippleDeleteTokens(TokenStream* stream, uint* len, uint index, uint count)
{
	for (size_t i = index; i < *len; i++) {
		if (i+count < *len)
			memcpy(&((*stream)[i]), &((*stream)[i+count]), sizeof(Token));
		else
			(*stream)[i] = (Token){};
	}
	*len -= count;
}

bool isInteger(str word)
{
	uint __i = 0;
	if (word[0] == '-')
		__i++;

	while (word[__i] != '\0')
		if (!isdigit(word[__i++]))
			return false;

	// str INT64_MAX_STR = "9223372036854775807";
	// for (size_t i = 0; i < __i; i++) {
	// 	if (INT64_MAX_STR[i] == '\0' || word[i] > INT64_MAX_STR[i]) {
	// 		fprintf(stderr, "\nError: Integer \"%s\" larger than 64-bits.\n", word);
	// 		exit(1);
	// 	}
	// }

	return true;
}

bool isFloat(str word)
{
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

void parse(const str filename, TokenStream* stream, uint* parselen)
{
	/**
	 * @brief Parse words into token stream
	 */

	uint lexsize = 0;
	str* lexicon;
	lex(filename, &lexicon, &lexsize);

	// Local token stream
	TokenStream tstream;
	// Length of token stream
	uint length = 0;

	while (lexsize)
	{
		const str word = *lexicon;
		const uint len = strlen(word);

		if /* Handle comments */ (word[0] == COMMENT)
		{
			// Ignore them for now
		}
		else if /* Delimiter */ (strcmp(word, ";") == 0)
		{
			// Ignore for now, auto splitting
		}
		else if /* Expression Start */ (
				strcmp(word, "[") == 0 || strcmp(word, "{") == 0 || strcmp(word, "(") == 0)
		{
			// check previous
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = ExpressionStartToken,
					.value = (TokenValue)NULL,
			};
		}
		else if /* Expression End */ (
				strcmp(word, "]") == 0 || strcmp(word, "}") == 0 || strcmp(word, ")") == 0)
		{
			// check previous
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = ExpressionEndToken,
					.value = (TokenValue)NULL,
			};
		}
		// Keywords ---------------------------------------------------------------
		else if /* Include other files */ (strcmp(word, "include") == 0)
		{
			lexicon = &lexicon[1];
			lexsize--;

			str arg = *lexicon;
			setTargetCompiling(arg);
			parse(arg, &tstream, &length);
		}
		else if /* End */ (strcmp(word, "while") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)WHILE,
			};
		}
		else if /* End */ (strcmp(word, "if") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)IF,
			};
		}
		else if /* End */ (strcmp(word, "then") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)THEN,
			};
		}
		else if /* End */ (strcmp(word, "elif") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)ELIF,
			};
		}
		else if /* Else */ (strcmp(word, "else") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)ELSE,
			};
		}
		else if /* Do */ (strcmp(word, "do") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)DO,
			};
		}
		else if /* End */ (strcmp(word, "end") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)END,
			};
		}
		else if /* Syscall */ (strcmp(word, "return") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)RETURN,
			};
		}
		else if /* Syscall */ (strcmp(word, "syscall") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = KeywordToken,
					.value = (TokenValue)(Keyword)SYSCALL,
			};
		}
		// Declarations -----------------------------------------------------------
		else if /* Variable declaration */ (strcmp(word, "var") == 0)
		{
			lexicon = &lexicon[1];
			lexsize--;

			str arg = *lexicon;
			uint split = strcspn(arg, ":");
			// Type of variable not given
			if (split == strlen(arg))
				CompilerError(fstr("Untyped variable \"%s\" not supported yet.", arg));

			arg[split++] = '\0';
			str type = &arg[split];

			pushToken(&tstream, &length);

			tstream[length - 1] = (Token){
					.type = DeclarationToken,
					.value = (TokenValue)(Identifier){
							.name = arg,
							.msize = 0,
							.type = 0,
					}};

			strcpy(tstream[length - 1].value.__i.name, arg);

			if (strcmp(type, "int") == 0)
			{
				tstream[length - 1].value.__i.type = IntValue;
				tstream[length - 1].value.__i.msize = sizeof(__int64_t);
			}
			else if (strcmp(type, "float") == 0)
			{
				tstream[length - 1].value.__i.type = FloatValue;
				tstream[length - 1].value.__i.msize = sizeof(float);
			}
			else if (strcmp(type, "str") == 0)
			{
				tstream[length - 1].value.__i.type = StringValue;
				tstream[length - 1].value.__i.msize = sizeof(char*);
			}
		}
		else if /* Function Declarations */ (strcmp(word, "fn") == 0)
		{
			lexicon = &lexicon[1];
			lexsize--;

			str arg = *lexicon;
			uint split = strcspn(arg, ":");
			if (split == strlen(arg))
				CompilerError(fstr("Untyped function \"%s\" not supported yet.", arg));

			arg[split++] = '\0';
			str type = &arg[split];

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = ProcedureToken,
					.value = (TokenValue)(Function){
							.name = arg,
							.nargs = 0,
							.type = 0,
					}};

			strcpy(tstream[length - 1].value.__f.name, arg);

			if (strcmp(type, "int") == 0)
			{
				tstream[length - 1].value.__f.type = IntValue;
				tstream[length - 1].value.__i.msize = sizeof(__int64_t);
			}
			else if (strcmp(type, "float") == 0)
			{
				tstream[length - 1].value.__f.type = FloatValue;
				tstream[length - 1].value.__i.msize = sizeof(float);
			}
			else if (strcmp(type, "str") == 0)
			{
				tstream[length - 1].value.__f.type = StringValue;
				tstream[length - 1].value.__i.msize = sizeof(char*);
			}

			// add n args
		}
		// Operations -------------------------------------------------------------
		else if /* Assignment Operation */ (strcmp(word, "=") == 0)
		{
			Token* prev = popToken(tstream, &length);

			if (prev->type != DeclarationToken && prev->type != IdentifierToken)
				CompilerError("Assigning to non-identifier.");

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)ASSIGN,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length - 1], prev, sizeof(Token));
		}
		else if /* Addition Operation */ (strcmp(word, "+") == 0)
		{
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)ADD,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length - 1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Subtraction Operation */ (strcmp(word, "-") == 0)
		{
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)SUB,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length - 1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Multiplication Operation */ (strcmp(word, "*") == 0)
		{
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)MUL,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length - 1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Division Operation */ (strcmp(word, "/") == 0)
		{
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)DIV,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length - 1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Logical AND Operation */ (strcmp(word, "&&") == 0)
		{
			Token* prev = popToken(tstream, &length);
			// check if prev is a function identifier

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)LOGICAL_AND,
			};
		}
		else if /* Call Operation */ (strcmp(word, "<|") == 0)
		{
			Token* prev = popToken(tstream, &length);

			// check if prev is a function identifier
			bool found = false;
			for (size_t i = 0; i < length; i++)
			{
				if (
						tstream[i].type == ProcedureToken &&
						strcmp(tstream[i].value.__f.name, prev->value.__f.name) == 0)
				{
					found = true;
				}
			}

			if (!found)
				CompilerError(fstr("Use of un-declared function \"%s\".", prev->value.__f.name));

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = OperatorToken,
					.value = (TokenValue)(Operator)CALL,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length - 1], prev, sizeof(Token));
		}
		// Literals ---------------------------------------------------------------
		else if /* Int Literals */ (isInteger(word))
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = LiteralToken,
					.value = (TokenValue)(Literal){
							.type = IntValue,
							.msize = sizeof(int),
							.value = (LiteralValue)(int)atoi(word),
					}};
		}
		else if /* String Literals */ (word[0] == '\"' && word[len - 1] == '\"')
		{
			str value = &word[1];
			value[len - 2] = '\0';

			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = LiteralToken,
					.value = (TokenValue)(Literal){
							.type = StringValue,
							.msize = (len - 2) * sizeof(char),
							.value = (LiteralValue)(str)value,
					}};
			strcpy(tstream[length - 1].value.__l.value.__s, value);
		}
		else if /* Float Literals */ (isFloat(word))
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = LiteralToken,
					.value = (TokenValue)(Literal){
							.type = FloatValue,
							.msize = sizeof(float),
							.value = (LiteralValue)(float)atof(word),
					}};
		}
		else if /* NULL Literals */ (strcmp(word, "null") == 0)
		{
			pushToken(&tstream, &length);
			tstream[length - 1] = (Token){
					.type = LiteralToken,
					.value = (TokenValue)(Literal){
							.type = NullValue,
							.msize = sizeof(uint *),
							.value = (LiteralValue)(int)0,
					}};
		}
		// Word or Identifier -----------------------------------------------------
		else if (isalpha(word[0]) && strlen(word) <= 32)
		{
			bool found = false;
			for (size_t i = 0; i < length; i++)
			{
				if (
						tstream[i].type == DeclarationToken &&
						strcmp(tstream[i].value.__i.name, word) == 0)
				{
					found = true;
					pushToken(&tstream, &length);
					tstream[length - 1] = (Token){
							.type = IdentifierToken,
							.value = (TokenValue)tstream[i].value};
					memcpy(&tstream[length - 1].value, &tstream[i].value, sizeof(TokenValue));
				}
			}

			if (!found)
				CompilerError(fstr("Un-declared identifier \"%s\".", word));
		}

		else /* Something unrecognised was thrown own way */
			CompilerError(fstr("Unknown word \"%s\".", word));

		// Advance to next lex
		lexicon = &lexicon[1];
		lexsize--;
	}

	*parselen += length;
	*stream = malloc((*parselen) * sizeof(Token));

	if ((*parselen) > 1)
		for (size_t i = 0; i < (*parselen) - 1; i++)
			(*stream)[i] = tstream[i];

	*stream = tstream;
}

// --------------------------
// Targets ------------------

void wline(str* buf, str ln)
{
	uint _size = strlen(*buf) + strlen(ln);
	_size++;

	str new = malloc(_size * sizeof(char));
	strcpy(new, *buf);
	new[strlen(*buf)] = '\n';

	strcpy(&new[strlen(*buf) + 1], ln);
	new[_size] = '\0';
	*buf = new;
}

void fline(str* buf, str ln, ...) {
	va_list args;
	va_start(args, ln);

	uint _LINE_MAXSIZE = strlen(ln) + 128;
	str line = malloc(_LINE_MAXSIZE * sizeof(char));
	int flen = vsnprintf(line, _LINE_MAXSIZE * sizeof(char), ln, args);

	if (flen > _LINE_MAXSIZE)
		CompilerError("Formattted string too large.");

	va_end(args);

	uint _size = strlen(*buf);
	if (flen > 0)
		_size += flen;
	_size++;

	str new = malloc(_size * sizeof(char));
	strcpy(new, *buf);
	new[strlen(*buf)] = '\n';

	strcpy(&new[strlen(*buf) + 1], line);
	new[_size] = '\0';
	*buf = new;
}

str __linux_syscall_argloc(uint narg) {
	switch (narg)
	{
	case 0: return "rax";
	case 1: return "rdi";
	case 2: return "rsi";
	case 3: return "rdx";
	case 4: return "r10";
	case 5: return "r8";
	case 6: return "r9";

	default: return "";
	}
}

str _val_(Token token) {
	switch (token.type)
	{
		case DeclarationToken: return fstr("[%s]", token.value.__i.name);
		case IdentifierToken: return fstr("[_var_%s]", token.value.__i.name);
		
		case MemoryToken: return token.value.m;
		
		case LiteralToken: {
			switch (token.value.__l.type)
			{
				case FloatValue:
				case StringValue: 
					return fstr("[%s]", token.value.__l.value.__s);
				
				case IntValue:
					return fstr("%d", token.value.__l.value.__i);

				default: return "";
			}
		}
		
		default: return "";
	}
}

str _addr_(Token token) {
	switch (token.type)
	{
		case DeclarationToken: return fstr("%s", token.value.__i.name);
		case IdentifierToken: return fstr("_var_%s", token.value.__i.name);
		
		case MemoryToken: return token.value.m;
		
		case LiteralToken: {
			switch (token.value.__l.type)
			{
				case FloatValue:
				case StringValue: 
					return token.value.__l.value.__s;

				case IntValue:
					return fstr("%d", token.value.__l.value.__i);

				default: return "";
			}
		}
		
		default: return "";
	}
}

str resolveOperator(TokenStream* stream, uint* length, uint index, size_t* return_index) 
{
	Operator op = (*stream)[index].value.__o;
	str ops = "";

	if ((*stream)[index + 1].type == OperatorToken)
		wline(&ops, resolveOperator(stream, length, index + 1, return_index));

	if ((*stream)[index + 2].type == OperatorToken)
		wline(&ops, resolveOperator(stream, length, index + 2, return_index));

	Token _op_arg1 = (*stream)[index + 1];
	Token _op_arg2 = (*stream)[index + 2];

	// printf("\n%d", index);
	// printf("\n");
	// for (size_t i = 0; i < (*length); i++)
	// {
	// 	printf("\tT%d", (*stream)[i].type);
	// }
	
	switch (op)
	{
		case ASSIGN: {
			if (_op_arg1.type != IdentifierToken && _op_arg1.type != DeclarationToken)
				CompilerError("Assigning to non-identifier");
			
			str opsize = "qword";
			Identifier dest = _op_arg1.value.__i;
			switch (_op_arg2.type)
			{
				case IdentifierToken:
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

		case ADD: {
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

		case SUB: {
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
		
		default:
			break;
	}

	return ops;
}

void clean_codegen(str* code) {
	for (size_t i = 0; i < strlen((*code)); i++)
		if (isalnum((*code)[i]) == 0 && ispunct((*code)[i]) == 0 && isspace((*code)[i]) == 0)
			(*code)[i] = ' ';
}

void codegen_x86_64(TokenStream stream, uint length, str outfile)
{
	FILE* fout = fopen(outfile, "w");

	if (fout == NULL)
		CompilerError(fstr("Couldn't create \"%s\".", outfile));

	str
		head,
		text = "",
		data = "",
		bss = "";

	// Header
	fline(&head, "BITS %d\n", 64);
	
	// Pre-allocate addresses for literals
	wline(&data, "section .data");
	uint istr = 0, iflt = 0;
	for (size_t _ti = 0; _ti < length; _ti++)
	{
		Token* token = &stream[_ti];
		if (token->type != LiteralToken)
			continue;

		Literal* literal = &token->value.__l;
		switch (literal->type)
		{
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
	wline(&bss, "section .bss");
	for (size_t _ti = 0; _ti < length; _ti++)
	{
		Token* token = &stream[_ti];
		if (token->type == DeclarationToken) {
			Identifier* iden = &token->value.__i;

			str ptr_name = fstr("_var_%s", iden->name);
			fline(&bss, "%s: resb %d", ptr_name, iden->msize);

			iden->name = malloc(strlen(ptr_name) * sizeof(char));
			strcpy(iden->name, ptr_name);
		}
		else if (token->type == ProcedureToken) {
			Function* function = &token->value.__f;
			
			str fn_name = fstr("_fn_%s", function->name);
			str rt_name = fstr("rtn%s", fn_name);

			switch (function->type)
			{
			case IntValue:
				fline(&bss, "%s: resb %d", rt_name, sizeof(__int64_t));
				break;
			case FloatValue:
				fline(&bss, "%s: resb %d", rt_name, sizeof(float));
				break;
			case StringValue:
				fline(&bss, "%s: resb %d", rt_name, sizeof(char*));
				break;
			default: break;
			}
			
			function->name = malloc(strlen(fn_name) * sizeof(char));
			strcpy(function->name, fn_name);
		}

	}

	// Iterate through functions
	wline(&text, "section .text");
	wline(&text, "global _start");
	wline(&text, "_start:");

	// Iterate through tokens
	size_t return_index = -1;
	for (size_t index = 0; index < length; index++)
	{
		Token *token = &(stream[index]);

		switch (token->type)
		{
			case KeywordToken: {
				Keyword value = token->value.__k;
				switch (value)
				{
					case SYSCALL: {
						uint _syscall_nargs = 0;
						while (token[_syscall_nargs + 1].value.__k != END)
							_syscall_nargs++;

						for (size_t i = 0; i < _syscall_nargs; i++)
							fline(&text, "mov %s, %s", __linux_syscall_argloc(i), _val_(token[i+1]));

						wline(&text, "syscall");
						rippleDeleteTokens(&stream, &length, index, _syscall_nargs + 2);
						return_index = index;
						}
						break;

					default:
						break;
				}
				}
				break;

			case DeclarationToken:
				/* code */
				break;

			case OperatorToken:
				fline(&text, resolveOperator(&stream, &length, index, &return_index));
				break;

			case LiteralToken:
				fline(&text, "mov rax, %s", _val_(*token));
				break;


			case ExpressionStartToken:
				/* code */
				break;

			case ExpressionEndToken:
				/* code */
				break;
		}

		if (return_index != -1) {
			index = return_index - 1;
			return_index = -1;
		}
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

// --------------------------
// Main ---------------------

bool isTargetFile(str arg)
{
	if (strlen(arg) < 5)
		return false;

	str ending = &arg[strlen(arg) - 5];
	if (strcmp(ending, ".dang") == 0)
		return true;
	else
		return false;
}

bool isCompilerFlag(str arg)
{
	if (strlen(arg) < 2)
		return false;

	if (arg[0] == '-')
		return true;
	else
		return false;
}

enum _ARCH_ 
{
	x86_64,
};

void compileTarget(const str filename, const str outfile, enum _ARCH_ arch)
{
	/**
	 * @brief Generate code corresponding to stream
	 */

	printf("[INFO] Compiling target %s...", filename);
	fflush(stdout);

	// lex and parse
	Token *stream;
	uint length = 0;

	setTargetCompiling(filename);
	parse(filename, &stream, &length);

	switch (arch)
	{
	case x86_64:
		codegen_x86_64(stream, length, outfile);
		break;
	
	default:
		break;
	}
}

int main(int argc, str argv[])
{
	// Ignore name of binary
	argv = &argv[1];
	argc--;

	str *targets = malloc(argc * sizeof(str));
	str *cflags = malloc(argc * sizeof(str));
	int n_targets = 0, n_cflags = 0;

	int __c = argc;
	while (argc)
	{
		str arg = argv[--argc];
		if (isTargetFile(arg))
			targets[n_targets++] = arg;
		else if (isCompilerFlag(arg))
			cflags[n_cflags++] = arg;
		else
			printf("[INFO] Ignoring unknown argument \"%s\"\n", arg);
	}

	for (size_t i = n_targets; i < __c; i++)
		free(targets[i]);
	for (size_t i = n_cflags; i < __c; i++)
		free(cflags[i]);

	// Initialize list of targets
	__TARGETS__ = malloc(sizeof(str));
	__TARGETS__[0] = NULL;

	while (n_targets)
	{
		const str target = *targets;

		uint len = strcspn(target, ".");
		char name[len];
		for (size_t i = 0; i < len; i++)
			name[i] = target[i];
		name[len] = '\0';

		/**
		 * @brief Compilation
		 * 1. Read file into buffer as string
		 * 2. Lex file into logical words
		 * 3. Parse words into token stream
		 * 4. Generate code corresponding to stream
		 * 5. Pop target from targets array
		 */

		str ASM = fstr("%s.asm", name);
		str OBJ = fstr("%s.o", name);

		compileTarget(target, ASM, x86_64);

		system(fstr("nasm -felf64 %s", ASM));
		system(fstr("ld -o %s %s", name, OBJ));

		// system(fstr("rm %s %s", ASM, OBJ));

		// Shift base pointer ahead
		targets = &targets[1];
		n_targets--;
	}

	return 0;
}
