; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern str_concat
extern strcmp


string_proc_list_create_asm:

string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    push rbx             ; Save callee-saved register
    movzx ebx, dil       ; type (uint8_t)
    mov rdx, rsi         ; hash (char*)
    mov rdi, 32          ; sizeof(string_proc_node) = 32 bytes
    call malloc
    test rax, rax
    jz .end
    mov qword [rax], 0      ; next = NULL
    mov qword [rax+8], 0    ; previous = NULL
    mov byte [rax+16], bl   ; type
    mov [rax+24], rdx       ; hash
.end:
    pop rbx
    leave
    ret
string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    mov r12, rdi          ; list
    mov r13b, sil         ; type
    mov r14, rdx          ; hash
    mov dil, r13b         ; arg1: type
    mov rsi, r14          ; arg2: hash
    call string_proc_node_create_asm
    test rax, rax
    jz .end
    cmp qword [r12], 0    ; Check if list is empty
    jne .append
    mov [r12], rax        ; first = node
    mov [r12+8], rax      ; last = node
    jmp .end
.append:
    mov rcx, [r12+8]      ; current last node
    mov [rcx], rax        ; last->next = node
    mov [rax+8], rcx      ; node->previous = last
    mov [r12+8], rax      ; list->last = node
.end:
    pop r14
    pop r13
    pop r12
    leave
    ret

string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi          ; list
    mov r13b, sil         ; type
    mov r14, rdx          ; target hash
    ; Allocate initial empty string
    mov rdi, 1
    call malloc
    test rax, rax
    jz .error
    mov byte [rax], 0
    mov r15, rax          ; result
    mov rcx, [r12]        ; current_node = list->first
.loop:
    test rcx, rcx
    jz .done
    ; Check type
    mov al, [rcx+16]
    cmp al, r13b
    jne .next
    ; Check hash with strcmp
    mov rdi, [rcx+24]     ; node->hash
    mov rsi, r14          ; target hash
    call strcmp
    test eax, eax
    jnz .next
    ; Concat strings
    mov rdi, r15
    mov rsi, [rcx+24]
    call str_concat
    test rax, rax
    jz .concat_error
    ; Replace old string
    mov rdi, r15
    mov r15, rax
    call free
.next:
    mov rcx, [rcx]        ; current_node = current_node->next
    jmp .loop
.concat_error:
    mov rdi, r15
    call free
    xor r15d, r15d
.error:
    xor eax, eax
.done:
    mov rax, r15
    pop r15
    pop r14
    pop r13
    pop r12
    leave
    ret
