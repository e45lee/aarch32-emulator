/**
    This class represents the overall CPU, which includes the execution engine and any additional components (e.g., coprocessors, interrupt handling).
 */
class CPU {
private:
    CPU_ExecutionEngine execution_engine; // The execution engine responsible for executing instructions
public:
    CPU(Memory* mem, uint32_t initial_pc);

    void step();
};