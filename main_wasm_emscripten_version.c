#include "include/cpu.h"
#include <emscripten.h>
#include <string.h>
#include <stdlib.h> // Needed for free() and strtoll()
#include <stdio.h>


struct cpu *global_cpu = NULL;
char user_program[100][64];

// PARSER FUNCTIONS
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
    if (strcmp(str,"PUSH")==0) return PUSH;
    if (strcmp(str,"POP")==0) return POP;
    if (strcmp(str,"CALL")==0) return CALL;
    if (strcmp(str,"RET")==0) return RET;
    return INVALID_OPCODE;
}

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

struct instruction parse_line(char* line) {
    struct instruction inst={0};
    char* token1 = strtok(line, " ,");
    inst.op=string_to_opcode(token1);
    if (inst.op==HLT || inst.op==RET) {return inst;}

    char* token2 = strtok(NULL, " ,");
    inst.dest_reg=string_to_reg(token2);

    char* token3 = strtok(NULL, " ,");
    if (token3==NULL) {
        inst.op2_type = OPERAND_TYPE_RAW;
        inst.num = strtoll(token2, NULL, 10);
    } else {
        cpu_register parsed_reg=string_to_reg(token3);
        if (parsed_reg==INVALID_REGISTER) {
            inst.op2_type=OPERAND_TYPE_RAW;
            inst.num=strtoll(token3, NULL, 10);
        } else {
            inst.op2_type=OPERAND_TYPE_REGISTER;
            inst.reg=parsed_reg;
        }
    }
    return inst;
}

struct cpu* init_cpu() {
    //allocating the struct itself on the heap
    struct cpu *c = malloc(sizeof(struct cpu));
    if (!c) {
        printf("Error allocating memory for cpu\n");
        exit(1);
    }

    //allocating 1MB of virtual RAM on the heap
    c->memory = malloc(1024 * 1024);
    if (!c->memory) {
        printf("Memory allocation failed\n");
        free(c);
        exit(1);
    }

    //clean and zero out all the registers to ensure clean state.
    // NOTE: RSP is initialized to the top of the stack (grows downward)
    c->rax = 0; c->rbx = 0; c->rcx = 0; c->rdx = 0;
    c->rsp = (1024 * 1024) - 8;
    c->rbp = 0; c->rip = 0;
    c->zf = false; c->cf = false; c->sf = false; c->of = false;

    return c;
}

//WEBASSEMBLY API FUNCTIONS (No printf allowed here, JS handles the UI!)
EMSCRIPTEN_KEEPALIVE
void init_emulator() {
    if (global_cpu != NULL) {
        free(global_cpu->memory);
        free(global_cpu);
    }
    global_cpu = init_cpu();
    
    //clear out the old program memory
    for(int i = 0; i < 100; i++) {
        user_program[i][0] = '\0';
    }
}

EMSCRIPTEN_KEEPALIVE
void load_instruction(int index, const char* instruction) {
    if (index >= 0 && index < 100) {
        strncpy(user_program[index], instruction, 63);
        user_program[index][63] = '\0'; //ensure null termination
    }
}

EMSCRIPTEN_KEEPALIVE
int step_emulator() {
    char buffer[64];
    strcpy(buffer, user_program[global_cpu->rip]);
    
    struct instruction inst = parse_line(buffer);
    
    if (inst.op == HLT) {
        return 0; //tell JS that the CPU halted
    }
    
    global_cpu->rip++;
    execute_instruction(global_cpu, inst);
    
    return 1; //tell JS to keep going
}

//getters so JS can read the CPU state and update the HTML DOM
EMSCRIPTEN_KEEPALIVE int get_rax() { return (int)global_cpu->rax; }
EMSCRIPTEN_KEEPALIVE int get_rbx() { return (int)global_cpu->rbx; }
EMSCRIPTEN_KEEPALIVE int get_rcx() { return (int)global_cpu->rcx; }
EMSCRIPTEN_KEEPALIVE int get_rdx() { return (int)global_cpu->rdx; }
EMSCRIPTEN_KEEPALIVE int get_rsp() { return (int)global_cpu->rsp; }
EMSCRIPTEN_KEEPALIVE int get_rbp() { return (int)global_cpu->rbp; }
EMSCRIPTEN_KEEPALIVE int get_rip() { return (int)global_cpu->rip; }
EMSCRIPTEN_KEEPALIVE int get_zf()  { return global_cpu->zf ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE int get_cf()  { return global_cpu->cf ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE int get_sf()  { return global_cpu->sf ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE int get_of()  { return global_cpu->of ? 1 : 0; }

EMSCRIPTEN_KEEPALIVE
uint8_t* get_memory_ptr() {
    if (global_cpu == NULL) return NULL;
    return global_cpu->memory;
}