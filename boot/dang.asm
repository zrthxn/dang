BITS 64

section .text
_fn_exit:
pop rsi
pop qword [_var_fn_exit_status]
push rsi
mov rax, 60
mov rdi, [_var_fn_exit_status]
syscall
ret
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
mov qword [_var_code], 69
mov rax, 69
mov rax, [_var_code]
add rax, 1
mov rdx, rax
push qword rdx
call _fn_exit
mov rdx, [_rtn_fn_exit]
mov rax, 60
mov rdi, 0
syscall

section .data
str0: db "Hello, World!", 0x00
str1: db "Goodnight Tennessee", 0x00

section .bss
_var_greet: resb 8
_rtn_fn_exit: resb 8
_var_fn_exit_status: resb 8
_var_code: resb 8

