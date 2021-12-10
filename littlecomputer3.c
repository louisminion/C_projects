/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // uint16_t
#include <stdio.h>  // FILE
#include <signal.h> // SIGINT
/* windows only */
#include <Windows.h>
#include <conio.h>  // _kbhit

HANDLE hStdin = INVALID_HANDLE_VALUE;

/*MEMORY/*
/* 65536 memory locations, each of which can store a 16-bit value */
uint16_t memory[UINT16_MAX];
/*REGISTERS (used for data the cpu is working on) 
*- 8 general purpose (R0-R7), 1 program counter (PC), and 1 condition flag (CD)*/
// Enumeration is a user defined datatype in C - assigning names to constants
// As it is, it variables in the enum are assigned values counting up from the first one - ie R_R1 = 1,... R_COUNT = 10
enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};
//  declare the array of registers of type unsigned 16-bit integer - declaring an array of size R_COUNT
uint16_t reg[R_COUNT];

//INSTRUCTION SET

// Opcode; which instruction to do. Parameters; inouts to the task being performed
// An opcode is a task the CPU knows how to do

// Each instruction is 16 bits, and the left 4 store the opcode

// Defining opcodes
enum
{
    OP_BR = 0, /*branch*/
    OP_ADD, /* add */
    OP_LD, /* load */
    OP_ST, /* store */
    OP_JSR, /* jump register */
    OP_AND, /* bitwise and */
    OP_LDR, /* load register */
    OP_STR, /* store register */
    OP_RTI, /* unused */
    OP_NOT, /* bitwise not */
    OP_LDI, /* load indirect */
    OP_STI, /* store indirect */
    OP_JMP, /* jump */
    OP_RES, /* reserved */
    OP_LEA, /* load effective address */
    OP_TRAP, /* execute trap */
};

// Condition flags - R_COND stores flags which provide info about the most recently executed calculation, 
// used to check logical conditions like if statements
// A cpu has various condition flags to signal different situations. The LC-3 has 3 flags

//  << is a bitshift operator -means shift to the left
enum
{
    FL_POS = 1 << 0, /* P */ /* Shift 1 to the left zero bits 1<<0 == 2^0 == 1 ==binary 0001 */
    FL_ZRO = 1 << 1, /* Z */ /* Shift 1 to the left 1 bit 1<<1 == 2^1 ==2 == binary 0010 */
    FL_NEG = 1 << 2, /* N */ /* Shift 1 to the left 2 bits 1 << 2 == 2^2 == binary 0100 */
};

// Sign extend - take a 5bit signed number and turn into a propperly signed 16 bit number
uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

// Any time a value is written to a register, we need to update the flags to indicate its sign
void update_flags(uint16_t r)
{
    if (reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

//  Need to write procedure for running programs and executing operations
/*
* 1. Load an instruction into memory at the address of the PC register
* 2. Increment the PC register
* 3. Look at the opcode to determine which instruction to perform
* 4. Perform the instruction using the parameters in the instruction. 
* 5. Go back to step 1.
*/ 

int main(int argc, const char* argv[])
{   if (argc < 2)
    {
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }
    for (int j = 1; j < argc; ++j)
    {
        if (!read_image(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }
  
    /* Set program counter to starting position, 0x3000 is default */
    enum {PC_START = 0x3000 }; /* setting pc-start to value 0x3000*/
    reg[R_PC] = PC_START;  /* setting the pc register to the value PC_START*/

    int running = 1; /* declaring an int called running for while loop */
    while (running)
    {
        /* fetch */
        uint16_t instr = mem_read(reg[R_PC]++); /* load the value at the program counter position, then increment the pc by one*/
        // instr - the 16 bit instruction to be performed. The first 4 bits are the opcode, the other 12 the instr
        uint16_t op = instr >> 12; /* shifting 12 bits right gives you the first 4 bits, the opcode part */
        switch (op)
        {
            case OP_ADD:
                {
            
                // an instruction; opcode (4 bits), destination register (3), source1 - the first number to add (3),
                // immediate (1) or register (0) mode bit, the depending on whether immediate or register mode is to be used, 
                //  either two empty bits and a source2 (3), or a second value embedded in the final 5 bits of the instruction itself.
                /* destination register*/
                // move to dr part, bitwise and with 7 to find register to put result in 
                // (essentially gets rid of opcode part while preserving value of dr)
                uint16_t r0 = (instr >> 9) & 0x7; /* DR is one of R0 to R7 to store the result */
                //  first operand source1
                // move to sr1, get only sr1 by anding with 7
                uint16_t r1 = (instr >> 6) & 0x7;
                /* whether we are in immediate mode */
                // move to immflag and and with 1 to get only this bit
                // anding with a binary number all 1s the size of the part of the instruction you're interested in gives you just that part
                uint16_t imm_flag = (instr >> 5) & 0x1;

                if (imm_flag)
                {
                    // move to imm5 and and with 11111 to get only those 5 bits
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    // put the value r1 + imm5 into the register at r0 (DR)
                    reg[r0] = reg[r1] + imm5;
                }
                else
                {
                    // getting sr2 from final 3 bits in instr and anding with 111
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] +reg[r2];
                    
                }
                update_flags(r0);
                }
                break;
            case OP_AND:
                {
                // Logical and, opcode 0101
                // instr; opcode, 3bit DR, SR1, immflag, then either 00 and sr2 or imm5
                //  getting dr
                uint16_t dr = (instr >> 9) & 0x7;
                uint16_t sr1 = (instr >> 6) & 0x7;
                uint16_t imm_flag = (instr >> 5) & 0x1;
                if (imm_flag)
                {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[dr] = reg[sr1] & imm5;
                } 
                update_flags(dr);
                }
                break; 
            case OP_NOT:
                {
                // Logical NOT 
                //  dr = register to store
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t SR = (instr >> 6) & 0x7;
                reg[DR] = ~reg[SR];
                update_flags(DR);
                }
                break;
            case OP_BR:
                {
                // Conditional Branch opcode 0
                // 0000,n,z,p,PCOffset9
                // increment program counter if a condition is true (as stored in the condition flags)
                //  ands nzp with reg[COND], where reg[COND] is a 
                //  3-bit value where exactly one bit is a 1.
                uint16_t nzp = (instr >> 9) & 0x7; /* the three bit code to be anded with the condition register*/
                uint16_t PCOffset9  = sign_extend(instr & 0x1FF, 9);
                if (nzp & reg[R_COND])
                {
                    reg[R_PC] += PCOffset9;
                }
                
                
                }
                break;
            case OP_JMP:
                {
                // JMP or RET (return from subroutine, special case of JMP)
                // Jumps to location specified by contents of base register
                //  PC=BaseR
                // instr- 1100, 000, BaseR, 000000 (JMP)
                // instr- 1100, 000, 111, 000000 (RET)
                // RET jumps to location specified by R_R7, which is the original location before subroutine call
                uint16_t BaseR = (instr >> 6) & 0x7;
                reg[R_PC] = BaseR;
                }
                break;
            case OP_JSR:
                {
                // Jump to subroutine
                //  Incremented PC is stored in R7 (linkage back to calling routine)
                //  PC loaded with address of first instruction of subroutine (causing jump to that address)
                //  address of subroutine is obtained from base register (if bit 11 is 0) or
                //  by sign extending bits 0 to 10 and adding to PC (if bit 11 is 1)
                reg[R_R7]= reg[R_PC];
                uint16_t offset_or_abs_flag = (instr >> 11) & 0x1;
                if (offset_or_abs_flag)
                {
                    uint16_t PCoffset11 = sign_extend(instr & 0x7FF, 11);
                    reg[R_PC] += PCoffset11;
                }
                else
                {
                    uint16_t BR = (instr >> 6) & 0x7;
                    reg[R_PC] = BR;
                }
                }
                break;
            case OP_LD:
                {
                //  Load
                // First get an address from bits 0 to 8 added to PC.
                //  The load the contents of this address into DR.
                //  0010, DR, PCoffset9
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t PCOffset9  = sign_extend(instr & 0x1FF, 9);
                reg[DR] = mem_read(reg[R_PC]+PCOffset9);
                update_flags(DR);
                }
                break;
            case OP_LDI:
                {
                //  Load indirect
                //  Compute address from 0 to 8 bits added to PC
                //  load from memory this value, then load from memory at the address given by that value
                // load into DR
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t PCOffset9  = sign_extend(instr & 0x1FF, 9);
                reg[DR] = mem_read(mem_read(reg[R_PC]+PCOffset9));
                update_flags(DR);
                }
                break;
            case OP_LDR:
                {
                //  Load Base + offset
                //  Compute address from first 5 bits then add to address at BR
                //  Load into DR.
                //  Set condition codes
                uint16_t DR = (instr >> 9) & 0x7;
                uint16_t BR = (instr >> 6) & 0x7;
                uint16_t offset6 = sign_extend(instr & 0x3F, 6);
                reg[DR] = mem_read(BR+offset6);
                update_flags(DR); 
                }
                break;
            case OP_LEA:
            case OP_ST:
            case OP_STI:
            case OP_STR:
            case OP_TRAP:
            case OP_RES:
            case OP_RTI:
            default:
                break;

        }
        

    }
}




// WINDOWS STANDARDS (not relevant to learning about VMS)
// uint16_t check_key()
// {
//     return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
// }

// DWORD fdwMode, fdwOldMode;

// void disable_input_buffering()
// {
//     hStdin = GetStdHandle(STD_INPUT_HANDLE);
//     GetConsoleMode(hStdin, &fdwOldMode); /* save old mode */
//     fdwMode = fdwOldMode
//             ^ ENABLE_ECHO_INPUT  /* no input echo */
//             ^ ENABLE_LINE_INPUT; /* return when one or
//                                     more characters are available */
//     SetConsoleMode(hStdin, fdwMode); /* set new mode */
//     FlushConsoleInputBuffer(hStdin); /* clear buffer */
// }

// void restore_input_buffering()
// {
//     SetConsoleMode(hStdin, fdwOldMode);
// }

// signal(SIGINT, handle_interrupt);
// disable_input_buffering();

// restore_input_buffering();
