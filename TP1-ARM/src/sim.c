#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. 
     * */
    int current_instruction = mem_read_32(CURRENT_STATE.PC);
    int opcode = current_instruction >> 26;
    int rn = (current_instruction >> 16) & 0x1F;
    int rd = (current_instruction >> 12) & 0x1F;
    int rm = current_instruction & 0x1F;
    int imm = current_instruction & 0xFF;
    int imm2 = current_instruction & 0xFFF;
    int imm3 = current_instruction & 0x7FF;
    int imm4 = current_instruction & 0xFFFF;
    int imm5 = current_instruction & 0x7FF;
    int imm6 = current_instruction & 0x3FF;
    int imm7 = current_instruction & 0x7FF;
    int imm8 = current_instruction & 0xFF;
    int imm9 = current_instruction & 0x1FF;
    //print for debugging
    printf("current_instruction: %x\n", current_instruction);
    printf("opcode: %x\n", opcode);
    //printf("rn: %x\n", rn);
    //printf("rd: %x\n", rd);
}
