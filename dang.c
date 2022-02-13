#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false, true } bool;
typedef char* str;

// --------------------------
// Lexer --------------------

void lex() {

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
	USE,
} 
Keyword;

typedef enum 
{
	// Binary Ops 
	ADD, // +
	SUB, // -
	MUL, // *
	DIV, // /
	MOD, // %

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
	BIT_NOT, 			// !
	LOGICAL_NOT,  // !!
	
	// INCREMENT, // ++
	// DECREMENT, // --
	// DEREF, // *
}
Operation;


enum ValueTypes {
	Int,
	Float,
	String,
	Null,
};

struct Literal 
{
	enum ValueTypes type;
	union {
		int;
		float;
		str;
	} value;
};

struct Identifier 
{
	str name;
	enum ValueTypes type;
};


typedef enum TokenTypes {
	KeywordToken,
	LiteralToken,
	IdentifierToken,
	OperationToken,
} 
TokenType;

struct Token
{
	TokenType type;
	union {
		Keyword;
		Operation;
		struct Literal;
		struct Identifier;
	} token;

	struct Token* next;
	struct Token* prev;
};

void parser() {

}

// --------------------------
// Main ---------------------

bool isTargetFile(str arg) {
	if(strlen(arg) < 5) return false;

	str ending = &arg[ strlen(arg) - 5];
	if (strcmp(ending, ".dang") == 0)
		return true;
	else
		return false;
}

bool isCompilerFlag(str arg) {
	if(strlen(arg) < 2) return false;

	if (arg[0] == '-')
		return true;
	else
		return false;
}

void printall(str* array, size_t size) {
	for (size_t i = 0; i < size; i++)
		printf("%s\n", array[i]);
}


void readTargetFile(str filename) {
	FILE* f_ptr = fopen(filename, "r");



	fclose(f_ptr);
}

int main(int argc, str argv[]) {
	// dang -f <target>.dang --Flag
	
	// Ignore name of binary 
	argv = &argv[1];
	argc--;

	str* targets = malloc( argc * sizeof(str) );	
	str* cflags = malloc( argc * sizeof(str) );
	int n_targets = 0, n_cflags = 0;

	while(argc) {
		str arg = argv[--argc];
		if (isTargetFile(arg))
			targets[n_targets++] = arg;
		else if (isCompilerFlag(arg))
			cflags[n_cflags++] = arg;
		else
			printf("Ignoring unknown argument \"%s\"\n", arg);
	}

	for (size_t i = n_targets; i < argc; i++)
		free(targets[i]);
	for (size_t i = n_cflags; i < argc; i++)
		free(cflags[i]);

	/**
	 * @todo
	 * Get cmdline args and decide what to do
	 * compile
	 * - open and read target file into string
	 * - split string by keywords into tokens
	 * - decide token struct
	 */

	for (size_t i = 0; i < n_targets; i++)
	{
		const str target = targets[i];
		printf("Compiling target %s\n", target);

		/**
		 * @brief Read target file
		 * @todo 
		 */


	}
	

	return 0;
}

