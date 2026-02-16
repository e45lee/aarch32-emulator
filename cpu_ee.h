/**
    cpu_ee.h
    Headers for the CPU subsystem of the AArch32 emulator.
     This includes:
        - a basic CPU Execution Engine class that provides methods for fetching, decoding, and executing instructions.
            It also includes registers and a program counter (PC) to keep track of the current instruction being executed.
 */

#ifndef CPU_EE_H
#define CPU_EE_H

#include "instruction.h"

// Forward declaration
class Memory;

/**
    This class represents the execution engine of the CPU, responsible for executing instruction and managing
    the CPU state (registers, program counter, etc.). It interacts with the memory subsystem to read/write data.
    The decode cycle is handled here, but the fetch-decode-execute loop is managed by the CPU class.
 */
class CPU_ExecutionEngine {
protected:
    uint32_t registers[16]; // General-purpose registers R0-R15 (R13 is the stack register, R14 is the link register, R15 is the program counter)
    uint32_t cpsr; // Current Program Status Register

    bool wrotePC;   // Flag to indicate if the instruction wrote to the PC (for branching)
    uint32_t newPC; // Temporary variable to hold the new PC value if the instruction writes to it

    Memory* memory; // Pointer to the memory subsystem

private:
    void executeDataProcessing(AArch32Instruction instr);
    void executeLoadStore(AArch32Instruction instr);
    void executeBranch(AArch32Instruction instr);
    void executeBranchExchange(AArch32Instruction instr);

    // Helper methods
    uint32_t applyShift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, bool& carry_out);
    void updateFlags(uint32_t result, bool carry, bool overflow);
public:
    CPU_ExecutionEngine(Memory* mem, uint32_t initial_pc);

    void executeInstruction(AArch32Instruction instr);
    bool shouldExecuteInstruction(AArch32Instruction instr);

    uint32_t getNextPC() const;
    bool didWritePC() const;
    void setNextPC();
};



#endif