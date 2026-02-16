/**
    This class represents the overall CPU, which includes the execution engine and any additional components (e.g., coprocessors, interrupt handling).
 */
 #include "cpu_ee.h"

const uint32_t halted_pc = 0x3fff0000; // A special PC value that indicates the CPU is halted. This is not a valid instruction address.

class CPU {
private:
    CPU_ExecutionEngine execution_engine; // The execution engine responsible for executing instructions
    Memory *mem;
public:
    CPU(Memory* mem, uint32_t initial_pc);

    ExecutionResult step();
    ExecutionResult stepUntilPC(uint32_t target_pc);
    uint32_t getRegister(int reg) const;
    uint32_t getCPSR() const;
    bool isHalted() const;
};