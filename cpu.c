#include "include/cpu.h"
#include <stdio.h>
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



/*
 a real program is a sequential list (an array) of instructions.
 a real processor, the CPU looks at the RIP register (Instruction Pointer).
    1. it fetches the instruction at the index RIP.
    2. it increments RIP by 1.
    3. it executes the instruction.
    4. it repeats.
control flow (loops and if-statements) is literally just manually changing the RIP register so the CPU reads a different part of the array.
*/

/* Introducing CMP, JMP
before a CPU can jump, it has to compare two things.
x86-64, the CMP (Compare) instruction is identical to SUB (Subtract). It subtracts operand 2 from operand 1 and updates the flags (zf, sf, etc.).
only difference: CMP throws the mathematical result. It does not overwrite the destination register. It only exists to set the flags for the next instruction.

once the flags are set, the CPU can jump.
we will use absolute line numbers for our jumps right now (e.g. JMP 5 means jump to instruction index 5).
    1. JMP (Jump): unconditional. always sets c->rip = source_val.
    2. JE (Jump if Equal): checks the zero flag. If we did CMP RAX, 10 and RAX was indeed 10, the subtraction resulted in 0, so zf is true. JE looks at zf. If zf == true, it sets c->rip = source_val.
    3. JNE (Jump if Not Equal): the exact opposite. If zf == false, it jumps.
 */

void execute_instruction(struct cpu *c, struct instruction inst) {
    uint64_t temp_var; //temporary variable to store / hold values.
    uint64_t *dest = get_reg_ptr(c, inst.dest_reg); //get_reg_ptr() to get the memory address of the destination register.
    //if HLT was called or an invalid register was passed,
    // dest might be NULL. We should handle HLT before trying to use 'dest'.
    if (inst.op==HLT) {
        printf("HLT: Halting the CPU\n");
        exit(0);
    }
    //allowing dest to be NULL only if the instruction is a Jump,
    //cause in JMP like JMP 5, the destination will be parsed as 5 but 5 is an integer we need to handle that.
    if (dest==NULL && inst.op != JMP && inst.op != JE && inst.op != JNE && inst.op != CALL && inst.op != RET) {
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
        case CMP:
            temp_var=*dest-source_val;
            update_flags(c, temp_var);
            break;
        case JMP:
            c->rip=source_val;
            break;
        case JE:
            if (c->zf ==true) {
                c->rip=source_val;
            }
            break;
        case JNE:
            if (c->zf ==false) {
                c->rip=source_val;
            }
            break;
        case PUSH:
            //grow the stack downward by 8 bytes
            c->rsp -= 8;
            //treat the memory at c->rsp as a 64-bit integer pointer
            uint64_t *stack_write_ptr = (uint64_t*)&(c->memory[c->rsp]);
            *stack_write_ptr = *dest; //write the value into RAM
            break;
        case POP:
            //get a 64-bit pointer to the current top of the stack
            uint64_t *stack_read_ptr = (uint64_t*)&(c->memory[c->rsp]);
            //read the value and put it into the destination register
            // (dest is already calculated at the top of execute_instruction!)
            *dest = *stack_read_ptr;
            //shrink the stack (moves the pointer back up)
            c->rsp += 8;
            break;
        case CALL: {
            //push the current RIP to the stack
            c->rsp -= 8;
            uint64_t *stack_write_ptr = (uint64_t*)&(c->memory[c->rsp]);
            *stack_write_ptr = c->rip;

            //jump to the target address
            c->rip = source_val;
            break;
        }
        case RET: {
            //pop the saved return address off the stack
            uint64_t *stack_read_ptr = (uint64_t*)&(c->memory[c->rsp]);
            //set RIP to that saved address
            c->rip = *stack_read_ptr;
            //shrink the stack
            c->rsp += 8;
            break;
        }
        default:
            printf("Error: Unknown opcode.\n");
            break;
    }
}