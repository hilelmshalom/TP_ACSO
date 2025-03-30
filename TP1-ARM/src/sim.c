#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"

/* Instrucciones principales */
#define OP_ADD_EXT_REG      0x8B000000  // 10001011001 (bits 31-21)
#define OP_ADD_IMM          0x91000000  // 10010001 (bits 31-24)
#define OP_ADDS_EXT_REG     0xAB000000  // 10101011001 (bits 31-21)
#define OP_ADDS_IMM         0xB1000000  // 10110001 (bits 31-24)
#define OP_MUL              0x9B007C00  // 10011011000 (bits 31-21)
#define OP_SUBS_EXT_REG     0xEB000000  // 11101011001 (bits 31-21)
#define OP_SUBS_IMM         0xF1000000  // 11110001 (bits 31-24)

/* Comparaciones (alias de SUBS) */
// #define OP_CMP_EXT_REG      OP_SUBS_EXT_REG  // Mismo opcode que SUBS_EXT_REG
// #define OP_CMP_IMM          OP_SUBS_IMM      // Mismo opcode que SUBS_IMM

/* Operaciones lógicas */
#define OP_ANDS             0xEA000000  // 11101010 (bits 31-24)
#define OP_EOR              0xCA000000  // 11001010 (bits 31-24)
#define OP_ORR              0xAA000000  // 10101010 (bits 31-24)

/* Saltos */
#define OP_B                0x14000000  // 100101 (bits 31-26)
#define OP_BR               0xD61F0000  // 11010110000 (bits 31-21)
#define OP_B_COND           0x54000000  // 01010100 (bits 31-24)

/* Shifts */
#define OP_LSL              0xD3400000  // 110100110 (bits 31-23) + N=0
#define OP_LSR              0xD3400000  // 110100110 (bits 31-23) + N=1

/* Load/Store */
#define OP_STUR             0xF8000000  // 11111000000 (bits 31-21)
#define OP_STURB            0x38000000  // 00111000000 (bits 31-21)
#define OP_STURH            0x78000000  // 01111000000 (bits 31-21)
#define OP_LDUR             0xF8400000  // 11111000010 (bits 31-21)
#define OP_LDURB            0x38400000  // 00111000010 (bits 31-21)
#define OP_LDURH            0x78400000  // 01111000010 (bits 31-21)

/* Otras */
#define OP_MOVZ             0xD2800000  // 110100101 (bits 31-23)
#define OP_CBZ              0xB4000000  // 10110100 (bits 31-24)
#define OP_CBNZ             0xB5000000  // 10110101 (bits 31-24)
#define OP_HLT              0xD4000000  // 11010100 (bits 31-24)

/* Máscaras para bits variables */
#define MASK_ADDS_SHIFT     0x00C00000  // bits 23-22 (shift)
#define MASK_ADDS_IMM12     0x003FFC00  // bits 21-10 (imm12)
#define MASK_B_COND_IMM19   0x00FFFFE0  // bits 23-5 (imm19)
#define MASK_B_IMM26        0x03FFFFFF  // bits 25-0 (imm26)
#define MASK_LDST_IMM9      0x003FF000  // bits 20-12 (imm9)
#define MASK_REG_RD         0x0000001F  // bits 4-0
#define MASK_REG_RN         0x000003E0  // bits 9-5
#define MASK_REG_RM         0x001F0000  // bits 20-16

/* Máscaras para opcodes de longitud conocida */
#define MASK_OP_21         0xFFE00000  // bits 31-21
#define MASK_OP_24         0xFF000000  // bits 31-24
#define MASK_OP_26         0xFC000000  // bits 31-26
#define MASK_OP_23         0xFF800000  // bits 31-23

// #define COND_EQ  0x0  // Equal (Z == 1)
// #define COND_NE  0x1  // Not Equal (Z == 0)
// #define COND_GT  0xA  // Greater Than (Z == 0 && N == 0)
// #define COND_LT  0xB  // Less Than (N == 1)
// #define COND_GE  0xC  // Greater Than or Equal (N == 0)
// #define COND_LE  0xD  // Less Than or Equal (Z == 1 || N == 1)

// prueba
#define COND_EQ  0  // Equal (Z == 1)
#define COND_NE  1  // Not Equal (Z == 0)
#define COND_GT  2  // Greater Than (Z == 0 && N == 0)
#define COND_LT  3  // Less Than (N == 1)
#define COND_GE  4  // Greater Than or Equal (N == 0)
#define COND_LE  5  // Less Than or Equal (Z == 1 || N == 1)

typedef enum {
    /* Instrucciones principales */
    INST_ADD_REG,
    INST_ADD_IM,
    INST_ADDS_REG,
    INST_ADDS_IM,
    INST_MUL,
    INST_SUBS_REG,
    INST_SUBS_IM,
    
    /* Comparaciones (alias de SUBS) */
    INST_CMP_REG,
    INST_CMP_IM,
    
    /* Operaciones lógicas */
    INST_ANDS,
    INST_EOR,
    INST_ORR,
    
    /* Saltos */
    INST_B,
    INST_BR,
    INST_B_COND,
    
    /* Shifts */
    INST_LSL,
    INST_LSR,
    
    /* Load/Store */
    INST_STUR,
    INST_STURB,
    INST_STURH,
    INST_LDUR,
    INST_LDURB,
    INST_LDURH,
    
    /* Otras */
    INST_MOVZ,
    INST_CBZ,
    INST_CBNZ,
    INST_HLT,
    
    INST_UNKNOWN
} InstructionType;

typedef struct {
    InstructionType type;
    uint16_t opcode;
    uint8_t Rd;
    uint8_t Rn;
    uint8_t Rm;
    uint8_t Rt;
    int64_t imm;
    uint32_t cond;
    uint8_t shift;
    // Otros campos según necesidad
} DecodedInstruction;

// Helper functions
static void update_flags(int64_t result) {
    NEXT_STATE.FLAG_N = (result < 0);
    NEXT_STATE.FLAG_Z = (result == 0);
}

static uint32_t shift_and_mask(uint32_t number, int shift_amount, int mask_bits) {
    uint32_t shifted = number >> shift_amount;
    uint32_t mask = (1U << mask_bits) - 1; // Create a mask with the first `mask_bits` set to 1
    return shifted & mask;
}

// Add this function to sim.c or an appropriate file
int32_t sign_extend(int32_t value, int bits) {
    int32_t mask = 1 << (bits - 1);
    return (value ^ mask) - mask;
}



DecodedInstruction decode_instruction(uint32_t instruction) {
    DecodedInstruction inst = {INST_UNKNOWN};
    
    //─────────────────────── ARITMÉTICAS/COMPARACIÓN ──────────────────────
    if ((instruction & MASK_OP_21) == OP_ADD_EXT_REG) {
        inst.type = INST_ADD_REG;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    else if ((instruction & MASK_OP_24) == OP_ADD_IMM) {
        inst.type = INST_ADD_IM;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_ADDS_IMM12) >> 10;
        inst.shift = (instruction & MASK_ADDS_SHIFT) >> 22;
    }
    else if ((instruction & MASK_OP_21) == OP_ADDS_EXT_REG) {
        inst.type = INST_ADDS_REG;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    else if ((instruction & MASK_OP_24) == OP_ADDS_IMM) {
        inst.type = INST_ADDS_IM;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_ADDS_IMM12) >> 10;
        inst.shift = (instruction & MASK_ADDS_SHIFT) >> 22;
    }
    else if ((instruction & MASK_OP_21) == OP_MUL) {
        inst.type = INST_MUL;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    else if ((instruction & MASK_OP_21) == OP_SUBS_EXT_REG) {
        inst.type = (instruction & MASK_REG_RD) == 31 ? INST_CMP_REG : INST_SUBS_REG;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    else if ((instruction & MASK_OP_24) == OP_SUBS_IMM) {
        inst.type = (instruction & MASK_REG_RD) == 31 ? INST_CMP_IM : INST_SUBS_IM;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_ADDS_IMM12) >> 10;
        inst.shift = (instruction & MASK_ADDS_SHIFT) >> 22;
    }
    // HALT
    else if ((instruction & MASK_OP_24) == OP_HLT) {
        inst.type = INST_HLT;
    }

    //────────────────────────────── LÓGICAS ──────────────────────────────
    else if ((instruction & OP_ANDS) == OP_ANDS) {
        inst.type = INST_ANDS;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    else if ((instruction & OP_EOR) == OP_EOR) {
        inst.type = INST_EOR;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    else if ((instruction & OP_ORR) == OP_ORR) {
        inst.type = INST_ORR;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.Rm = (instruction & MASK_REG_RM) >> 16;
    }
    //────────────────────────────── SALTOS ───────────────────────────────
    else if ((instruction & MASK_OP_26) == OP_B) {
        inst.type = INST_B;
        inst.imm = (instruction & MASK_B_IMM26) << 2;
        inst.imm = sign_extend(inst.imm, 28);  // Sign-extend para saltos relativos
    }
    else if ((instruction & MASK_OP_21) == OP_BR) {
        inst.type = INST_BR;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
    }
    else if ((instruction & MASK_OP_24) == OP_B_COND) {
        inst.type = INST_B_COND;
        inst.cond = instruction & 0x0F;
        inst.imm = (instruction & MASK_B_COND_IMM19) >> 5;
        inst.imm = sign_extend(inst.imm << 2, 21);  // Sign-extend de 21 bits
    }
    //───────────────────────────── LOAD/STORE ─────────────────────────────
    else if ((instruction & MASK_OP_21) == OP_STUR) {
        inst.type = INST_STUR;
        inst.Rt = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_LDST_IMM9) >> 12;
    }
    else if ((instruction & MASK_OP_21) == OP_LDUR) {
        inst.type = INST_LDUR;
        inst.Rt = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_LDST_IMM9) >> 12;
    }
    else if ((instruction & MASK_OP_21) == OP_STURB) {
        inst.type = INST_STURB;
        inst.Rt = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_LDST_IMM9) >> 12;
    }
    else if ((instruction & MASK_OP_21) == OP_LDURB) {
        inst.type = INST_LDURB;
        inst.Rt = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_LDST_IMM9) >> 12;
    }
    else if ((instruction & MASK_OP_21) == OP_STURH) {
        inst.type = INST_STURH;
        inst.Rt = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_LDST_IMM9) >> 12;
    }
    else if ((instruction & MASK_OP_21) == OP_LDURH) {
        inst.type = INST_LDURH;
        inst.Rt = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction & MASK_LDST_IMM9) >> 12;
    }
    
    //─────────────────────────────── OTROS ────────────────────────────────
    else if ((instruction & MASK_OP_23) == OP_MOVZ) {
        inst.type = INST_MOVZ;
        inst.Rd = instruction & MASK_REG_RD;
        inst.imm = (instruction >> 5) & 0xFFFF;
    }
    else if ((instruction & MASK_OP_23) == OP_LSL) {
        inst.type = INST_LSL;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction >> 10) & 0x3F;
    }
    else if ((instruction & MASK_OP_23) == OP_LSR) {
        inst.type = INST_LSR;
        inst.Rd = instruction & MASK_REG_RD;
        inst.Rn = (instruction & MASK_REG_RN) >> 5;
        inst.imm = (instruction >> 10) & 0x3F;
    }


    else if ((instruction & MASK_OP_24) == OP_CBZ) {
        inst.type = INST_CBZ;
        inst.Rt = instruction & MASK_REG_RD;
        inst.imm = (instruction >> 5) & 0x7FFFF;
    }

    else if ((instruction & MASK_OP_24) == OP_CBNZ) {
        inst.type = INST_CBNZ;
        inst.Rt = instruction & MASK_REG_RD;
        inst.imm = (instruction >> 5) & 0x7FFFF;
    }
    else {
        inst.type = INST_UNKNOWN;
        printf("Instrucción desconocida: %08X\n", instruction);
    }
    return inst;
}

void process_instruction() {
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    DecodedInstruction inst = decode_instruction(instruction);
    // muestra en salida los detalles de la instrucción decodificada
    printf("Instruction: %08X\n", inst.type);


        switch (inst.type) {
        case INST_ADD_REG:
            // Implementar lógica de ADD_REG
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] + CURRENT_STATE.REGS[inst.Rm];
            printf("@ %d : %d + %d\n", inst.Rd, inst.Rn, inst.Rm);

            break;
        case INST_ADD_IM:
            // Implementar lógica de ADD_IM
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] + (inst.imm << inst.shift);
            printf("@[%d] : [%d] + %ld\n", inst.Rd, inst.Rn, inst.imm);
            break;

        case INST_ADDS_REG:
            // Implementar lógica de ADDS_REG
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] + CURRENT_STATE.REGS[inst.Rm];
            update_flags(NEXT_STATE.REGS[inst.Rd]);
            printf("@ %d : %d + %d\n", inst.Rd, inst.Rn, inst.Rm);
            break;

        case INST_ADDS_IM:
            // Implementar lógica de ADDS_IM
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] + (inst.imm << inst.shift);
            update_flags(NEXT_STATE.REGS[inst.Rd]);
            printf("@[%d] : [%d] + %ld\n", inst.Rd, inst.Rn, inst.imm);

            break;

        case INST_MUL:
            // Implementar lógica de MUL
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] * CURRENT_STATE.REGS[inst.Rm];
            printf("@ %d : %d x %d\n", inst.Rd, inst.Rn, inst.Rm);

            break;
        
        case INST_CMP_REG:
            printf("Comparando %ld - %ld\n", CURRENT_STATE.REGS[inst.Rn], CURRENT_STATE.REGS[inst.Rm]);
            int64_t result = CURRENT_STATE.REGS[inst.Rn] - CURRENT_STATE.REGS[inst.Rm];
            update_flags(result);

        case INST_SUBS_REG:
            // Resta
            printf("Restando %ld - %ld\n", CURRENT_STATE.REGS[inst.Rn], CURRENT_STATE.REGS[inst.Rm]);
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] - CURRENT_STATE.REGS[inst.Rm];
            update_flags(NEXT_STATE.REGS[inst.Rd]);
            break;
        
        case INST_CMP_IM:
            // Comparación
            printf("Comparando %ld - %ld\n", CURRENT_STATE.REGS[inst.Rn], (inst.imm << inst.shift));
            result = CURRENT_STATE.REGS[inst.Rn] - (inst.imm << inst.shift);
            update_flags(result);
            break;
            
        case INST_SUBS_IM:
            // Resta
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] - (inst.imm << inst.shift);
            update_flags(NEXT_STATE.REGS[inst.Rd]);
            break;

        //------------------------- LÓGICAS --------------------------------
        case INST_ANDS:
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] & CURRENT_STATE.REGS[inst.Rm];
            update_flags(NEXT_STATE.REGS[inst.Rd]);
            break;

        case INST_EOR:
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] ^ CURRENT_STATE.REGS[inst.Rm];
            break;

        case INST_ORR:
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] | CURRENT_STATE.REGS[inst.Rm];
            break;

        //------------------------- SALTOS --------------------------------
        case INST_B:
            NEXT_STATE.PC += inst.imm;
            break;
        
        case INST_BR:
            NEXT_STATE.PC = CURRENT_STATE.REGS[inst.Rn];
            break;

        case INST_B_COND:
            // Evaluate condition and update PC
            switch (inst.cond) {
                case COND_EQ: // BEQ (Branch if Equal)
                    if (CURRENT_STATE.FLAG_Z) { // Z flag is set
                        NEXT_STATE.PC += inst.imm;
                    }
                    break;
                case COND_NE: // BNE (Branch if Not Equal)
                    if (!CURRENT_STATE.FLAG_Z) { // Z flag is not set
                        NEXT_STATE.PC += inst.imm;
                    }
                    break;
                case COND_GT: // BGT (Branch if Greater Than)
                    if (!CURRENT_STATE.FLAG_Z && !CURRENT_STATE.FLAG_N) { // Z=0 and N=0
                        NEXT_STATE.PC += inst.imm;
                    }
                    break;
                case COND_LT: // BLT (Branch if Less Than)
                    if (CURRENT_STATE.FLAG_N) { // N flag is set
                        NEXT_STATE.PC += inst.imm;
                    }
                    break;
                case COND_GE: // BGE (Branch if Greater Than or Equal)
                    if (!CURRENT_STATE.FLAG_N) { // N=0
                        NEXT_STATE.PC += inst.imm;
                    }
                    break;
                case COND_LE: // BLE (Branch if Less Than or Equal)
                    if (CURRENT_STATE.FLAG_Z || CURRENT_STATE.FLAG_N) { // Z=1 or N=1
                        NEXT_STATE.PC += inst.imm;
                    }
                    break;
                default:
                    printf("Unknown condition code: %u\n", inst.cond);
                    break;
            }
            break;

        //------------------------- LOAD/STORE ------------------------------
        case INST_LSL:
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] << inst.imm;
            break;

        case INST_LSR:
            NEXT_STATE.REGS[inst.Rd] = CURRENT_STATE.REGS[inst.Rn] >> inst.imm;
            break;

        case INST_STUR:
            // STUR: M[X2 + imm] = X1
            mem_write_32(0x10000000 + CURRENT_STATE.REGS[inst.Rn] + inst.imm, CURRENT_STATE.REGS[inst.Rt]);
            break;
        
        case INST_STURB:
            {
                uint32_t address = 0x10000000 + CURRENT_STATE.REGS[inst.Rn] + inst.imm;
                uint32_t aligned_value = mem_read_32(address & ~0x3); // Read the aligned 32-bit word
                uint8_t byte_value = CURRENT_STATE.REGS[inst.Rt] & 0xFF; // Extract the least significant 8 bits
                int byte_offset = address & 0x3; // Determine the byte offset within the 32-bit word

                // Insert the byte into the correct position
                aligned_value &= ~(0xFF << (byte_offset * 8)); // Clear the target byte
                aligned_value |= (byte_value << (byte_offset * 8)); // Set the target byte

                mem_write_32(address & ~0x3, aligned_value); // Write back the modified 32-bit word
            }
            break;

        case INST_STURH:
            {
                uint32_t address = 0x10000000 + CURRENT_STATE.REGS[inst.Rn] + inst.imm;
                uint32_t aligned_value = mem_read_32(address & ~0x3); // Read the aligned 32-bit word
                uint16_t halfword_value = CURRENT_STATE.REGS[inst.Rt] & 0xFFFF; // Extract the least significant 16 bits
                int halfword_offset = (address & 0x3) >> 1; // Determine the halfword offset (0 or 1)

                // Insert the halfword into the correct position
                aligned_value &= ~(0xFFFF << (halfword_offset * 16)); // Clear the target halfword
                aligned_value |= (halfword_value << (halfword_offset * 16)); // Set the target halfword

                mem_write_32(address & ~0x3, aligned_value); // Write back the modified 32-bit word
            }
            break;

        case INST_LDUR:
            // LDUR: X1 = M[X2 + imm]
            NEXT_STATE.REGS[inst.Rt] = mem_read_32(0x10000000 + CURRENT_STATE.REGS[inst.Rn] + inst.imm);
            break;

        case INST_LDURB:
            // LDURB: X1 = 56'b0, M[X2 + imm](7:0)
            {
                uint32_t address = 0x10000000 + CURRENT_STATE.REGS[inst.Rn] + inst.imm;
                uint32_t aligned_value = mem_read_32(address & ~0x3); // Read the aligned 32-bit word
                int byte_offset = address & 0x3; // Determine the byte offset within the 32-bit word
                uint8_t byte_value = (aligned_value >> (byte_offset * 8)) & 0xFF; // Extract the byte
                NEXT_STATE.REGS[inst.Rt] = (uint64_t)byte_value; // Zero-extend to 64 bits
            }
            break;

        case INST_LDURH:
            // LDURH: X1 = 48'b0, M[X2 + imm](15:0)
            {
                uint32_t address = 0x10000000 + CURRENT_STATE.REGS[inst.Rn] + inst.imm;
                uint32_t aligned_value = mem_read_32(address & ~0x3); // Read the aligned 32-bit word
                int halfword_offset = (address & 0x3) >> 1; // Determine the halfword offset (0 or 1)
                uint16_t halfword_value = (aligned_value >> (halfword_offset * 16)) & 0xFFFF; // Extract the halfword
                NEXT_STATE.REGS[inst.Rt] = (uint64_t)halfword_value; // Zero-extend to 64 bits
            }
            break;

        case INST_MOVZ:
            // MOVZ: Xd = imm << (shift * 16)
            NEXT_STATE.REGS[inst.Rd] = inst.imm << (inst.shift * 16);
            break;

        case INST_CBZ:
            // CBZ: Branch to label if X3 (Rt) is 0
            if (CURRENT_STATE.REGS[inst.Rt] == 0) {
            NEXT_STATE.PC += inst.imm;
            }
            break;

        case INST_CBNZ:
            // CBNZ: Branch to label if X3 (Rt) is not 0
            if (CURRENT_STATE.REGS[inst.Rt] != 0) {
            NEXT_STATE.PC += inst.imm;
            }
            break;

        // ----- HALT -----
        case INST_HLT:
            // Detener la simulación
            printf("Simulación detenida por instrucción HLT\n");
            RUN_BIT = 0; // Detener la simulación
            break;
        default:
            // Instrucción no implementada
            printf("Instrucción no implementada\n");
            break;
        }
    // Actualizar PC para la siguiente instrucción
    NEXT_STATE.PC += 4;
    // Actualizar el estado actual
    CURRENT_STATE = NEXT_STATE;
        
    }
    // Actualizar el estado de la memoria
    // mem_write_32(CURRENT_STATE.PC, CURRENT_STATE.REGS[0]); // Ejemplo
    // Actualizar el estado de los registros
    // mem_write_32(CURRENT_STATE.REGS[0], CURRENT_STATE.REGS[1]); // Ejemplo
    // Actualizar los flags
    // update_flags(CURRENT_STATE.REGS[0]); // Ejemplo
    // Actualizar el estado de la memoria