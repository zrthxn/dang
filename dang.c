#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum { false, true } bool;
typedef unsigned int uint;
typedef char* str;

#define LF      '\n'
#define CR      '\r'
#define TAB	    '\t'
#define SPACE   ' '
#define COMMENT '#'

void printall(str* array, size_t size) {
	for (size_t i = 0; i < size; i++)
		printf("%s\n", array[i]);
}

// --------------------------
// Lexer --------------------

void readTargetFile(str filename, str* buffer, uint* size) {
	/**
	 * @brief Read file into buffer as string
	 */

	FILE* f_ptr = fopen(filename, "r");

	if (f_ptr == NULL) {
		fprintf(stderr, "\nError: Couldn't open file \"%s\"\n", filename);
		exit(1);
	}

	// Find size of file
	fseek(f_ptr, 0L, SEEK_END);
	*size = ftell(f_ptr);
	fseek(f_ptr, 0L, SEEK_SET);

	// Allocate that many bytes and read
	*buffer = malloc((*size) * sizeof(char));

	uint index = 0;
	while (index < *size) {
		char c = fgetc(f_ptr);
		if (c != EOF) 
			(*buffer)[index++] = c;
		else {
			if (index != *size)
				fprintf(stderr, "\nEncountered EOF after %u bytes.\n", index);
			break;
		}
	}

	fclose(f_ptr);
}

void lex(const str filename, str** lexicon, uint* lexsize) {
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
			(current[0] == '\"' && (__ci > 0 ? current[__ci-1] != '\"' : true)) ||

			// Unless handling comments: ignore line until you see LF or CR
			(current[0] == COMMENT && c != LF && c != CR)
		)
		{
			char new[++__ci];
			for (size_t i = 0; i < __ci-1; i++)
				new[i] = current[i];

			new[__ci-1] = c; new[__ci] = '\0';
			
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
			if (current[0] == COMMENT) {
				current = malloc(sizeof(char));
				strcpy(current, "");
				__ci = 0;
				continue;
			}

			str __lexes[++length];
			// Save previous entries
			if (length > 1) for (size_t i = 0; i < length-1; i++) {
				__lexes[i] = malloc(strlen(__lexicon[i]) * sizeof(char));
				strcpy(__lexes[i], __lexicon[i]);
			}

			// Add new entry
			__lexes[length-1] = malloc((__ci + 1) * sizeof(char));
			strcpy(__lexes[length-1], current);

			// Copy all to new location
			__lexicon = malloc((length) * sizeof(str));
			for (size_t i = 0; i < length; i++) {
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
	*lexicon = malloc((*lexsize) * sizeof(str*));
	for (size_t i = 0; i < *lexsize; i++) 
	{
		(*lexicon)[i] = malloc(strlen(__lexicon[i]) * sizeof(char));
		strcpy((*lexicon)[i], __lexicon[i]);
	}
}

// --------------------------
// Parser -------------------

typedef 
	enum { _ = 100,
		VAR,
		FN,
		IF,
		THEN,
		ELIF,
		ELSE,
		END,
		WHILE,
		DO,
		INCLUDE,
		__KEYWORDS_COUNT,
	} 
Keyword;

typedef 
	enum { __ = __KEYWORDS_COUNT,
		// Binary Ops
		ADD, // +
		SUB, // -
		MUL, // *
		DIV, // /
		MOD, // %

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
	} 
Operator;

typedef 
	enum { ___ = __OPERATIONS_COUNT,
		IntValue,
		FloatValue,
		StringValue,
		NullValue,
		__VALUETYPES_COUNT,
	} 
ValueType;

typedef 
	union {
		int __i;
		float __f;
		str __s;
		void* n;
	} 
LiteralValue;

typedef 
	struct {
		ValueType type;
		LiteralValue value;
		uint msize; 
	}
Literal;

typedef 
	struct {
		str name;
		ValueType type;
		uint msize;
	} 
Identifier;

typedef 
	struct {
		str name;
		ValueType type;
		uint nargs;
	} 
Function;

// Tokenization Types ----
typedef 
	enum { ____ = __VALUETYPES_COUNT,
		KeywordToken,
		LiteralToken,
		DeclarationToken,
		IdentifierToken,
		OperatorToken,
		ExpressionStartToken,
		ExpressionEndToken,
		__TOKENTYPES_COUNT
	} 
TokenType;

typedef 
	union {
		Keyword __k;
		Operator __o;
		Identifier __i;
		Function __f;
		Literal __l;
		void* n;
	} 
TokenValue;

typedef 
	struct {
		TokenType type;
		TokenValue value;
	} 
Token;

typedef Token* TokenStream;

void pushToken(TokenStream* stream, uint* len) {
	TokenStream __new = malloc((++(*len)) * sizeof(Token));
	if ((*len) > 1) for (size_t i = 0; i < (*len)-1; i++)
		__new[i] = (*stream)[i];

	// memcpy(&__new[(*len)-1], el, sizeof(Token));
	__new[(*len)-1] = (Token){};
	*stream = __new;
}

Token* popToken(TokenStream stream, uint* len) {
	return &stream[--(*len)];
}

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
	// 		fprintf(stderr, "\nError: Integer \"%s\" larger than 64-bits.\n", word);
	// 		exit(1);
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

		if /* Include other files */ (strcmp(word, "include") == 0) {
			lexicon = &lexicon[1];
			lexsize--;

			str arg = *lexicon;
			// parse( next word as filepath )
		}
		else if /* Handle comments */ (word[0] == COMMENT) {
			// Ignore them for now
		}
		else if /* Delimiter */ (strcmp(word, ";") == 0) {
			// Ignore for now, auto splitting
		}
		else if /* Expression Start */ (
			strcmp(word,  "[") == 0 || strcmp(word,  "{") == 0 || strcmp(word,  "(") == 0
		) {
			// check previous
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = ExpressionStartToken,
				.value = (TokenValue)NULL,
			};
		}
		else if /* Expression End */ (
			strcmp(word,  "]") == 0 || strcmp(word,  "}") == 0 || strcmp(word,  ")") == 0
		) {
			// check previous
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = ExpressionEndToken,
				.value = (TokenValue)NULL,
			};
		}
		// Keywords ---------------------------------------------------------------
		else if /* End */ (strcmp(word, "while") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = KeywordToken,
				.value = (TokenValue)(Keyword)WHILE,
			};
		}
		else if /* End */ (strcmp(word, "if") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = KeywordToken,
				.value = (TokenValue)(Keyword)IF,
			};
		}
		else if /* End */ (strcmp(word, "then") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = KeywordToken,
				.value = (TokenValue)(Keyword)THEN,
			};
		}
		else if /* End */ (strcmp(word, "elif") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = KeywordToken,
				.value = (TokenValue)(Keyword)ELIF,
			};
		}
		else if /* End */ (strcmp(word, "do") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = KeywordToken,
				.value = (TokenValue)(Keyword)DO,
			};
		}
		else if /* End */ (strcmp(word, "end") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = KeywordToken,
				.value = (TokenValue)(Keyword)END,
			};
		}
		// Declarations -----------------------------------------------------------
		else if /* Variable declaration */ (strcmp(word, "var") == 0) {
			lexicon = &lexicon[1];
			lexsize--;

			str arg = *lexicon;
			uint split = strcspn(arg, ":");
			if (split == strlen(arg)) { // Type of variable not given 
				fprintf(stderr, "\nError: Untyped variable \"%s\" not supported yet.\n", arg);
				exit(1);
			}

			arg[split++] = '\0';
			str type = &arg[split];

			pushToken(&tstream, &length);

			tstream[length-1] = (Token){
				.type = DeclarationToken,
				.value = (TokenValue)(Identifier){
					.name = arg,
					.msize = 0,
					.type = 0,
				}
			};

			strcpy(tstream[length-1].value.__i.name, arg);

			if (strcmp(type, "int") == 0) {
				tstream[length-1].value.__i.type = IntValue;
				tstream[length-1].value.__i.msize = sizeof(__int64_t);
			} else if (strcmp(type, "float") == 0) {
				tstream[length-1].value.__i.type = FloatValue;
				tstream[length-1].value.__i.msize = sizeof(float);
			} else if (strcmp(type, "str") == 0) {
				tstream[length-1].value.__i.type = StringValue;
				tstream[length-1].value.__i.msize = sizeof(char*);
			}
		}
		else if /* Function Declarations */ (strcmp(word, "fn") == 0) {
			lexicon = &lexicon[1];
			lexsize--;

			str arg = *lexicon;
			uint split = strcspn(arg, ":");
			if (split == strlen(arg)) { // Type of function not given 
				fprintf(stderr, "\nError: Untyped function \"%s\" not supported yet.\n", arg);
				exit(1);
			}

			arg[split++] = '\0';
			str type = &arg[split];

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = DeclarationToken,
				.value = (TokenValue)(Function){
					.name = arg,
					.nargs = 0,
					.type = 0,
				}
			};

			strcpy(tstream[length-1].value.__i.name, arg);

			if (strcmp(type, "int") == 0) {
				tstream[length-1].value.__i.type = IntValue;
				tstream[length-1].value.__i.msize = sizeof(__int64_t);
			} else if (strcmp(type, "float") == 0) {
				tstream[length-1].value.__i.type = FloatValue;
				tstream[length-1].value.__i.msize = sizeof(float);
			} else if (strcmp(type, "str") == 0) {
				tstream[length-1].value.__i.type = StringValue;
				tstream[length-1].value.__i.msize = sizeof(char*);
			}

			// add n args
		}
		// Operations -------------------------------------------------------------
		else if /* Assignment Operation */ (strcmp(word, "=") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)ASSIGN,
			};
		}
		else if /* Addition Operation */ (strcmp(word, "+") == 0) {
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)ADD,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length-1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Subtraction Operation */ (strcmp(word, "-") == 0) {
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)SUB,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length-1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Multiplication Operation */ (strcmp(word, "*") == 0) {
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)MUL,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length-1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Division Operation */ (strcmp(word, "/") == 0) {
			Token* prev = popToken(tstream, &length);

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)DIV,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length-1], prev, sizeof(Token));
			// tstream[length-1] = *prev;
		}
		else if /* Logical AND Operation */ (strcmp(word, "&&") == 0) {
			Token* prev = popToken(tstream, &length);
			// check if prev is a function identifier

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)LOGICAL_AND,
			};
		}
		else if /* Call Operation */ (strcmp(word, "<|") == 0) {
			Token* prev = popToken(tstream, &length);

			// check if prev is a function identifier
			bool found = false;
			for (size_t i = 0; i < length; i++) {
				if (
					tstream[i].type == DeclarationToken && 
					strcmp(tstream[i].value.__f.name, prev->value.__f.name) == 0
				) {
					found = true;
				}
			}
			
			if (!found) {
				fprintf(stderr, "\nError: Use of un-declared function \"%s\"\n", prev->value.__f.name);
				exit(1);
			}

			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = OperatorToken,
				.value = (TokenValue)(Operator)CALL,
			};

			pushToken(&tstream, &length);
			memcpy(&tstream[length-1], prev, sizeof(Token));
		}
		// Literals ---------------------------------------------------------------
		else if /* Int Literals */ (isInteger(word)) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = LiteralToken,
				.value = (TokenValue)(Literal){
					.type = IntValue,
					.msize = sizeof(int),
					.value = (LiteralValue)(int)atoi(word)
				}
			};
		}
		else if /* String Literals */ (word[0] == '\"' && word[len-1] == '\"' ) {
			str value = &word[1];
			value[len-2] = '\0';
			
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = LiteralToken,
				.value = (TokenValue)(Literal){
					.type = StringValue,
					.msize = (len-2) * sizeof(char),
					.value = (LiteralValue)(str)value,
				}
			};
			strcpy(tstream[length-1].value.__l.value.__s, value);
		}
		else if /* Float Literals */ (isFloat(word)) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = LiteralToken,
				.value = (TokenValue)(Literal){
					.type = FloatValue,
					.msize = sizeof(float),
					.value = (LiteralValue)(float)atof(word),
				}
			};
		}
		else if /* NULL Literals */ (strcmp(word, "null") == 0) {
			pushToken(&tstream, &length);
			tstream[length-1] = (Token){
				.type = LiteralToken,
				.value = (TokenValue)(Literal){
					.type = NullValue,
					.msize = sizeof(uint*),
					.value = (LiteralValue)(int)0,
				}
			};
		}
		// Word or Identifier -----------------------------------------------------
		else if (isalpha(word[0]) && strlen(word) <= 32) {
			bool found = false;
			for (size_t i = 0; i < length; i++) {
				if (
					tstream[i].type == DeclarationToken && 
					strcmp(tstream[i].value.__i.name, word) == 0
				) {
					found = true;
					pushToken(&tstream, &length);
					tstream[length-1] = (Token){
						.type = IdentifierToken,
						.value = (TokenValue)tstream[i].value
					};
				}
			}
			
			if (!found) {
				fprintf(stderr, "\nError: Un-declared identifier \"%s\"\n", word);
				exit(1);
			}
		} 
		
		else /* Something unrecognised was thrown own way */
		{
			fprintf(stderr, "\nError: Unknown word \"%s\"\n", word);
			exit(1);
		}

		// Advance to next lex
		lexicon = &lexicon[1];
		lexsize--;
	}

	*parselen = length;
	*stream = malloc((*parselen) * sizeof(Token));

	if ((*parselen) > 1) for (size_t i = 0; i < (*parselen)-1; i++)
		(*stream)[i] = tstream[i];

	*stream = tstream;
}

// --------------------------
// Targets ------------------

void codegen_x86_64(Token* TokenStream, uint length, str buffer, int* buf_size) 
{
	
}


// --------------------------
// Main ---------------------

bool isTargetFile(str arg) {
	if (strlen(arg) < 5)
		return false;

	str ending = &arg[strlen(arg) - 5];
	if (strcmp(ending, ".dang") == 0)
		return true;
	else
		return false;
}

bool isCompilerFlag(str arg) {
	if (strlen(arg) < 2)
		return false;

	if (arg[0] == '-')
		return true;
	else
		return false;
}

void compileTarget(const str filename) {
	/**
	 * @brief Generate code corresponding to stream 
	 */

	printf("[INFO] Compiling target %s...", filename);
	fflush(stdout);

	// lex and parse
	Token* stream;
	uint length = 0;
	parse(filename, &stream, &length);

	printf("String: %s\n", stream[2].value.__l.value.__s);
	return;

	// Generate asm code
	while (length)
	{
		Token token = *stream;

		stream = &stream[1];
		length--;
	}
}

int main(int argc, str argv[])
{
	// dang -f <target>.dang --Flag

	// Ignore name of binary
	argv = &argv[1];
	argc--;

	str* targets = malloc(argc * sizeof(str));
	str* cflags = malloc(argc * sizeof(str));
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

	/**
	 * @todo
	 * Get cmdline args and decide what to do
	 * compile
	 * - open and read target file into string
	 * - split string by keywords into tokens
	 * - decide token struct
	 */

	while (n_targets)
	{
		const str target = *targets;

		/**
		 * @brief Compilation
		 * 1. Read file into buffer as string
		 * 2. Lex file into logical words
		 * 3. Parse words into token stream
		 * 4. Generate code corresponding to stream 
		 * 5. Pop target from targets array
		 */

		compileTarget(target);

		// Shift base pointer ahead
		targets = &targets[1];
		n_targets--;
	}

	return 0;
}
