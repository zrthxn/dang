#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "utils.c"

#ifndef LEXER_C_INCLUDED
#define LEXER_C_INCLUDED
// --------------------------
// Lexer --------------------

#define LF '\n'
#define CR '\r'
#define TAB '\t'
#define SPACE ' '
#define COMMENT '#'

void readTargetFile(str filename, str *buffer, uint *size)
{
	/**
	 * @brief Read file into buffer as string
	 */

	FILE *f_ptr = fopen(filename, "r");

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

void lex(const str filename, str **lexicon, uint *lexsize)
{
	/**
	 * @brief Lex file into logical words
	 */

	str buffer;
	uint size;

	readTargetFile(filename, &buffer, &size);
	printf("\b\b\b, %d bytes.\n", size);

	str *__lexicon;
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

#endif