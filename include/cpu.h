#ifndef CPU_H
#define CPU_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

//enums
//in a real CPU, instructions are raw binary (e.g., 0x48 0x89 0xC3). Since we are building a text-based emulator first, our input will be a string like "MOV RAX, 10"
//to the compiler, it's just an array of characters: ['M', 'O', 'V', '\0']. We need to translate that text into a structured C object that our execution engine can evaluate with a simple switch statement.
typedef enum{
    INVALID_OPCODE=-1,
    MOV,
    ADD,
    SUB,
    HLT,
    CMP, // the code for CMP to JNE was added during building the control flow. The further explanations and details are added in the parser function.
    JMP,
    JE,
    JNE
}opcode;

//need a way to represent the registers as integers so we aren't comparing strings later.
typedef enum{
    INVALID_REGISTER,
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSP,
    REG_RBP,
    REG_RIP,
}cpu_register;

//tells us whether to read 'reg' or 'int'
typedef enum {
    OPERAND_TYPE_REGISTER,
    OPERAND_TYPE_RAW  // Raw integer
} operand_type;

//structs
struct cpu {
    //general purpose regs 64 bits unsigned.
    uint64_t rax; //accumulator primarily used for arithmetic operations and function return values.
    uint64_t rbx; //base primarily used as a pointer to base data like arrays
    uint64_t rcx; //counter commonly used as loop counter and for shoft/rotate instructions.
    uint64_t rdx; //data reg used in extended arithmetic, (multiplication and division) and I/O operations.
    uint64_t rsp; //stack ptr points to the top of the call stack.
    uint64_t rbp; //base ptr used to reference parameters and local variables on the stack.
    uint64_t rip; //instruction ptr holds the address of the next instruction.

    //boolean flags
    bool zf; //zero flag set if the last operation resulted in 0.
    bool cf; //carry flag set if an operation caused an arithmetic carry/borrow.
    bool sf; //sign flag set if the result was negative.
    bool of; //overflow flag set if signed arithmetic overflowed.

    uint8_t *memory; //allocate it on the Heap using malloc() when we initialize the CPU. The heap is designed to hold massive amounts of data.
};

struct instruction {
    //field for opcode, destination register, value (register or int)
    opcode op;
    cpu_register dest_reg;
    operand_type op2_type;

    //union looks exactly like a struct, but with one massive difference: all of its members share the exact same memory location. The size of a union is simply the size of its largest member.
    union {
        cpu_register reg; //used if op2_type == OPERAND_TYPE_REGISTER
        int64_t num; //used if op2_type == OPERAND_TYPE_RAW
    };
};

struct cpu* init_cpu();
void execute_instruction(struct cpu *c, struct instruction inst);
uint64_t* get_reg_ptr(struct cpu *c, cpu_register reg);
void update_flags(struct cpu *c, uint64_t result);
#endif
