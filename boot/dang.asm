BITS 64

section .text
global _start
_start:
mov rax, 0x01
mov rdi, 1
mov rsi, str0
mov rdx, 13
syscall

mov qword [greet], str0

mov rax, 0x01
mov rdi, 1
mov rsi, [greet]
mov rdx, 13
syscall

mov rax, 60
mov rdi, 0
syscall

section .data
str0: db "Hello, World!"

section .bss
greet: resb 13