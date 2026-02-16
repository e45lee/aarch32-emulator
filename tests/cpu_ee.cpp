/**
    Tests for the CPU execution engine of the AArch32 emulator.
    This includes tests for:
        - Data processing instructions (arithmetic, logical, move, compare)
        - Load/Store instructions (word and byte)
        - Branch instructions (B, BL, BX)
        - Conditional execution
        - Flag updates
 */

#include "../cpu_ee.h"
#include "../memory.h"
#include <catch2/catch_test_macros.hpp>

// Helper function to access CPU registers (we'll need to add getters)
class TestableExecutionEngine : public CPU_ExecutionEngine {
public:
    TestableExecutionEngine(Memory* mem, uint32_t initial_pc) : CPU_ExecutionEngine(mem, initial_pc) {}

    uint32_t getRegister(int reg) const { return registers[reg]; }
    uint32_t getCPSR() const { return cpsr; }
    void setRegister(int reg, uint32_t value) { registers[reg] = value; }
    void setCPSR(uint32_t value) { cpsr = value; }

    using CPU_ExecutionEngine::executeInstruction;
    using CPU_ExecutionEngine::shouldExecuteInstruction;
};

TEST_CASE("Data Processing - MOV immediate", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // MOV R0, #42 (always execute, no flags update)
    // Encoding: CCCC 00 I 1101 S Rn Rd imm12
    // 1110 00 1 1101 0 0000 0000 000000101010
    AArch32Instruction instr;
    instr.raw = 0xE3A0002A; // MOV R0, #42

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 42);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - ADD with flags", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up registers
    cpu.setRegister(1, 10);
    cpu.setRegister(2, 20);

    // ADD R0, R1, R2 with S bit set
    // Encoding: CCCC 00 I opcode S Rn Rd operand2
    // 1110 00 0 0100 1 0001 0000 00000000 0010
    AArch32Instruction instr;
    instr.raw = 0xE0910002; // ADD R0, R1, R2, S=1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 30);

    // Check Z flag is clear
    REQUIRE((cpu.getCPSR() & (1 << 30)) == 0);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - SUB with carry and overflow", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 5);
    cpu.setRegister(2, 10);

    // SUB R0, R1, R2 with S bit set (5 - 10 = -5)
    // This should set N flag and clear C flag
    AArch32Instruction instr;
    instr.raw = 0xE0510002; // SUB R0, R1, R2, S=1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == (uint32_t)-5);

    // N flag should be set (negative result)
    REQUIRE((cpu.getCPSR() & (1 << 31)) != 0);
    // C flag should be clear (borrow occurred)
    REQUIRE((cpu.getCPSR() & (1 << 29)) == 0);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - AND logical operation", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0xFF00FF00);
    cpu.setRegister(2, 0xF0F0F0F0);

    // AND R0, R1, R2
    AArch32Instruction instr;
    instr.raw = 0xE0010002; // AND R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xF000F000);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - ORR logical operation", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0x0F0F0F0F);
    cpu.setRegister(2, 0xF0F0F0F0);

    // ORR R0, R1, R2
    AArch32Instruction instr;
    instr.raw = 0xE1810002; // ORR R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xFFFFFFFF);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - MVN (move not)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0x00000000);

    // MVN R0, R1 (move NOT of R1 to R0)
    AArch32Instruction instr;
    instr.raw = 0xE1E00001; // MVN R0, R1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xFFFFFFFF);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - CMP sets flags but doesn't write result", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 10);
    cpu.setRegister(2, 10);
    cpu.setRegister(0, 999); // Set R0 to a known value

    // CMP R1, R2 (compare, should set Z flag since they're equal)
    AArch32Instruction instr;
    instr.raw = 0xE1510002; // CMP R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // R0 should be unchanged
    REQUIRE(cpu.getRegister(0) == 999);

    // Z flag should be set
    REQUIRE((cpu.getCPSR() & (1 << 30)) != 0);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - Shifted operand", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 10);
    cpu.setRegister(2, 5);

    // ADD R0, R1, R2, LSL #2 (R0 = 10 + (5 << 2) = 10 + 20 = 30)
    // Encoding with shift: Rm=2, shift_type=00(LSL), shift_imm=2
    AArch32Instruction instr;
    instr.raw = 0xE0810102; // ADD R0, R1, R2, LSL #2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 30);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Load/Store - STR word", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(0, 0x12345678);
    cpu.setRegister(1, 0x1000); // Base address

    // STR R0, [R1, #4] - Store R0 to address R1+4
    AArch32Instruction instr;
    instr.raw = 0xE5810004; // STR R0, [R1, #4]

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // Check that the word was written correctly (little-endian)
    REQUIRE(mem.readByte(0x1004) == 0x78);
    REQUIRE(mem.readByte(0x1005) == 0x56);
    REQUIRE(mem.readByte(0x1006) == 0x34);
    REQUIRE(mem.readByte(0x1007) == 0x12);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Load/Store - LDR word", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up memory with test data (little-endian)
    mem.writeByte(0x2000, 0xAA);
    mem.writeByte(0x2001, 0xBB);
    mem.writeByte(0x2002, 0xCC);
    mem.writeByte(0x2003, 0xDD);

    cpu.setRegister(1, 0x2000);

    // LDR R0, [R1] - Load word from address in R1
    AArch32Instruction instr;
    instr.raw = 0xE5910000; // LDR R0, [R1]

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xDDCCBBAA);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Load/Store - STRB byte", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(0, 0x12345678);
    cpu.setRegister(1, 0x3000);

    // STRB R0, [R1] - Store byte from R0 to address in R1
    AArch32Instruction instr;
    instr.raw = 0xE5C10000; // STRB R0, [R1]

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(mem.readByte(0x3000) == 0x78);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Load/Store - LDRB byte", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    mem.writeByte(0x4000, 0xAB);
    cpu.setRegister(1, 0x4000);

    // LDRB R0, [R1] - Load byte from address in R1
    AArch32Instruction instr;
    instr.raw = 0xE5D10000; // LDRB R0, [R1]

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xAB);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Load/Store - Pre-indexed with write-back", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(0, 0x11223344);
    cpu.setRegister(1, 0x5000);

    // STR R0, [R1, #8]! - Store with pre-index and write-back
    AArch32Instruction instr;
    instr.raw = 0xE5A10008; // STR R0, [R1, #8]!

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // Check the data was written to R1+8
    REQUIRE(mem.readByte(0x5008) == 0x44);

    // Check that R1 was updated (write-back)
    REQUIRE(cpu.getRegister(1) == 0x5008);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Load/Store - Post-indexed", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(0, 0xAABBCCDD);
    cpu.setRegister(1, 0x6000);

    // STR R0, [R1], #12 - Store with post-index
    AArch32Instruction instr;
    instr.raw = 0xE481000C; // STR R0, [R1], #12

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // Data should be written to original R1 address
    REQUIRE(mem.readByte(0x6000) == 0xDD);

    // R1 should be updated after the store
    REQUIRE(cpu.getRegister(1) == 0x600C);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Branch - B (unconditional branch)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // B #16 (branch forward by 16 bytes)
    // offset = 4 (shifted left by 2 = 16)
    AArch32Instruction instr;
    instr.raw = 0xEA000004; // B #16

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.didWritePC() == true);
    // PC + 8 + offset = 0x1000 + 8 + 16 = 0x1018
    REQUIRE(cpu.getNextPC() == 0x1018);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
}

TEST_CASE("Branch - BL (branch with link)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x2000);

    // BL #32 (branch with link)
    AArch32Instruction instr;
    instr.raw = 0xEB000008; // BL #32

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // R14 (LR) should contain return address (PC + 4)
    REQUIRE(cpu.getRegister(14) == 0x2004);

    // PC should be updated
    REQUIRE(cpu.didWritePC() == true);
    REQUIRE(cpu.getNextPC() == 0x2028); // 0x2000 + 8 + 32
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
}

TEST_CASE("Branch - Negative offset", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x3000);

    // B #-8 (branch backward)
    // offset = -2 (0xFFFFFE in 24-bit, shifted left by 2 = -8)
    AArch32Instruction instr;
    instr.raw = 0xEAFFFFFE; // B #-8

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.didWritePC() == true);
    // PC + 8 - 8 = 0x3000
    REQUIRE(cpu.getNextPC() == 0x3000);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
}

TEST_CASE("Branch - BX (branch and exchange)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x4000);

    cpu.setRegister(5, 0x5000);

    // BX R5
    // Encoding: 1110 000 1 0010 1111 1111 1111 0001 Rm
    AArch32Instruction instr;
    instr.raw = 0xE12FFF15; // BX R5

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.didWritePC() == true);
    REQUIRE(cpu.getNextPC() == 0x5000);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
}

TEST_CASE("Conditional Execution - EQ (equal)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set Z flag (equal condition)
    cpu.setCPSR(1 << 30);

    cpu.setRegister(1, 100);

    // MOVEQ R0, R1 (execute only if equal/zero flag set)
    AArch32Instruction instr;
    instr.raw = 0x01A00001; // MOVEQ R0, R1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 100);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Conditional Execution - NE (not equal), condition false", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set Z flag (condition will be false for NE)
    cpu.setCPSR(1 << 30);

    cpu.setRegister(0, 999);
    cpu.setRegister(1, 100);

    // MOVNE R0, R1 (execute only if not equal, i.e., Z flag clear)
    AArch32Instruction instr;
    instr.raw = 0x11A00001; // MOVNE R0, R1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // R0 should be unchanged because condition is false
    REQUIRE(cpu.getRegister(0) == 999);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Conditional Execution - GT (greater than)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set flags for GT: Z=0, N=V
    // Let's set N=0, V=0, Z=0
    cpu.setCPSR(0);

    cpu.setRegister(1, 42);

    // MOVGT R0, R1 (execute only if greater than)
    AArch32Instruction instr;
    instr.raw = 0xC1A00001; // MOVGT R0, R1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 42);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Conditional Execution - MI (minus/negative)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set N flag
    cpu.setCPSR(1 << 31);

    cpu.setRegister(1, 123);

    // MOVMI R0, R1 (execute only if negative)
    AArch32Instruction instr;
    instr.raw = 0x41A00001; // MOVMI R0, R1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 123);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - Zero result sets Z flag", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 100);
    cpu.setRegister(2, 100);

    // SUBS R0, R1, R2 (100 - 100 = 0, should set Z flag)
    AArch32Instruction instr;
    instr.raw = 0xE0510002; // SUB R0, R1, R2 with S=1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 0);
    REQUIRE((cpu.getCPSR() & (1 << 30)) != 0); // Z flag set
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - Overflow detection", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Test signed overflow: 0x7FFFFFFF + 1 = 0x80000000 (overflow)
    cpu.setRegister(1, 0x7FFFFFFF);
    cpu.setRegister(2, 1);

    // ADDS R0, R1, R2
    AArch32Instruction instr;
    instr.raw = 0xE0910002; // ADD R0, R1, R2 with S=1

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 0x80000000);
    // V flag should be set (overflow occurred)
    REQUIRE((cpu.getCPSR() & (1 << 28)) != 0);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - RSB (reverse subtract)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 10);
    cpu.setRegister(2, 30);

    // RSB R0, R1, R2 (R0 = R2 - R1 = 30 - 10 = 20)
    AArch32Instruction instr;
    instr.raw = 0xE0610002; // RSB R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 20);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - BIC (bit clear)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0xFFFFFFFF);
    cpu.setRegister(2, 0xFF00FF00);

    // BIC R0, R1, R2 (R0 = R1 & ~R2)
    AArch32Instruction instr;
    instr.raw = 0xE1C10002; // BIC R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0x00FF00FF);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - EOR (exclusive OR)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0xAAAAAAAA);
    cpu.setRegister(2, 0x55555555);

    // EOR R0, R1, R2
    AArch32Instruction instr;
    instr.raw = 0xE0210002; // EOR R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xFFFFFFFF);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}

TEST_CASE("Data Processing - TEQ (test equivalence)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0x12345678);
    cpu.setRegister(2, 0x12345678);
    cpu.setRegister(0, 999); // Should remain unchanged

    // TEQ R1, R2 (test if equal via XOR, should set Z flag)
    AArch32Instruction instr;
    instr.raw = 0xE1310002; // TEQ R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 999); // R0 unchanged
    REQUIRE((cpu.getCPSR() & (1 << 30)) != 0); // Z flag set (values are equal)
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
}
