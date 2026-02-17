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
#include <set>

// Constants
const uint32_t halted_pc = 0x3fff0000; // A special PC value that indicates the CPU is halted. This is not a valid instruction address.  Initially stored in R14.

// Forward declaration
class Memory;

/**
    Structure to hold information about what an instruction wrote during execution.
*/
struct ExecutionResult {
    bool wroteRegister;            // True if a register was written
    std::set<int> registersWritten; // Set of register numbers that were written (0-15)
    bool wroteCPSR;                // True if CPSR was updated
    bool wroteMemory;              // True if memory was written
    uint32_t memoryAddress;        // Memory address that was written
    uint32_t memorySize;           // Size of memory write (1 for byte, 4 for word)

    ExecutionResult() : wroteRegister(false), wroteCPSR(false),
                       wroteMemory(false), memoryAddress(0), memorySize(0) {}
};

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
    ExecutionResult executeDataProcessing(AArch32Instruction instr);
    ExecutionResult executeLoadStore(AArch32Instruction instr);
    ExecutionResult executeBranch(AArch32Instruction instr);
    ExecutionResult executeBranchExchange(AArch32Instruction instr);
    ExecutionResult executeMultiply(AArch32Instruction instr);

    // Helper methods
    uint32_t applyShift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, bool& carry_out);
    void updateFlags(uint32_t result, bool carry, bool overflow);
public:
    CPU_ExecutionEngine(Memory* mem, uint32_t initial_pc);

    ExecutionResult executeInstruction(AArch32Instruction instr);
    bool shouldExecuteInstruction(AArch32Instruction instr);

    uint32_t getNextPC() const;
    bool didWritePC() const;
    void setNextPC();

    uint32_t getRegister(int reg) const;
    uint32_t getCPSR() const;
    void setRegister(int reg, uint32_t value);
};



#endif