#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

//in a real CPU, instructions are raw binary (e.g., 0x48 0x89 0xC3). Since we are building a text-based emulator first, our input will be a string like "MOV RAX, 10"
//to the compiler, it's just an array of characters: ['M', 'O', 'V', '\0']. We need to translate that text into a structured C object that our execution engine can evaluate with a simple switch statement.
typedef enum{
    INVALID_OPCODE=-1,
    MOV,
    ADD,
    SUB,
    HLT
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

//function that takes a string and returns opcode enum.
opcode string_to_opcode(const char* str) {
    if (str==NULL) return INVALID_OPCODE;
    if (strcmp(str,"MOV")==0) return MOV;
    if (strcmp(str,"ADD")==0) return ADD;
    if (strcmp(str,"SUB")==0) return SUB;
    if (strcmp(str,"HLT")==0) return HLT;
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
    cpu_register parsed_reg=string_to_reg(token3);
    if (parsed_reg==INVALID_REGISTER) { //returns INVALID_REGISTER, then the token must be a raw number
        inst.op2_type=OPERAND_TYPE_RAW;
        inst.num=strtoll(token3, NULL, 10); //convert a string like "10" into a 64-bit integer, strtoll (string to long long).
    }else {
        inst.op2_type=OPERAND_TYPE_REGISTER;
        inst.reg=parsed_reg;
    }
    return inst;
}