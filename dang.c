#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;
typedef char* str;

// --------------------------
// Lexer --------------------

void readTargetFile(str filename, str* buffer, int* size)
{
	/**
	 * @brief Read file into buffer as string
	 */

	FILE* f_ptr = fopen(filename, "r");

	if (f_ptr == NULL) {
		fprintf(stderr, "\nCouldn't open file \"%s\"\n", filename);
		exit(1);
	}

	// Find size of file
	fseek(f_ptr, 0L, SEEK_END);
	*size = ftell(f_ptr);
	fseek(f_ptr, 0L, SEEK_SET);

	// Allocate that many bytes and read
	*buffer = malloc((*size) * sizeof(char));
	fgets(*buffer, *size, f_ptr);

	fclose(f_ptr);
}

str* lex(const str filename, unsigned int* lexsize) {
	/**
	 * @brief Lex file into logical words
	 */

	str buffer;
	int size;

	readTargetFile(filename, &buffer, &size);
	printf("\b\b\b, %d bytes.\n", size);

	str* lexicon;
	unsigned int length = 0;

	// lexing
	// length++;

	*lexsize = length;
	*lexicon[length] = NULL;

	return lexicon;
}

// --------------------------
// Parser -------------------

typedef enum
{
	VAR,
	FN,
	IF,
	ELIF,
	ELSE,
	END,
	WHILE,
	DO,
	INCLUDE,
} Keyword;

typedef enum
{
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
} 
Operator;

typedef enum
{
	IntValue,
	FloatValue,
	StringValue,
	NullValue,
} ValueType;

typedef struct
{
	ValueType type;
	union
	{
		int __i;
		float __f;
		str __s;
	} value;
} Literal;

typedef struct
{
	str name;
	ValueType type;
} Identifier;

typedef enum
{
	KeywordToken,
	LiteralToken,
	IdentifierToken,
	OperatorToken,
} TokenType;

typedef struct
{
	TokenType type;
	union
	{
		Keyword __k;
		Operator __o;
		Identifier __i;
		Literal __l;
	} token;
} Token;


Token* parse(const str filename, unsigned int* parselen)
{
	/**
	 * @brief Parse words into token stream
	 */

	int lexsize = 0;
	str* lexicon = lex(filename, &lexsize);

	Token* TokenStream;
	unsigned int length = 0;

	while (lexsize && lexicon != NULL)
	{
		const str lex = *lexicon;

		// TokenStream[length] = malloc(sizeof(Token));

		// Includes
		if (strcmp(lex, "include"))
		{
			// parse( next lex )
		}

		// Token new = {
		// 	.type = KeywordToken,
		// 	.token = END,
		// 	.prev = TokenStream,
		// 	.next = NULL,
		// };

		// TokenStream->next = &new;

		// Advance to next lex
		lexicon = &lexicon[1];
		free(lexicon[0]);
		lexsize--;

		length++;
	}

	*parselen = length;
	return TokenStream;
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

void printall(str* array, size_t size)
{
	for (size_t i = 0; i < size; i++)
		printf("%s\n", array[i]);
}

void compileTarget(const str filename)
{
	/**
	 * @brief Generate code corresponding to stream 
	 */

	printf("Compiling target %s...", filename);
	fflush(stdout);

	// lex and parse
	unsigned int length = 0;
	Token* stream = parse(filename, &length);
	printf("\nparsed\n");

	// Generate asm code
	while (length && stream != NULL)
	{
		Token token = *stream;

		stream = &stream[1];
		// free(stream[0]);
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
			printf("Ignoring unknown argument \"%s\"\n", arg);
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
		free(targets[0]);
		n_targets--;
	}

	return 0;
}
