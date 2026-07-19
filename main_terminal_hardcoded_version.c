/*
 In this version of code, if you want try this, update the char *program[]={...} with the asm code in main,
 and the program will run on terminal.
 */


#include "include/cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//function that takes a string and returns opcode enum.
//function to recognize these string commands.
opcode string_to_opcode(const char* str) {
    if (str==NULL) return INVALID_OPCODE;
    if (strcmp(str,"MOV")==0) return MOV;
    if (strcmp(str,"ADD")==0) return ADD;
    if (strcmp(str,"SUB")==0) return SUB;
    if (strcmp(str,"HLT")==0) return HLT;
    if (strcmp(str,"CMP")==0) return CMP;
    if (strcmp(str,"JMP")==0) return JMP;
    if (strcmp(str,"JE")==0) return JE;
    if (strcmp(str,"JNE")==0) return JNE;
    return INVALID_OPCODE;
}

//function for the registers. It should take a string like "RAX" and return REG_RAX.
cpu_register string_to_reg(const char* str) {
    if (str==NULL) return INVALID_REGISTER;
    if (strcmp(str,"RAX")==0) return REG_RAX;
    if (strcmp(str,"RBX")==0) return REG_RBX;
    if (strcmp(str,"RCX")==0) return REG_RCX;
    if (strcmp(str,"RDX")==0) return REG_RDX;
    if (strcmp(str,"RSP")==0) return REG_RSP;
    if (strcmp(str,"RBP")==0) return REG_RBP;
    if (strcmp(str,"RIP")==0) return REG_RIP;
    return INVALID_REGISTER;
}

//parser to break the input and map to the nums to execute instructions.
struct instruction parse_line(char* line) {
    struct instruction inst={0}; //blank struct to store.

    char* token1 = strtok(line, " ,"); //pass the string, and the delimiters " ," (space and comma)
    inst.op=string_to_opcode(token1); //finding opcode
    if (inst.op==HLT) {return inst;} //if the opcode is HLT, the instruction takes no operands

    char* token2 = strtok(NULL, " ,");//pass NULL to tell strtok to keep going from where it stopped.
    inst.dest_reg=string_to_reg(token2); //storing the dest.

    char* token3 = strtok(NULL, " ,"); //it could be "RBX" or it could be "10".
    if (token3==NULL) {
        // its a 2-token instruction like "JMP 5"
        // the destination register doesn't matter, but the number is in token2!
        inst.op2_type = OPERAND_TYPE_RAW;
        inst.num = strtoll(token2, NULL, 10);
    }else {
        // its a 3-token instruction like "MOV RAX, 10"
        cpu_register parsed_reg=string_to_reg(token3);
        if (parsed_reg==INVALID_REGISTER) { //returns INVALID_REGISTER, then the token must be a raw number
            inst.op2_type=OPERAND_TYPE_RAW;
            inst.num=strtoll(token3, NULL, 10); //convert a string like "10" into a 64-bit integer, strtoll (string to long long).
        }else {
            inst.op2_type=OPERAND_TYPE_REGISTER;
            inst.reg=parsed_reg;
        }
    }
    return inst;
}

struct cpu* init_cpu() {
    //allocating the struct itself on the heap
    struct cpu *c=malloc(sizeof(struct cpu));
    if (!c) {
        printf("Error allocating memory for cpu\n");
        exit(1);
    }

    //allocating 1MB of virtual RAM on the heap
    c->memory=malloc(1024*1024);
    if (!c->memory) {
        printf("Memory allocation failed\n");
        free(c);
        exit(1);
    }

    //clean and zero out all the registers to ensure clean state.
    c->rax = 0; c->rbx = 0; c->rcx = 0; c->rdx = 0;
    c->rsp = 0; c->rbp = 0; c->rip = 0;
    c->zf = false; c->cf = false; c->sf = false; c->of = false;

    return c;
}
int main(){
    struct cpu *c=init_cpu();
    char *program[] = {
        "MOV RAX, 5",
        "SUB RAX, 1",
        "CMP RAX, 0",
        "JNE 1",
        "HLT"
    };

    while (true) {
        char buffer[64];
        //need a temporary string buffer because strtok destroys the string it parses
        strcpy(buffer,program[c->rip]);//fetch
        struct instruction inst=parse_line(buffer);//decode
        c->rip++;//increment, before execute, so if the execute step is a JMP, the jump address isn't immediately overwritten on the next cycle.
        execute_instruction(c,inst);//execute
        printf("RAX:%lu\n",c->rax);//long unsigned
        printf("RIP:%lu\n",c->rip);
    }
return 0;
}

