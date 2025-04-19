# Solución a la bomba 15

## Fase 1: String Comparación

### Análisis
Primero, desensamblé la función `phase_1` en GDB y noté una instrucción clave:
```asm
lea rsi,[rip+0xc7c62]  ; Carga la dirección del string esperado en RSI
```

### Método de Resolución
Usé el comando `x/s 0x4c9a58` (dirección calculada como `rip + 0xc7c62`) para inspeccionar esa ubicación en memoria, y apareció el string:  
`"Al que es amigo, jamas lo dejen en la estacada Siempre el amigo mas fiel es una conduta honrada"`.

Luego, vi que el programa llama a `strings_not_equal` para comparar mi input con ese string. Si no coincidían, saltaba a `explode_bomb`.

### Pasos detallados en GDB
1. **Desensamblar `phase_1`:**  
   ```bash  
   (gdb) disas phase_1  
   ```  
2. **Inspeccionar el string en `RSI`:**  
   ```bash  
   (gdb) x/s 0x4c9a58  
   ```  
3. **Verificar la comparación:**  
   El input debe coincidir exactamente con el string revelado.

### Solución
Introduje el texto tal cual en la fase 1, ¡y la bomba no explotó! Así confirmé que era la contraseña hardcodeada.

## Fase 2: Operaciones Lógicas

### Análisis del código con GDB
Primero, usé **GDB** para depurar el binario y entender `phase_2`. Establecí breakpoints críticos:
```bash
# Breakpoints clave
break *0x401e22   # Inicio de phase_2
break *0x401e62   # Llamada a strtol (primer número)
break *0x401e77   # Llamada a strtol (segundo número)
break *0x401e8c   # Llamada a strtol (tercer número)
break *0x401e98   # Comparación final (¡aquí se decide todo!)
```
Con `run` y `continue`, fui avanzando hasta cada breakpoint, usando `info registers` para inspeccionar valores en RAX, RBX, etc.

### Descubrimiento de la lógica oculta
En la dirección **0x401e94**, noté la instrucción `xor ebx, ebp`, seguida de `sar ebx, 1` (desplazamiento aritmético a la derecha). Esto me indicó que:
1. **XOR** entre el primer y segundo número.
2. **Dividir por 2** el resultado (con signo).
3. Comparar ese valor con el **tercer número** (en **0x401e98**).

Si la comparación fallaba, saltaba a `explode_bomb`.

### Validación con un script en Python
Para encontrar combinaciones válidas, escribí un script que probaba múltiples valores:
```python
for i in range(-20, 20):
    for j in range(-20, 20):
        xor_result = i ^ j
        expected_answer = xor_result // 2  # División con signo
        if expected_answer == -8:  # Busqué casos donde el tercer número fuera -8
            print(f"Posible solución: {i} {j} {-8}")
```
Entre las opciones, **10, -6, -8** llamaron mi atención. Verifiqué manualmente:
- `10 ^ (-6) = -16` (en complemento a 2).
- `-16 // 2 = -8` → ¡Éxito!

### Solución
Introduje **10 -6 -8** como input y... ¡la bomba no explotó! Confirmé que era la solución correcta al ver que el programa continuaba a la siguiente fase.

## Fase 3: Búsqueda Binaria

### Análisis inicial
Analicé el código ensamblador de `phase_3` y noté que la función `cuenta` implementaba una búsqueda binaria recursiva. Al observar la línea `401fd8: cmp eax,0xb` en `cuenta`, entendí que la bomba explotaría si el contador de pasos superaba 0d11=0xb. Además, la validación final en `phase_3` (`4020ca: cmp DWORD PTR [rsp+0x14],0x6`) requería que el número de pasos fuera mayor a 6.

### Entendimiento de la estructura
Sabía que `palabras.txt` tenía 10,784 palabras ordenadas alfabéticamente. Una búsqueda binaria en este rango teóricamente tomaría hasta ⌈log₂(10784)⌉ = **14 pasos**, pero el código limitaba el máximo a 11.

### Identificación del punto crítico
Usando GDB, coloqué un breakpoint en `strcmp` (`break *0x401fe7`) para inspeccionar las palabras comparadas. En cada iteración, verifiqué el valor de `RSI` con `x/s $rsi` y el contador de pasos en la dirección almacenada en `R13`.

### Selección de palabra estratégica
Busqué una palabra cerca del **inicio del archivo** para minimizar pasos. La palabra "aboquillar" aparecía en el **paso 7** (posición ~84). Esto se debe a que en cada iteración, la búsqueda binaria divide el espacio restante por 2:
```
Iteración 1: 10,784 / 2 = 5,392
Iteración 2: 5,392 / 2 = 2,696
Iteración 3: 2,696 / 2 = 1,348
Iteración 4: 1,348 / 2 = 674
Iteración 5: 674 / 2 = 337
Iteración 6: 337 / 2 = 168
Iteración 7: 168 / 2 = 84 → ¡Coincide con la posición de "aboquillar"!
```

### Validación del contador
Usando `x/d $r13` en GDB, confirmé que el contador llegaba exactamente a **7** antes de terminar la búsqueda. Esto cumplía con las condiciones:
- Mayor que 6 (`cmp DWORD PTR [rsp+0x14],0x6`).
- Menor o igual que 11 (`cmp eax,0xb`).

### Solución
Finalmente, usé la cadena `"aboquillar 7"` como input para `phase_3`. El entero `7` coincidía con el contador de pasos, y la palabra existía en la posición validada, desactivando la bomba.

## Fase 4: Seguimiento de Arreglo

### Entendimiento del formato de entrada
Primero, analicé el código de `phase_4` y noté que se llama a `sscanf` con la dirección `0x4c70ec`. Usé GDB para inspeccionar esa dirección con el comando:
```bash
x/s 0x4c70ec
```
Vi que el formato era `%d %d`, lo que me indicó que el input debía ser **dos números enteros**.

### Identificación de restricciones del primer número
El primer número (almacenado en `[rsp]`) se modifica con `AND 0xF`, lo que limita su valor a un rango de **0 a 15**. Además, si el resultado es `0xF` (15), la bomba explota. Por eso, el primer número debía estar entre **0 y 14**.

### Análisis del arreglo crítico con GDB
El código hace referencia a un arreglo en `0x4cde40`. Usé este comando para ver sus valores:
```bash
x/16wx 0x4cde40
```
El arreglo mostró:
`[7, 4, 6, 8, 13, 10, 15, 9, 0, 12, 3, 5, 2, 11, 1, 14]`
Esto correspondía a los índices **0 a 15**.

### Breakpoints clave
- **En `phase_4`:** Para detener la ejecución al inicio:
  ```bash
  b *0x402138
  ```
- **Después del loop:** Para verificar el contador (`edx`) y la suma (`ecx`):
  ```bash
  b *0x40219e
  ```
- **Antes de la comparación final:** Para validar los valores:
  ```bash
  b *0x4021a3
  ```

### Seguimiento del índice inicial
Usando el primer número **4** (que cumple `4 AND 0xF = 4`), seguí el recorrido en el arreglo:
- Índice inicial: **4** → valor **13**.
- Siguientes índices: **13 → 11 → 5 → 10 → 3 → 8 → 0 → 7 → 9 → 12 → 2 → 6 → 15** (13 pasos en total).

### Cálculo de la suma
Sumé los valores del recorrido:
```
13 + 11 + 5 + 10 + 3 + 8 + 0 + 7 + 9 + 12 + 2 + 6 + 15 = 101
```
Esto me dio el segundo número: **101**.

### Verificación con registros en GDB
Después de ejecutar el programa con el input `4 101`, inspeccioné los registros en los breakpoints:
- **`edx` (contador de pasos):** Mostró **13** (como se requería).
- **`ecx` (suma total):** Mostró **101**, confirmando que el segundo número era correcto.

### Solución
El input **4 101** desactiva la bomba porque cumple con:
- Índice inicial válido (`4`).
- Recorrido de 13 pasos en el arreglo.
- Suma exacta de **101** en los valores del recorrido.

## Fase Secreta: Árbol Binario

### Acceso a la fase secreta
Al resolver la **phase_3**, agregué el tercer argumento `abrete_sesamo`. Luego, completé la **phase_4** con éxito (usando `4 101`), lo que activó la `secret_phase`. Verifiqué esto con GDB al observar que el código saltaba a la dirección `0x402210` después de desactivar la fase 4.

**Comandos clave en GDB:**
```bash
break *0x402630  # phase_defused (para detectar activación)
run
# Verifiqué que se llamara a secret_phase:
x/i 0x4026dc
```

### Investigación de la fase secreta
**a. Breakpoints críticos:**
```bash
break *0x402210  # secret_phase (entrada)
break *0x4021e9  # Retorno de fun7 (para ver el valor de eax)
```

**b. Análisis del árbol binario:**
Inspeccioné los nodos desde la raíz (`n1` en `0x4f91f0`) usando:
```bash
x/3gx 0x4f91f0  # Muestra valor, hijo izquierdo, hijo derecho
```
Repetí esto para cada nodo hasta reconstruir el árbol completo.

### Estructura del árbol
```
n1 (0x4f91f0): 36 (0x24)
│
└── derecho: n22 (0x4f9230): 50 (0x32)
    │
    └── izquierda: n33 (0x4f9270): 45 (0x2d)
        │
        └── derecho: n46 (0x4f91b0): 47 (0x2f)
```

### Determinación del input correcto (47)
**a. Recorrido requerido para retornar 5:**
1. **n1 (36)**: 47 > 36 → derecha (`2 * resultado + 1`).
2. **n22 (50)**: 47 < 50 → izquierda (`2 * resultado`).
3. **n33 (45)**: 47 > 45 → derecha (`2 * resultado + 1`).
4. **n46 (47)**: ¡Valor igual! → retorna `0`.

**Cálculo final:**
```
5 = 2 * (2 * (2 * 0 + 1)) + 1
   = 2 * (2 * 1) + 1
   = 2 * 2 + 1
   = 5 ✔️
```

**b. Verificación en GDB:**
```bash
# Ejecuté con input 47:
run
# En el breakpoint de fun7 (0x4021e9), verifiqué eax:
print $eax  # Mostró 5
```

### Comandos clave para replicar
```bash
# Inspeccionar nodos:
x/3gx 0x4f91f0  # Nodo raíz
x/3gx 0x4f9230  # Nodo 50
x/3gx 0x4f9270  # Nodo 45
x/3gx 0x4f91b0  # Nodo 47

# Breakpoints:
break *0x402210  # secret_phase
break *0x4021e9  # Retorno de fun7
```

### Solución
El input **47** en la `secret_phase` asegura que `fun7` retorne **5**, desactivando la bomba. Usé GDB para validar cada paso del recorrido del árbol y confirmar que los registros cumplían con la lógica requerida.