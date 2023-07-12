from typing import Tuple


def index_to_loc(file: str, index: int) -> Tuple[int, int]:
    """algo to decode loc from index

    Args:
        file (str): file string
        index (int): index of character

    Returns:
        Tuple[int, int]: row, column
    """
    
    assert index <= len(file), ValueError("Index outside file length!")
    
    chars = 0
    for row, line in enumerate(file.splitlines()):
        if chars + len(line) > index:
            col = index - chars + 1
            return (row + 1, col)
        
        row += 1
        chars += len(line) + 1
        
    raise ValueError("Index outside file length!")


class Stack(list):
    """ Dummy wrapper around list for naming convention
    """
    
    def push(self, __object):
        super().append(__object)
    
    @property
    def empty(self) -> bool:
        return len(self) == 0


class Scope(dict):
    """ Dummy wrapper around dict
    """
    
    def push(self, __name: str, __object):
        assert __name not in self, f"{__name} exists in scope"
        self[__name] = __object
    
    @property
    def empty(self) -> bool:
        return len(self) == 0
    
    @classmethod
    def from_parent(cls, parent):
        new = cls()
        new.update(parent)
        return new

# #include <stddef.h>
# #include <stdio.h>
# #include <stdlib.h>

# #ifndef UTILS_C_INCLUDED
# #define UTILS_C_INCLUDED

# typedef enum { false, true } bool;
# typedef unsigned int uint;
# typedef char *str;

# // Stack of module paths being compiled
# str *__TARGETS__;

# /**
#  * @brief Conveinience function to throw compiler error
#  *
#  * @param message Error message
#  */
# void CompilerError(str message) {
#   fprintf(stderr, "\nError: %s\n", message);
#   exit(1);
# }

# /**
#  * @brief Create owned formatted strings
#  *
#  * @param ln Initial string
#  * @param ... Formatting params
#  * @return str Owned string
#  */
# str fstr(str ln, ...) {
#   va_list args;
#   va_start(args, ln);

#   uint _LINE_MAXSIZE = strlen(ln) + 128;
#   str line = malloc(_LINE_MAXSIZE * sizeof(char));
#   int flen = vsnprintf(line, _LINE_MAXSIZE * sizeof(char), ln, args);

#   if (flen > _LINE_MAXSIZE)
#     CompilerError("Formattted string too large.");

#   va_end(args);

#   str new;
#   if (flen > 0) {
#     new = malloc(flen * sizeof(char));
#     strcpy(new, line);
#   }

#   free(line);
#   return new;
# }

# /**
#  * @brief Write unformatted line to buffer
#  *
#  * @param buf
#  * @param ln
#  */
# void wline(str *buf, str ln) {
#   uint _flen = strlen(ln);
#   if (_flen == 0)
#     return;

#   uint _size = strlen(*buf);
#   str new = malloc((_size + (_size > 0 ? 2 : 1) + _flen) * sizeof(char));
#   strcpy(new, *buf);
#   free(*buf);

#   if (_size > 0)
#     new[_size++] = '\n';

#   strcpy(&(new[_size]), ln);
#   new[_size + _flen] = '\0';
#   *buf = new;
# }

# /**
#  * @brief Write formatted string to buffer
#  *
#  * @param buf
#  * @param ln
#  * @param ...
#  */
# void fline(str *buf, str ln, ...) {
#   va_list args;
#   va_start(args, ln);

#   uint _LINE_MAXSIZE = strlen(ln) + 128;
#   str line = malloc(_LINE_MAXSIZE * sizeof(char));
#   int flen = vsnprintf(line, _LINE_MAXSIZE * sizeof(char), ln, args);

#   if (flen > _LINE_MAXSIZE)
#     CompilerError("Formattted string too large.");

#   va_end(args);

#   if (flen <= 0)
#     return;

#   uint _size = strlen(*buf);
#   str new = malloc((_size + (_size > 0 ? 2 : 1) + flen) * sizeof(char));
#   strcpy(new, *buf);
#   free(*buf);

#   if (_size > 0)
#     new[_size++] = '\n';

#   strcpy(&new[_size], line);
#   free(line);
#   new[_size + flen] = '\0';
#   *buf = new;
# }

# /**
#  * @brief Check if a given module has been included
#  * Will be useful to check for circular dependencies
#  */
# bool isTargetCompiling(str module) {
#   uint __ti = 0;
#   while (__TARGETS__[__ti] != NULL)
#     if (strcmp(__TARGETS__[__ti++], module) == 0)
#       return true;

#   return false;
# }

# /**
#  * @brief Set a given module has been included
#  */
# void setTargetCompiling(str module) {
#   if (isTargetCompiling(module))
#     CompilerError(fstr(
#         "Circular dependency, \"%s\" dependends on a module that is using it.",
#         module));

#   uint __ti = 0;
#   str *targets = malloc(sizeof(str));

#   targets[__ti] = malloc(strlen(module) * sizeof(char));
#   strcpy(targets[__ti], module);

#   while (__TARGETS__[__ti] != NULL) {
#     targets[__ti + 1] = malloc(strlen(__TARGETS__[__ti]) * sizeof(char));
#     strcpy(targets[__ti + 1], __TARGETS__[__ti]);
#     free(__TARGETS__[__ti]);
#     targets[(++__ti) + 1] = NULL;
#   }
#   free(__TARGETS__);
#   __TARGETS__ = targets;
# }

# bool arrIncludes(str array[], uint len, str query) {
#   for (uint i = 0; i < len; i++) {
#     if (strcmp(array[i], query) == 0)
#       return true;
#   }
#   return false;
# }

# int indexOf(str array[], uint len, str query) {
#   for (uint i = 0; i < len; i++) {
#     if (strcmp(array[i], query) == 0)
#       return i;
#   }
#   return -1;
# }

# void printall(str *array, size_t size) {
#   for (size_t i = 0; i < size; i++)
#     printf("%s\n", array[i]);
# }

# #endif