#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
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