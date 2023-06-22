#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "src/utils.c"
#include "src/lexer.c"
#include "src/parser.c"
#include "src/codegen.c"

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

void compileTarget(const str filename, const str outfile)
{
	/**
	 * @brief Generate code corresponding to stream
	 */

	printf("[INFO] Compiling target %s...", filename);
	fflush(stdout);

	// lex and parse
	setTargetCompiling(filename);
	
	uint length = 0;
	TokenStream stream = parse(filename);

	codegen(stream, outfile);
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
	while (__c)
	{
		str arg = argv[--__c];
		if (isTargetFile(arg))
			targets[n_targets++] = arg;
		else if (isCompilerFlag(arg))
			cflags[n_cflags++] = arg;
		else
			printf("[INFO] Ignoring unknown argument \"%s\"\n", arg);
	}

	for (size_t i = n_targets; i < argc; i++)
		free(targets[i]);
	for (size_t i = n_cflags; i < argc; i++)
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
		str OUT = "a.out";

		if (arrIncludes(cflags, n_cflags, "-o"))
			OUT = cflags[indexOf(cflags, n_cflags, "-o") + 1];

		compileTarget(target, ASM);

		system(fstr("nasm -felf64 %s", ASM));
		system(fstr("ld -o %s %s", OUT, OBJ));
		system(fstr("rm %s", OBJ));
		
		if (arrIncludes(cflags, n_cflags, "-asm") == false)
			system(fstr("rm %s", ASM));

		// Shift base pointer ahead
		targets = &targets[1];
		n_targets--;
	}

	return 0;
}
