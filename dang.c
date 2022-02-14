#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;
typedef char* str;

void printall(str* array, size_t size) {
	for (size_t i = 0; i < size; i++)
		printf("%s\n", array[i]);
}

// --------------------------
// Lexer --------------------

void readTargetFile(str filename, str* buffer, unsigned int* size) {
	/**
	 * @brief Read file into buffer as string
	 */

	FILE* f_ptr = fopen(filename, "r");

	if (f_ptr == NULL) {
		fprintf(stderr, "\nCouldn't open file \"%s\"\n", filename);
		fflush(stderr);
		exit(1);
	}

	// Find size of file
	fseek(f_ptr, 0L, SEEK_END);
	*size = ftell(f_ptr);
	fseek(f_ptr, 0L, SEEK_SET);

	// Allocate that many bytes and read
	*buffer = malloc((*size) * sizeof(char));

	unsigned int index = 0;
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

void lex(const str filename, str** lexicon, unsigned int* lexsize) {
	/**
	 * @brief Lex file into logical words
	 */

	str buffer;
	unsigned int size;

	readTargetFile(filename, &buffer, &size);
	printf("\b\b\b, %d bytes.\n", size);

	str* __lexicon;
	size_t length = 0;
	str current = "";
	unsigned int __ci = 0;

	while (size) 
	{
		const char c = *buffer;

		if (
			// Ignore LF, TAB and SPACE, 
			(c != '\n' && c != '\t' && c != ' ') ||
			// unless first char of current word is quote
			// and the quote isnt closed yet; for strings
			(current[0] == '\"' && (__ci > 0 ? current[__ci-1] != '\"' : true))
		)
		{
			char new[++__ci];
			for (size_t i = 0; i < __ci-1; i++)
				new[i] = current[i];

			new[__ci-1] = c; new[__ci] = '\0';
			
			current = malloc((__ci + 1) * sizeof(char));
			strcpy(current, new);
		}
		// time to add a new lex
		else if (strcmp(current, "") != 0)
		{
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

			// printf("LEX %s\n", __lexicon[length-1]);
			current = malloc(sizeof(char));
			strcpy(current, "");
			__ci = 0;
		}			

		buffer = &buffer[1];
		size--;
	}

	*lexsize = length;
	*lexicon = malloc((*lexsize) * sizeof(str*));
	for (size_t i = 0; i < *lexsize; i++) {
		(*lexicon)[i] = malloc(strlen(__lexicon[i]) * sizeof(char));
		strcpy((*lexicon)[i], __lexicon[i]);
	}
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


Token* parse(const str filename, unsigned int* parselen) {
	/**
	 * @brief Parse words into token stream
	 */

	unsigned int lexsize = 0;
	str* lexicon; 
	lex(filename, &lexicon, &lexsize);

	printall(lexicon, lexsize);
	exit(0);

	Token* TokenStream = malloc(sizeof(Token));
	unsigned int length = 0;

	while (lexsize)
	{
		const str word = *lexicon;

		// *(&TokenStream[length]) = malloc(sizeof(Token));

		// Includes
		if (strcmp(word, "include")) {
			// parse( next word )
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
		lexsize--;

		length++;
	}

	*parselen = length;
	return TokenStream;
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

	printf("Compiling target %s...", filename);
	fflush(stdout);

	// lex and parse
	unsigned int length = 0;
	Token* stream = parse(filename, &length);

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
		n_targets--;
	}

	return 0;
}
