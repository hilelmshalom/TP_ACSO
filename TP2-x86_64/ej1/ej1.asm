; ej1.asm
; Implementación en ensamblador x86-64 de las funciones de manejo de listas.

; Define constantes para mayor legibilidad
%define NULL 0
%define TRUE 1
%define FALSE 0

; --- Estructura string_proc_list (16 bytes) ---
%define LIST_FIRST  0
%define LIST_LAST   8

; --- Estructura string_proc_node (32 bytes) ---
%define NODE_NEXT       0
%define NODE_PREVIOUS   8
%define NODE_TYPE       16
%define NODE_HASH       24

section .data
    empty_string: db "", 0  ; String vacío para usar en string_proc_list_concat_asm

section .text

; --- Declaraciones Externas ---
extern malloc
extern free
extern str_concat  ; Aunque ya no se usa en la función crítica, puede ser usada por otras partes.
extern strlen      ; Necesaria para la nueva implementación
extern strcpy      ; Necesaria para la nueva implementación
extern strcat      ; Necesaria para la nueva implementación

; Declaración de las funciones que se van a exportar
global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; -----------------------------------------------------------------------------
; string_proc_list* string_proc_list_create(void);
; Retorna en RAX un puntero a una nueva lista.
string_proc_list_create_asm:
    push    rbp
    mov     rbp, rsp

    ; 1. Llama a malloc para alocar memoria para la lista (16 bytes)
    mov     rdi, 16         ; Tamaño de la estructura string_proc_list
    call    malloc

    ; 2. Verifica si malloc devolvió NULL
    test    rax, rax
    jz      .end            ; Si es NULL, retorna NULL (RAX ya es 0)

    ; 3. Inicializa los punteros 'first' y 'last' a NULL
    mov     qword [rax + LIST_FIRST], NULL
    mov     qword [rax + LIST_LAST],  NULL

.end:
    pop     rbp
    ret

; -----------------------------------------------------------------------------
; string_proc_node* string_proc_node_create(uint8_t type, char* hash);
; Argumentos: RDI = type, RSI = hash
; Retorna en RAX un puntero al nuevo nodo.
string_proc_node_create_asm:
    push    rbp
    mov     rbp, rsp
    push    rbx             ; Preservar RBX
    push    r12             ; Preservar R12
    push    r13             ; Preservar R13

    mov     r12, rdi        ; Guardar 'type' en r12
    mov     r13, rsi        ; Guardar 'hash' en r13

    ; 1. Llama a malloc para alocar memoria para el nodo (32 bytes)
    mov     rdi, 32         ; Tamaño de la estructura string_proc_node
    call    malloc

    ; 2. Verifica si malloc devolvió NULL
    test    rax, rax
    jz      .end            ; Si es NULL, retorna NULL (RAX ya es 0)

    mov     rbx, rax        ; Guardar el puntero al nuevo nodo en RBX

    ; 3. Inicializa los campos del nodo
    mov     qword [rbx + NODE_NEXT], NULL       ; node->next = NULL
    mov     qword [rbx + NODE_PREVIOUS], NULL   ; node->previous = NULL
    mov     byte [rbx + NODE_TYPE], r12b        ; node->type = type
    mov     qword [rbx + NODE_HASH], r13        ; node->hash = hash

.end:
    ; RAX ya contiene el puntero al nodo (o NULL si falló malloc)
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; -----------------------------------------------------------------------------
; void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash);
; Argumentos: RDI = list, RSI = type, RDX = hash
string_proc_list_add_node_asm:
    push    rbp
    mov     rbp, rsp
    push    rbx             ; Preservar registros que se usarán
    push    r12
    push    r13
    push    r14

    mov     r12, rdi        ; Guardar 'list' en r12
    mov     r13, rsi        ; Guardar 'type' en r13
    mov     r14, rdx        ; Guardar 'hash' en r14

    ; 1. Crea un nuevo nodo llamando a string_proc_node_create_asm
    mov     rdi, r13        ; Arg1: type
    mov     rsi, r14        ; Arg2: hash
    call    string_proc_node_create_asm

    mov     rbx, rax        ; Guardar el nuevo nodo en RBX
    test    rbx, rbx
    jz      .end            ; Si el nodo es NULL, no hay nada que hacer

    ; 2. Verifica si la lista está vacía (list->first == NULL)
    cmp     qword [r12 + LIST_FIRST], NULL
    jne     .list_not_empty

.list_is_empty:
    ; La lista está vacía, el nuevo nodo es tanto el primero como el último
    mov     [r12 + LIST_FIRST], rbx ; list->first = node
    mov     [r12 + LIST_LAST],  rbx ; list->last = node
    jmp     .end

.list_not_empty:
    ; La lista no está vacía, agregar al final
    mov     rax, [r12 + LIST_LAST]  ; Obtener el último nodo actual
    mov     [rax + NODE_NEXT], rbx  ; list->last->next = node
    mov     [rbx + NODE_PREVIOUS], rax ; node->previous = list->last
    mov     [r12 + LIST_LAST], rbx  ; list->last = node

.end:
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

; -----------------------------------------------------------------------------
; char* string_proc_list_concat(string_proc_list* list, uint8_t type, char* hash);
;
; IMPLEMENTACIÓN OPTIMIZADA (ENFOQUE DE DOS PASOS)
; 1. Recorre la lista para calcular el tamaño total necesario.
; 2. Llama a malloc UNA SOLA VEZ.
; 3. Recorre la lista de nuevo para copiar los strings en el buffer.
;
; Argumentos: RDI = list, RSI = type, RDX = hash
; Retorna en RAX el puntero al string concatenado.
string_proc_list_concat_asm:
    push    rbp
    mov     rbp, rsp
    push    rbx             ; Preservar registros callee-saved
    push    r12
    push    r13
    push    r14
    push    r15

    ; --- Guardar argumentos ---
    mov     r12, rdi        ; r12 = list
    mov     r13b, sil       ; r13b = type (solo el byte)
    mov     r14, rdx        ; r14 = hash inicial

    ; ======================================================================
    ; PASO 1: CALCULAR EL TAMAÑO TOTAL DEL BUFFER
    ; ======================================================================
    
    ; Obtener longitud del hash inicial
    mov     rdi, r14
    call    strlen
    mov     rbx, rax        ; rbx = total_size = strlen(hash)

    ; Iniciar recorrido para sumar longitudes: r15 = current_node
    mov     r15, [r12 + LIST_FIRST]

.size_loop_start:
    test    r15, r15        ; while (current_node != NULL)
    jz      .size_loop_end

    ; if (current_node->type == type)
    cmp     byte [r15 + NODE_TYPE], r13b
    jne     .size_loop_continue

    ; El tipo coincide, sumar la longitud de su hash
    mov     rdi, [r15 + NODE_HASH]
    call    strlen
    add     rbx, rax        ; total_size += strlen(current_node->hash)

.size_loop_continue:
    mov     r15, [r15 + NODE_NEXT] ; current_node = current_node->next
    jmp     .size_loop_start

.size_loop_end:
    inc     rbx             ; total_size += 1 (para el terminador nulo '\0')

    ; ======================================================================
    ; PASO 2: ASIGNAR MEMORIA Y COPIAR LOS DATOS
    ; ======================================================================

    ; Asignar memoria (una sola vez)
    mov     rdi, rbx        ; Argumento para malloc: total_size
    call    malloc

    test    rax, rax        ; Si malloc falla, retornar NULL
    jz      .exit

    mov     rbx, rax        ; rbx = puntero al nuevo buffer (resultado)

    ; Copiar el hash inicial en el buffer
    mov     rdi, rbx        ; Destino: el nuevo buffer
    mov     rsi, r14        ; Fuente: el hash inicial
    call    strcpy

    ; Iniciar segundo recorrido para concatenar: r15 = current_node
    mov     r15, [r12 + LIST_FIRST]

.copy_loop_start:
    test    r15, r15        ; while (current_node != NULL)
    jz      .copy_loop_end

    ; if (current_node->type == type)
    cmp     byte [r15 + NODE_TYPE], r13b
    jne     .copy_loop_continue

    ; El tipo coincide, concatenar su hash
    mov     rdi, rbx                ; Destino: el buffer de resultado
    mov     rsi, [r15 + NODE_HASH]  ; Fuente: el hash del nodo
    call    strcat

.copy_loop_continue:
    mov     r15, [r15 + NODE_NEXT]  ; current_node = current_node->next
    jmp     .copy_loop_start

.copy_loop_end:
    mov     rax, rbx        ; El resultado final está en rbx

.exit:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret