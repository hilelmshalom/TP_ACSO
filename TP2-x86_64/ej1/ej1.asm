; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data
empty_str: db 0           ; Cadena vacía para inicialización
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
extern strdup
extern strlen
extern strcat

string_proc_list_create_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16                  ; Reservar espacio para variables locales
        
        mov     edi, 16                  ; Solicitar 16 bytes (2 punteros de 64 bits)
        call    malloc
        
        mov     QWORD [rbp-8], rax       ; Almacenar puntero asignado
        cmp     QWORD [rbp-8], NULL      ; Verificar éxito de malloc
        jne     .inicializar_punteros
        
        mov     eax, NULL                ; Fallo de asignación, retornar NULL
        jmp     .salida

.inicializar_punteros:
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax], NULL        ; list->first = NULL
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], NULL      ; list->last = NULL
        mov     rax, QWORD [rbp-8]      ; Devolver puntero a la lista

.salida:
        leave
        ret

string_proc_node_create_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32                  ; Reservar espacio para variables locales
        
        ; Guardar parámetros en la pila
        mov     eax, edi                 ; Tipo de nodo (byte)
        mov     QWORD [rbp-32], rsi      ; Puntero a cadena
        mov     BYTE [rbp-20], al        ; Almacenar tipo en pila
        
        mov     edi, 32                  ; Solicitar 32 bytes para el nodo
        call    malloc
        
        mov     QWORD [rbp-8], rax       ; Almacenar puntero asignado
        cmp     QWORD [rbp-8], NULL      ; Verificar éxito de malloc
        jne     .configurar_nodo
        
        mov     eax, NULL                ; Fallo de asignación, retornar NULL
        jmp     .salida_nodo

.configurar_nodo:
        mov     rax, QWORD [rbp-8]
        movzx   edx, BYTE [rbp-20]
        mov     BYTE [rax+16], dl        ; node->type = tipo recibido
        mov     rax, QWORD [rbp-8]
        mov     rdx, QWORD [rbp-32]
        mov     QWORD [rax+24], rdx      ; node->string = puntero a cadena
        
        ; Inicializar punteros de lista
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax], NULL        ; node->prev = NULL
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], NULL      ; node->next = NULL
        mov     rax, QWORD [rbp-8]       ; Devolver puntero al nodo

.salida_nodo:
        leave
        ret
string_proc_list_add_node_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 48
        
        ; Guardar parámetros en la pila
        mov     QWORD [rbp-24], rdi      ; Almacenar lista
        mov     eax, esi                 ; Tipo de nodo
        mov     QWORD [rbp-40], rdx      ; Puntero a cadena
        mov     BYTE [rbp-28], al        ; Almacenar tipo
        
        cmp     QWORD [rbp-24], NULL     ; Verificar lista válida
        je      .lista_invalida
        
        ; Crear nuevo nodo
        movzx   eax, BYTE [rbp-28]
        mov     rdx, QWORD [rbp-40]
        mov     rsi, rdx
        mov     edi, eax
        call    string_proc_node_create_asm
        
        mov     QWORD [rbp-8], rax       ; Almacenar nuevo nodo
        cmp     QWORD [rbp-8], NULL      ; Verificar creación exitosa
        je      .nodo_invalido
        
        ; Verificar si la lista está vacía
        mov     rax, QWORD [rbp-24]
        cmp     QWORD [rax], NULL        ; list->first == NULL?
        jne     .insertar_al_final
        
        ; Caso lista vacía: actualizar first y last
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax], rdx         ; list->first = nuevo_nodo
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx       ; list->last = nuevo_nodo
        jmp     .salida_add

.insertar_al_final:
        ; Enlazar nuevo nodo al final de la lista
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rax+8]       ; Obtener último nodo
        mov     rax, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx       ; nuevo_nodo->next = último_nodo
        mov     rax, QWORD [rbp-24]
        mov     rax, QWORD [rax+8]       ; último_nodo
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax], rdx         ; último_nodo->prev = nuevo_nodo
        mov     rax, QWORD [rbp-24]
        mov     rdx, QWORD [rbp-8]
        mov     QWORD [rax+8], rdx       ; list->last = nuevo_nodo

.lista_invalida:
.nodo_invalido:
.salida_add:
        leave
        ret


string_proc_list_concat_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 64
        
        ; Guardar parámetros en la pila
        mov     QWORD [rbp-40], rdi      ; Almacenar lista
        mov     eax, esi                 ; Tipo a filtrar
        mov     QWORD [rbp-56], rdx      ; Cadena inicial
        mov     BYTE [rbp-44], al        ; Almacenar tipo
        
        ; Validar parámetros de entrada
        cmp     QWORD [rbp-40], NULL
        je      .parametros_invalidos
        cmp     QWORD [rbp-56], NULL
        jne     .calcular_longitud

.parametros_invalidos:
        mov     eax, NULL
        jmp     .salida_concat

.calcular_longitud:
        ; Calcular longitud inicial de la cadena base
        mov     rax, QWORD [rbp-56]
        mov     rdi, rax
        call    strlen
        mov     QWORD [rbp-8], rax       ; Almacenar longitud total
        
        ; Recorrer lista para calcular longitud total
        mov     rax, QWORD [rbp-40]
        mov     rax, QWORD [rax]         ; Obtener primer nodo
        mov     QWORD [rbp-16], rax      ; Inicializar iterador

.iterar_nodos_longitud:
        cmp     QWORD [rbp-16], NULL
        je      .asignar_memoria
        
        ; Verificar coincidencia de tipo
        mov     rax, QWORD [rbp-16]
        movzx   eax, BYTE [rax+16]       ; node->type
        cmp     BYTE [rbp-44], al
        jne     .siguiente_nodo
        
        ; Sumar longitud de la cadena del nodo
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]     ; node->string
        test    rax, rax
        je      .siguiente_nodo
        
        mov     rdi, rax
        call    strlen
        add     QWORD [rbp-8], rax

.siguiente_nodo:
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax]         ; node->prev
        mov     QWORD [rbp-16], rax
        jmp     .iterar_nodos_longitud

.asignar_memoria:
        ; Asignar memoria para la cadena resultante
        mov     rax, QWORD [rbp-8]
        inc     rax                      ; +1 para el byte nulo
        mov     rdi, rax
        call    malloc
        mov     QWORD [rbp-24], rax      ; Almacenar buffer de salida
        
        cmp     QWORD [rbp-24], NULL
        jne     .construir_cadena
        
        mov     eax, NULL
        jmp     .salida_concat

.construir_cadena:
        ; Inicializar buffer y concatenar cadenas
        mov     rax, QWORD [rbp-24]
        mov     BYTE [rax], 0            ; Inicializar cadena vacía
        
        ; Concatenar cadena inicial
        mov     rdx, QWORD [rbp-56]
        mov     rax, QWORD [rbp-24]
        mov     rsi, rdx
        mov     rdi, rax
        call    strcat
        
        ; Recorrer lista nuevamente para concatenar
        mov     rax, QWORD [rbp-40]
        mov     rax, QWORD [rax]         ; Obtener primer nodo
        mov     QWORD [rbp-16], rax

.iterar_nodos_concat:
        cmp     QWORD [rbp-16], NULL
        je      .fin_concat
        
        ; Verificar coincidencia de tipo
        mov     rax, QWORD [rbp-16]
        movzx   eax, BYTE [rax+16]       ; node->type
        cmp     BYTE [rbp-44], al
        jne     .siguiente_nodo_concat
        
        ; Concatenar cadena del nodo si existe
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax+24]     ; node->string
        test    rax, rax
        je      .siguiente_nodo_concat
        
        mov     rdx, rax
        mov     rax, QWORD [rbp-24]
        mov     rsi, rdx
        mov     rdi, rax
        call    strcat

.siguiente_nodo_concat:
        mov     rax, QWORD [rbp-16]
        mov     rax, QWORD [rax]         ; node->prev
        mov     QWORD [rbp-16], rax
        jmp     .iterar_nodos_concat

.fin_concat:
        mov     rax, QWORD [rbp-24]      ; Devolver buffer con cadena resultante

.salida_concat:
        leave
        ret