BITS 64
section .data
str0: db "Hello, World!", 0x00
str1: db "Goodnight Tennessee", 0x00
section .bss
_var_greet: resb 8
_var_greet2: resb 8
_var_code: resb 8
section .text
global _start
_start:
mov rax, 1
mov rdi, 1
mov rsi, str0
mov rdx, 13
syscall
mov qword [_var_greet], str1
mov rax, str1
mov rax, 1
mov rdi, 1
mov rsi, [_var_greet]
mov rdx, 19
syscall
mov rsi, [_var_greet]
mov qword [_var_greet2], rsi
mov rax, 1
mov rdi, 1
mov rsi, [_var_greet2]
mov rdx, 19
syscall
mov rax, 9
add rax, 1
mov rdx, rax
mov rax, 8
sub rax, rdx
mov rdx, rax
mov rax, 3
add rax, rdx
mov rdx, rax
mov rax, 4
sub rax, rdx
mov rdx, rax
mov rax, 7
sub rax, rdx
mov rdx, rax
mov rax, 2
add rax, rdx
mov rdx, rax
mov rax, 3
sub rax, rdx
mov rdx, rax
mov rax, 4
add rax, rdx
mov rdx, rax
mov rax, 3
sub rax, rdx
mov rdx, rax
mov rax, 2
sub rax, rdx
mov rdx, rax
mov rax, 5
add rax, rdx
mov rdx, rax
mov rax, 1
add rax, rdx
mov rdx, rax
mov rax, 9
add rax, rdx
mov rdx, rax
mov rax, 8
sub rax, rdx
mov rdx, rax
mov rax, 3
add rax, rdx
mov rdx, rax
mov rax, 4
sub rax, rdx
mov rdx, rax
mov rax, 7
sub rax, rdx
mov rdx, rax
mov rax, 2
add rax, rdx
mov rdx, rax
mov rax, 3
sub rax, rdx
mov rdx, rax
mov rax, 4
add rax, rdx
mov rdx, rax
mov rax, 3
sub rax, rdx
mov rdx, rax
mov rax, 2
sub rax, rdx
mov rdx, rax
mov rax, 5
add rax, rdx
mov rdx, rax
mov [_var_code], rdx
mov rax, 60
mov rdi, [_var_code]
syscall
mov rax, 60
mov rdi, 0
syscall
