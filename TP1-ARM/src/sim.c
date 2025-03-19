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
    int opcode = (current_instruction & 0xFFF00000) >> 20; // Masking and then shifting to get the correct 12-bit opcodeint opcode = current_instruction >> 20;

    //print for debugging
    printf("current_instruction: %x\n", current_instruction);
    printf("opcode: %x\n", opcode);
    //avanzar el PC
    if(current_instruction == 0xd4400000){
        //HALTEAR
        printf("HALTEAR\n");
        RUN_BIT = 0;
    }
    NEXT_STATE.PC += 4;
}

// void haltear(){
//     RUN_BIT = 0;
// }