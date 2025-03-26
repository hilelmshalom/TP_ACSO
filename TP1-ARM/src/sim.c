#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"

// Instruction field masks
#define OPCODE_MASK 0xFFF00000
#define OPCODE_SHIFT 20
#define RD_MASK 0x00000F80
#define RD_SHIFT 7
#define RN_MASK 0x0000001F
#define RN_SHIFT 0
#define RM_MASK 0x0000001F
#define RM_SHIFT 16
#define IMM_MASK 0x00000FFF
#define IMM_SHIFT 0
#define SHIFT_MASK 0x00000060
#define SHIFT_SHIFT 5
#define COND_MASK 0x0F000000
#define COND_SHIFT 24
#define ADDR_MASK 0x00000FFF
#define ADDR_SHIFT 0

// Instruction opcodes
#define OP_ADDS 0xB1000000
#define OP_SUBS 0x6B000000
#define OP_ADD 0x0B000000
#define OP_MUL 0x9B007C00
#define OP_CMP 0x6B00001F
#define OP_ANDS 0x6A000000
#define OP_EOR 0x4A000000
#define OP_ORR 0x2A000000
#define OP_B 0x14000000
#define OP_BR 0xD61F0000
#define OP_CBZ 0xB4000000
#define OP_CBNZ 0xB5000000
#define OP_LSL 0xD3400000
#define OP_LSR 0xD3400400
#define OP_LDUR 0xF8400000
#define OP_STUR 0xF8000000
#define OP_MOVZ 0xD2800000
#define OP_HLT 0xD4400000

// Helper functions
static void update_flags(int64_t result) {
    NEXT_STATE.FLAG_N = (result < 0);
    NEXT_STATE.FLAG_Z = (result == 0);
}

static int64_t get_shifted_operand(int64_t value, int shift_type, int shift_amount) {
    switch(shift_type) {
        case 0: return value; // LSL
        case 1: return value >> shift_amount; // LSR
        default: return value;
    }
}

void process_instruction() {
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    uint32_t opcode = (instruction & OPCODE_MASK) >> OPCODE_SHIFT;
    printf("opcode: %x\n", opcode);
    uint32_t rd = (instruction & RD_MASK) >> RD_SHIFT;
    uint32_t rn = (instruction & RN_MASK) >> RN_SHIFT;
    uint32_t rm = (instruction & RM_MASK) >> RM_SHIFT;
    uint32_t imm = (instruction & IMM_MASK) >> IMM_SHIFT;
    uint32_t shift = (instruction & SHIFT_MASK) >> SHIFT_SHIFT;
    uint32_t cond = (instruction & COND_MASK) >> COND_SHIFT;
    uint32_t addr = (instruction & ADDR_MASK) >> ADDR_SHIFT;

    // Initialize NEXT_STATE with CURRENT_STATE
    NEXT_STATE = CURRENT_STATE;


    // Handle HLT instruction first
    if (instruction == OP_HLT) {
        RUN_BIT = 0;
        NEXT_STATE.PC += 4;
        return;
    }
    //printf("caso adds: %x\n", OP_ADDS >> (OPCODE_SHIFT));

    // Process different instruction types
    switch(opcode) {
        case (OP_ADDS >> OPCODE_SHIFT): {
            printf("caso adds: %x\n", OP_ADDS >> (OPCODE_SHIFT));
            int64_t operand2 = get_shifted_operand(imm, shift, 12);
            int64_t result = CURRENT_STATE.REGS[rn] + operand2;
            NEXT_STATE.REGS[rd] = result;
            update_flags(result);
            break;
        }
        // case (OP_SUBS >> OPCODE_SHIFT): {
        //     int64_t operand2 = get_shifted_operand(imm, shift, 12);
        //     int64_t result = CURRENT_STATE.REGS[rn] - operand2;
        //     NEXT_STATE.REGS[rd] = result;
        //     update_flags(result);
        //     break;
        // }
        case (OP_ADD >> OPCODE_SHIFT): {
            int64_t operand2 = get_shifted_operand(imm, shift, 12);
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] + operand2;
            break;
        }
        case (OP_MUL >> OPCODE_SHIFT): {
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] * CURRENT_STATE.REGS[rm];
            break;
        }
        case (OP_CMP >> OPCODE_SHIFT): {
            int64_t operand2 = get_shifted_operand(imm, shift, 12);
            int64_t result = CURRENT_STATE.REGS[rn] - operand2;
            update_flags(result);
            break;
        }
        case (OP_ANDS >> OPCODE_SHIFT): {
            int64_t operand2 = get_shifted_operand(imm, shift, 12);
            int64_t result = CURRENT_STATE.REGS[rn] & operand2;
            NEXT_STATE.REGS[rd] = result;
            update_flags(result);
            break;
        }
        case (OP_EOR >> OPCODE_SHIFT): {
            int64_t operand2 = get_shifted_operand(imm, shift, 12);
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] ^ operand2;
            break;
        }
        case (OP_ORR >> OPCODE_SHIFT): {
            int64_t operand2 = get_shifted_operand(imm, shift, 12);
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] | operand2;
            break;
        }
        case (OP_B >> OPCODE_SHIFT): {
            int64_t offset = ((int64_t)addr << 2);
            NEXT_STATE.PC += offset;
            return;
        }
        case (OP_BR >> OPCODE_SHIFT): {
            NEXT_STATE.PC = CURRENT_STATE.REGS[rn];
            return;
        }
        case (OP_CBZ >> OPCODE_SHIFT): {
            if (CURRENT_STATE.REGS[rd] == 0) {
                int64_t offset = ((int64_t)addr << 2);
                NEXT_STATE.PC += offset;
                return;
            }
            break;
        }
        case (OP_CBNZ >> OPCODE_SHIFT): {
            if (CURRENT_STATE.REGS[rd] != 0) {
                int64_t offset = ((int64_t)addr << 2);
                NEXT_STATE.PC += offset;
                return;
            }
            break;
        }
        case (OP_LSL >> OPCODE_SHIFT): {
            NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] << imm;
            break;
        }
        // case (OP_LSR >> OPCODE_SHIFT): {
        //     NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rn] >> imm;
        //     break;
        // }
        case (OP_LDUR >> OPCODE_SHIFT): {
            uint64_t address = CURRENT_STATE.REGS[rn] + (imm << 2);
            NEXT_STATE.REGS[rd] = mem_read_32(address);
            break;
        }
        case (OP_STUR >> OPCODE_SHIFT): {
            uint64_t address = CURRENT_STATE.REGS[rn] + (imm << 2);
            mem_write_32(address, CURRENT_STATE.REGS[rd]);
            break;
        }
        case (OP_MOVZ >> OPCODE_SHIFT): {
            NEXT_STATE.REGS[rd] = imm << (shift * 16);
            break;
        }
    }

    // Update PC for next instruction (unless it was already updated by a branch)
    NEXT_STATE.PC += 4;
}

// void haltear(){
//     RUN_BIT = 0;
// }