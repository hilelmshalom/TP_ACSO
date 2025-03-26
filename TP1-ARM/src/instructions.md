
# I304 TP 1 - Simulador CPU

**Universidad de San Andrés**  
**I301 - Arquitectura de Computadoras y Sistemas Operativos**  

**Profesor:** Daniel Veiga  
**Ayudantes:**  
- Nicolás Romero  
- Juan Carlos Suárez  
- Tomás Agustín Chimenti  
- Joaquín Torres  
- Tobías Moraut  
- Santiago Pierini  

## Fechas de entrega
- **Entrega:** 27/03/2025 - 11:59 PM  
- **Entrega tarde (máxima nota 8):** 30/03/2025 - 11:59 PM  

## Repositorio  
[GitHub - SPI-Udesa/TP_ACSO](https://github.com/SPI-Udesa/TP_ACSO.git)  

---

## Introducción  
Este trabajo práctico se basa en un ejercicio del curso "Computer Architecture" de la Universidad de Chicago.  
El objetivo es desarrollar un programa en **C** que simule la ejecución de instrucciones del conjunto **ARMv8**.  
Este simulador modelará el comportamiento de cada instrucción, permitiendo correr programas escritos en ARMv8 y observar sus resultados.  

El simulador deberá recibir como entrada un archivo con instrucciones en **hexadecimal**, ejecutarlas y actualizar el estado del CPU, incluyendo **registros, memoria y flags**.  

---

## Estructura del Simulador  
El simulador se divide en dos componentes principales:
1. **El Shell**  
2. **El Simulador**  

Los archivos están organizados de la siguiente manera en el directorio `/src`:  
- `shell.c` y `shell.h` → Implementan el Shell (No modificar).  
- `sim.c` → Implementación del simulador (Aquí va tu código).  
- **Otros archivos pueden ser agregados**, pero deben incluirse en el `Makefile`.

### 1. El Shell  
El shell proporciona comandos para controlar la ejecución del simulador.  

#### Comandos disponibles:  
- **`go`** → Corre el programa hasta encontrar `HLT`.  
- **`run <n>`** → Ejecuta `n` instrucciones.  
- **`mdump <low> <high>`** → Muestra el contenido de la memoria entre `<low>` y `<high>`.  
- **`rdump`** → Muestra el contenido de los registros y flags (`X0-X31`, `PC`, `N`, `Z`).  
- **`input reg_num reg_val`** → Asigna el valor `reg_val` al registro `reg_num`.  
- **`?`** → Muestra los comandos disponibles.  
- **`quit`** → Salir del shell.  

Para ejecutar el shell:  
```bash
./sim archivo.hex
```
Esto cargará el programa en memoria y generará un archivo `dumpsim` con la información de ejecución.

---

### 2. El Simulador  
El simulador ejecuta cada instrucción **leyéndola de la memoria** y **modificando el estado del CPU** según el **ISA de ARMv8**.  

#### **Estructura del estado del CPU**
```c
#define ARM_REGS 32  

typedef struct CPU_State {  
  uint64_t PC;          // Contador de programa  
  int64_t REGS[ARM_REGS];  // Registros X0-X31  
  int FLAG_N;  // Flag negativo  
  int FLAG_Z;  // Flag cero  
} CPU_State;  

CPU_State STATE_CURRENT, STATE_NEXT;  
int RUN_BIT;  
```
> **Nota:** Solo se implementan los flags **N y Z**. Los flags **Carry (C) y Overflow (V) siempre son 0**.  

#### **Acceso a memoria**  
Las siguientes funciones están disponibles en `shell.c`/`shell.h`:  
```c
uint32_t mem_read_32(uint64_t address);  
void mem_write_32(uint64_t address, uint32_t value);  
```
La memoria en ARM **es Little Endian** y direcciona a nivel de **byte**.  

#### **Ciclo de ejecución**  
Cada instrucción se ejecuta en la función `process_instruction()` dentro de `sim.c`.  
El ciclo del simulador funciona de la siguiente manera:  
```c
CURRENT_STATE = NEXT_STATE;
```
Por lo que **todas las modificaciones al estado del CPU deben hacerse en `NEXT_STATE`**.  

---

## La Tarea  
El archivo `sim.c` contiene la función vacía:  
```c
void process_instruction();
```
Debes **completar esta función** para ejecutar las instrucciones de ARMv8.  

### **Pasos para ejecutar una instrucción**  
1. **Buscar y decodificar la instrucción** → Identificar el tipo de instrucción y extraer sus componentes.  
2. **Ejecutar la instrucción** → Modificar los registros, memoria y flags según corresponda.  

### **Consejos Importantes**  
✔ **Escribir tests** para cada instrucción.  
✔ **No modificar `shell.c` ni `shell.h`**.  
✔ **Verificar resultados con `ref_sim`** ejecutando `rdump` y `mdump` tras cada instrucción.  

---

## **Simulador de referencia (`ref_sim`)**  
El repositorio contiene `ref_sim`, un simulador de referencia que **debe producir los mismos resultados** que el tuyo.  
Para testear tu implementación, usa programas en `/inputs` y compila de assembly a hexadecimal con `asm2hex`.  

```bash
cd src
make
./sim test.hex
```
Para limpiar la compilación:  
```bash
make clean
```
---

## **Instrucciones a Implementar**  
+ Implementar las variantes de 64 bits de las instrucciones
+ Program Counter de 32 bits

### **Instrucciones Aritméticas**  
- `ADDS (Extended Register, Immediate)`
- `SUBS (Extended Register, Immediate)`
- `ADD (Extended Register, Immediate)`
- `MUL`

### **Comparación**  
- `CMP (Extended Register, Immediate)`

### **Operaciones Lógicas**  
- `ANDS`
- `EOR`
- `ORR`

### **Saltos**  
- `B`  
- `BR`  
- `BEQ`, `BNE`, `BGT`, `BLT`, `BGE`, `BLE` (Condicionales)  
- `CBZ`, `CBNZ`

### **Desplazamientos**  
- `LSL (Immediate)`
- `LSR (Immediate)`

### **Carga y Almacenamiento**  
- `LDUR`, `LDURB`, `LDURH`  
- `STUR`, `STURB`, `STURH`  

### **Movimientos**  
- `MOVZ`

### **Instrucción de Finalización**  
- `HLT`

---

## **Especificaciones Importantes**  
- `XZR (X31)` es un registro que **siempre contiene ceros**.  
- Flags `Z` y `N` deben **actualizarse correctamente** en instrucciones aritméticas.  
- Instrucciones `ADDS` y `SUBS` **deben soportar `shift == 01` (imm12 << 12 bits)**.  
- **La memoria comienza en `0x10000000` y tiene tamaño `0x00100000`**.  

---
