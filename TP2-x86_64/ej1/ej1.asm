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


string_proc_list_create_asm:
    # Prologue
    pushq   %rbp
    movq    %rsp, %rbp

    # Allocate memory: sizeof(string_proc_list) = 16 (2 pointers)
    movl    $16, %edi       # size argument for malloc
    call    malloc          # call malloc, result in %rax

    testq   %rax, %rax      # check if malloc returned NULL
    je      .return_null    # if NULL, return NULL

    # Initialize list->first = NULL
    movq    $0, (%rax)

    # Initialize list->last = NULL
    movq    $0, 8(%rax)

    # Return pointer to newly created list
    jmp     .return

.return_null:
    movq    $0, %rax        # return NULL

.return:
    # Epilogue
    popq    %rbp
    ret
string_proc_node_create_asm:

string_proc_list_add_node_asm:

string_proc_list_concat_asm:

