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


//struct cpu *c: pass in a pointer to your virtual CPU. The function needs to know which CPU it's looking at.
//cpu_register reg: this is the enum value the parser spit out (e.g., REG_RAX).
//uint64_t* (Return Type):it returns a memory address.
//c->rax refers to the actual value sitting inside the struct (e.g., 0).
//& symbol means "Get the Address Of."

//writing &(c->rax), we are telling the compiler: "Do not give me the value inside RAX. Give me the memory address 0x1000 where RAX is stored."
uint64_t* get_reg_ptr(struct cpu *c, cpu_register reg) {
    switch (reg) {
            case REG_RAX: return &(c->rax);
            case REG_RBX: return &(c->rbx);
            case REG_RCX: return &(c->rcx);
            case REG_RDX: return &(c->rdx);
            case REG_RSP: return &(c->rsp);
            case REG_RBP: return &(c->rbp);
            case REG_RIP: return &(c->rip);
            default: return NULL;
    }
}

//the x86-64 architecture, instructions like ADD and SUB automatically update these flags based on the mathematical result. (Note: MOV does not update flags it just moves data).
void update_flags(struct cpu *c, uint64_t result) {
    //zero flag, true if the result of the operation is exactly 0.
    c->zf=(result==0);
    /*  explanation of the above one line code.
    if (result==0){
        c->zf=true;
    }else {
        c->zf=false;
    }
    */

    //sign flag, true if the result is negative. In a 64-bit integer, the highest bit (the 63rd bit) is the "sign bit". If it is 1, the number is negative.
    //bit-shift the result 63 places to the right, and use bitwise AND.
    /*
    shifting right (>>) by 63 places, we are pushing all the other 63 bits off the right edge and disappearing them,
    leaving Bit 63 sitting perfectly in the 1st position (Bit 0).

    & 1 acts as a mask. It forces every other bit in the number to become 0, ensuring that you isolate only that single bit that you just shifted into the first position.
    if the result was negative, c->sf becomes 1 (True).
    if the result was positive, c->sf becomes 0 (False).
     */
    c->sf=(result>>63)&1;
}

void execute_instruction(struct cpu *c, struct instruction inst) {
    uint64_t *dest = get_reg_ptr(c, inst.dest_reg); //get_reg_ptr() to get the memory address of the destination register.
    //if HLT was called or an invalid register was passed,
    // dest might be NULL. We should handle HLT before trying to use 'dest'.
    if (inst.op==HLT) {
        printf("HLT: Halting the CPU\n");
        exit(0);
    }
    if (dest==NULL) {
        printf("ERROR: Invalid destination register\n");
        return;
    }

    //need the actual integer value to add/subtract/move
    uint64_t source_val=0;
    if (inst.op2_type==OPERAND_TYPE_RAW) {
        source_val=inst.num;
    }else if (inst.op2_type == OPERAND_TYPE_REGISTER) {
        //get the pointer to the source register
        uint64_t *src_ptr = get_reg_ptr(c, inst.reg);
        source_val = *src_ptr; //dereference the pointer to extract the actual number inside it
    }


    //overwriting the destination register, we do not do dest = source_val. we must dereference it: *dest = source_val. If it's ADD, we add source_val to *dest.
    switch (inst.op) {
        case MOV:
            *dest = source_val;
            break;
        case ADD:
            *dest += source_val;
            update_flags(c, *dest);
            break;
        case SUB:
            *dest -= source_val;
            update_flags(c, *dest);
            break;
        default:
            printf("Error: Unknown opcode.\n");
            break;
    }
}
