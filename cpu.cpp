/*
    Implementation of the CPU class.
*/

#include "cpu.h"
#include "memory.h"

CPU::CPU(Memory* mem, uint32_t initial_pc) : mem(mem), execution_engine(mem, initial_pc) {
}

ExecutionResult CPU::step() {
    // Fetch instruction
    uint32_t pc = execution_engine.getRegister(15); // PC is R15
    if (pc == halted_pc) {
        // If PC is at the halted address, we consider the CPU to be halted and do not execute further instructions.
        return ExecutionResult(); // Return an empty result indicating no execution
    }

    uint32_t raw_instruction = mem->readByte(pc) | mem->readByte(pc + 1) << 8 | mem->readByte(pc + 2) << 16 | mem->readByte(pc + 3) << 24;

    // Decode instruction
    AArch32Instruction instr;
    instr.raw = raw_instruction;

    // Execute instruction
    ExecutionResult execResult = execution_engine.executeInstruction(instr);

    // Update PC for next instruction
    execution_engine.setNextPC();

    return execResult;
}

ExecutionResult CPU::stepUntilPC(uint32_t target_pc) {
    ExecutionResult result;
    do {
        result = step();
    } while (execution_engine.getRegister(15) != target_pc && execution_engine.getRegister(15) != halted_pc);
    return result;
}

uint32_t CPU::getRegister(int reg) const {
    return execution_engine.getRegister(reg);
}

void CPU::setRegister(int reg, uint32_t value) {
    execution_engine.setRegister(reg, value);
}

uint32_t CPU::getCPSR() const {
    return execution_engine.getCPSR();
}

bool CPU::isHalted() const {
    return execution_engine.getRegister(15) == halted_pc;
}