**Longitud de los códigos de operación (opcode) y agrupación por formato:**

### **1. Opcode de 11 bits (31-21):**
- **Formato:**  
  `opcode(11) | Rm(5) | ... | Rn(5) | Rd(5)`  
  **Instrucciones:**  
  - `ADD (ext reg)`  
  - `ADDS (ext reg)`  
  - `SUBS (ext reg)`  
  - `CMP (ext reg)`  
  - `MUL`  
  - `STUR`  
  - `STURB`  
  - `STURH`  
  - `LDUR`  
  - `LDURB`  
  - `LDURH`  

---

### **2. Opcode de 8 bits (31-24):**
- **Formato A:**  
  `opcode(8) | shift(2) | imm(12) | Rn(5) | Rd(5)`  
  **Instrucciones:**  
  - `ADD (imm)`  
  - `ADDS (imm)`  
  - `SUBS (imm)`  
  - `CMP (imm)`  

- **Formato B:**  
  `opcode(8) | shift(2) | N(1) | Rm(5) | imm(6) | Rn(5) | Rd(5)`  
  **Instrucciones:**  
  - `ANDS (shifted reg)`  
  - `EOR (shifted reg)`  
  - `ORR (shifted reg)`  

- **Formato C:**  
  `opcode(8) | imm(19) | Rt(5)`  
  **Instrucciones:**  
  - `CBZ`  
  - `CBNZ`  

- **Formato D:**  
  `opcode(8) | imm(19) | cond(4)`  
  **Instrucción:**  
  - `B cond`  

---

### **3. Opcode de 9 bits (31-23):**
- **Formato:**  
  `opcode(9) | N(1) | immr(6) | imms(6) | Rn(5) | Rd(5)`  
  **Instrucciones:**  
  - `LSL (imm)`  
  - `LSR (imm)`  
  - `MOVZ`  

---

### **4. Opcode de 6 bits (31-26):**
- **Formato:**  
  `opcode(6) | imm26(26)`  
  **Instrucción:**  
  - `B`  

---

### **5. Opcode de 11 bits (31-21) con formato único:**
- **Formato:**  
  `opcode(11) | [campos restantes específicos]`  
  **Instrucciones:**  
  - `BR`: `opcode(11) | [bits 20-16 vacíos] | Rn(5)`  
  - `CMP (ext reg)`: `Rd fijo en 11111`  
  - `CMP (imm)`: `Rd fijo en 11111`  

---

### **Resumen de longitudes de opcode:**
| Longitud | Instrucciones asociadas |  
|----------|-------------------------|  
| **11 bits** | ADD/ADDS/SUBS/CMP (ext reg), MUL, STUR/LDUR (variantes), BR |  
| **8 bits**  | ADD/ADDS/SUBS/CMP (imm), ANDS/EOR/ORR (shifted reg), CBZ/CBNZ, B cond |  
| **9 bits**  | LSL/LSR (imm), MOVZ |  
| **6 bits**  | B |  

**Nota:** Algunas instrucciones comparten opcode pero difieren en campos fijos (ej: `CMP (ext reg)` usa `Rd=11111`, mientras `SUBS (ext reg)` no).