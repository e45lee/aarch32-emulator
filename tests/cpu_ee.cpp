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
    ExecutionResult result = cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 42);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false); // S bit not set
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
    ExecutionResult result = cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 30);

    // Check Z flag is clear
    REQUIRE((cpu.getCPSR() & (1 << 30)) == 0);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == true); // S bit is set
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
    ExecutionResult result = cpu.executeInstruction(instr);

    // R0 should be unchanged
    REQUIRE(cpu.getRegister(0) == 999);

    // Z flag should be set
    REQUIRE((cpu.getCPSR() & (1 << 30)) != 0);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    // CMP doesn't write to a register
    REQUIRE(result.wroteRegister == false);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == true); // CMP always updates flags (implicit S bit)
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
    ExecutionResult result = cpu.executeInstruction(instr);

    // Check that the word was written correctly (little-endian)
    REQUIRE(mem.readByte(0x1004) == 0x78);
    REQUIRE(mem.readByte(0x1005) == 0x56);
    REQUIRE(mem.readByte(0x1006) == 0x34);
    REQUIRE(mem.readByte(0x1007) == 0x12);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == false);
    REQUIRE(result.wroteMemory == true);
    REQUIRE(result.memoryAddress == 0x1004);
    REQUIRE(result.memorySize == 4);
    REQUIRE(result.wroteCPSR == false);
}

TEST_CASE("Load/Store - LDR word shifted", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up memory with test data (little-endian)
    mem.writeByte(0x2000, 0xAA);
    mem.writeByte(0x2001, 0xBB);
    mem.writeByte(0x2002, 0xCC);
    mem.writeByte(0x2003, 0xDD);
    mem.writeByte(0x2004, 0xEE);

    cpu.setRegister(1, 0x2000); // Base address

    // LDR R0, [R1, R2, LSL #2] - Load from address R1 + (R2 << 2)
    cpu.setRegister(2, 1); // R2 = 1, so offset = 4
    AArch32Instruction instr;
    instr.raw = 0xE7910002; // LDR R0, [R1, R2, LSL #2]

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xEEDDCCBB);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
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
    ExecutionResult result = cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xDDCCBBAA);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
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
    ExecutionResult result = cpu.executeInstruction(instr);
    REQUIRE(mem.readByte(0x3000) == 0x78);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == false);
    REQUIRE(result.wroteMemory == true);
    REQUIRE(result.memoryAddress == 0x3000);
    REQUIRE(result.memorySize == 1);
    REQUIRE(result.wroteCPSR == false);
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
    ExecutionResult result = cpu.executeInstruction(instr);
    REQUIRE(cpu.getRegister(0) == 0xAB);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
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
    ExecutionResult result = cpu.executeInstruction(instr);

    // Check the data was written to R1+8
    REQUIRE(mem.readByte(0x5008) == 0x44);

    // Check that R1 was updated (write-back)
    REQUIRE(cpu.getRegister(1) == 0x5008);
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(1) == 1);
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
    ExecutionResult result = cpu.executeInstruction(instr);

    // Data should be written to original R1 address
    REQUIRE(mem.readByte(0x6000) == 0xDD);

    // R1 should be updated after the store
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(1) == 1);
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
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.didWritePC() == true);
    // PC + 8 + offset = 0x1000 + 8 + 16 = 0x1018
    REQUIRE(cpu.getNextPC() == 0x1018);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
    // B writes to PC (R15)
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(15) == 1);
    REQUIRE(result.registersWritten.size() == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
}

TEST_CASE("Branch - BL (branch with link)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x2000);

    // BL #32 (branch with link)
    AArch32Instruction instr;
    instr.raw = 0xEB000008; // BL #32

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    // R14 (LR) should contain return address (PC + 4)
    REQUIRE(cpu.getRegister(14) == 0x2004);

    // PC should be updated
    REQUIRE(cpu.didWritePC() == true);
    REQUIRE(cpu.getNextPC() == 0x2028); // 0x2000 + 8 + 32
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
    // BL writes to both R14 (link register) and R15 (PC)
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(14) == 1);
    REQUIRE(result.registersWritten.count(15) == 1);
    REQUIRE(result.registersWritten.size() == 2);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
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
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.didWritePC() == true);
    REQUIRE(cpu.getNextPC() == 0x5000);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC writes should be tracked via getNextPC(), not registers[15]
    // BX writes to PC (R15)
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(15) == 1);
    REQUIRE(result.registersWritten.size() == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
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
    ExecutionResult result = cpu.executeInstruction(instr);

    // R0 should be unchanged because condition is false
    REQUIRE(cpu.getRegister(0) == 999);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC should not change for non-branch instructions
    // Instruction didn't execute, so nothing should be written
    REQUIRE(result.wroteRegister == false);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
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

TEST_CASE("Data Processing - MOV to PC tracks register write", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0x2000);

    // MOV PC, R1 (write to PC/R15)
    // Encoding: 1110 00 0 1101 0 0000 1111 00000000 0001
    AArch32Instruction instr;
    instr.raw = 0xE1A0F001; // MOV PC, R1

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    // Verify PC write is tracked
    REQUIRE(cpu.didWritePC() == true);
    REQUIRE(cpu.getNextPC() == 0x2000);
    REQUIRE(cpu.getRegister(15) == initial_pc); // PC tracked separately

    // Verify ExecutionResult tracks the write to R15
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(15) == 1);
    REQUIRE(result.registersWritten.size() == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false);
}

TEST_CASE("Multiply - MUL basic", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 6);   // Rm
    cpu.setRegister(2, 7);   // Rs

    // MUL R0, R1, R2 (R0 = R1 * R2 = 6 * 7 = 42)
    // Encoding: cccc 0000 000S dddd 0000 ssss 1001 mmmm
    // 1110 0000 0000 0000 0000 0010 1001 0001
    AArch32Instruction instr;
    instr.raw = 0xE0000291; // MUL R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 42);
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false); // S bit not set
}

TEST_CASE("Multiply - MUL with flags", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0);
    cpu.setRegister(2, 100);

    // MULS R0, R1, R2 (R0 = 0 * 100 = 0, should set Z flag)
    // S bit = 1
    AArch32Instruction instr;
    instr.raw = 0xE0100291; // MULS R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 0);
    REQUIRE((cpu.getCPSR() & (1 << 30)) != 0); // Z flag set
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteCPSR == true); // S bit is set
}

TEST_CASE("Multiply - MUL large numbers", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 1000);
    cpu.setRegister(2, 2000);

    // MUL R3, R1, R2 (R3 = 1000 * 2000 = 2000000)
    AArch32Instruction instr;
    instr.raw = 0xE0030291; // MUL R3, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(3) == 2000000);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Multiply - MLA basic", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 5);   // Rm
    cpu.setRegister(2, 4);   // Rs
    cpu.setRegister(3, 10);  // Rn (accumulate)

    // MLA R0, R1, R2, R3 (R0 = R1 * R2 + R3 = 5 * 4 + 10 = 30)
    // A bit = 1
    AArch32Instruction instr;
    instr.raw = 0xE0203291; // MLA R0, R1, R2, R3

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 30);
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false); // S bit not set
}

TEST_CASE("Multiply - MLA with zero accumulate", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 12);
    cpu.setRegister(2, 3);
    cpu.setRegister(3, 0);

    // MLA R4, R1, R2, R3 (R4 = 12 * 3 + 0 = 36)
    AArch32Instruction instr;
    instr.raw = 0xE0243291; // MLA R4, R1, R2, R3

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(4) == 36);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Multiply - MLAS with flags", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 0);
    cpu.setRegister(2, 100);
    cpu.setRegister(3, 0);

    // MLAS R0, R1, R2, R3 (R0 = 0 * 100 + 0 = 0, should set Z flag)
    // S bit = 1, A bit = 1
    AArch32Instruction instr;
    instr.raw = 0xE0303291; // MLAS R0, R1, R2, R3

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 0);
    REQUIRE((cpu.getCPSR() & (1 << 30)) != 0); // Z flag set
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteCPSR == true); // S bit is set
}

TEST_CASE("Multiply - MLA negative result sets N flag", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Test that results in negative number
    cpu.setRegister(1, 0xFFFFFFFF); // -1 in two's complement
    cpu.setRegister(2, 5);
    cpu.setRegister(3, 0);

    // MLAS R0, R1, R2, R3 (R0 = -1 * 5 + 0 = -5, should set N flag)
    AArch32Instruction instr;
    instr.raw = 0xE0303291; // MLAS R0, R1, R2, R3

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == (uint32_t)-5);
    REQUIRE((cpu.getCPSR() & (1 << 31)) != 0); // N flag set (negative result)
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Divide - UDIV basic", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 42);  // Dividend
    cpu.setRegister(2, 7);   // Divisor

    // UDIV R0, R1, R2 (R0 = R1 / R2 = 42 / 7 = 6)
    // Encoding: cccc 0111 0011 dddd 1111 mmmm 0001 nnnn
    // 1110 0111 0011 0000 1111 0010 0001 0001
    AArch32Instruction instr;
    instr.raw = 0xE730F211; // UDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 6);
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false); // Division does not update flags
}

TEST_CASE("Divide - UDIV with remainder", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 100);  // Dividend
    cpu.setRegister(2, 7);    // Divisor

    // UDIV R0, R1, R2 (R0 = 100 / 7 = 14, remainder discarded)
    AArch32Instruction instr;
    instr.raw = 0xE730F211; // UDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 14);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Divide - UDIV by zero", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 100);  // Dividend
    cpu.setRegister(2, 0);    // Divisor (zero)

    // UDIV R0, R1, R2 (R0 = 100 / 0 = 0, result is UNPREDICTABLE)
    AArch32Instruction instr;
    instr.raw = 0xE730F211; // UDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // Division by zero returns 0 in our implementation
    REQUIRE(cpu.getRegister(0) == 0);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Divide - SDIV basic positive", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 42);   // Dividend
    cpu.setRegister(2, 7);    // Divisor

    // SDIV R0, R1, R2 (R0 = R1 / R2 = 42 / 7 = 6)
    // Encoding: cccc 0111 0001 dddd 1111 mmmm 0001 nnnn
    // 1110 0111 0001 0000 1111 0010 0001 0001
    AArch32Instruction instr;
    instr.raw = 0xE710F211; // SDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 6);
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.wroteMemory == false);
    REQUIRE(result.wroteCPSR == false); // Division does not update flags
}

TEST_CASE("Divide - SDIV negative dividend", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, (uint32_t)-42); // -42 as two's complement
    cpu.setRegister(2, 7);

    // SDIV R0, R1, R2 (R0 = -42 / 7 = -6)
    AArch32Instruction instr;
    instr.raw = 0xE710F211; // SDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == (uint32_t)-6);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Divide - SDIV negative divisor", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 42);
    cpu.setRegister(2, (uint32_t)-7); // -7 as two's complement

    // SDIV R0, R1, R2 (R0 = 42 / -7 = -6)
    AArch32Instruction instr;
    instr.raw = 0xE710F211; // SDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == (uint32_t)-6);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Divide - SDIV both negative", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, (uint32_t)-42); // -42 as two's complement
    cpu.setRegister(2, (uint32_t)-7);  // -7 as two's complement

    // SDIV R0, R1, R2 (R0 = -42 / -7 = 6)
    AArch32Instruction instr;
    instr.raw = 0xE710F211; // SDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 6);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Divide - SDIV by zero", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(1, 100);  // Dividend
    cpu.setRegister(2, 0);    // Divisor (zero)

    // SDIV R0, R1, R2 (R0 = 100 / 0 = 0, result is UNPREDICTABLE)
    AArch32Instruction instr;
    instr.raw = 0xE710F211; // SDIV R0, R1, R2

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // Division by zero returns 0 in our implementation
    REQUIRE(cpu.getRegister(0) == 0);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Block Data Transfer - STMIA basic", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up registers
    cpu.setRegister(0, 0x11111111);
    cpu.setRegister(1, 0x22222222);
    cpu.setRegister(2, 0x33333333);
    cpu.setRegister(13, 0x2000); // Base address (SP)

    // STMIA R13, {R0, R1, R2}
    // Encoding: cccc 100P USWL nnnn rrrrrrrrrrrrrrrr
    // 1110 1000 1000 1101 0000000000000111
    AArch32Instruction instr;
    instr.raw = 0xE88D0007; // STMIA R13, {R0-R2}

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    // Check that values were stored
    uint32_t val0 = mem.readByte(0x2000) | (mem.readByte(0x2001) << 8) |
                    (mem.readByte(0x2002) << 16) | (mem.readByte(0x2003) << 24);
    uint32_t val1 = mem.readByte(0x2004) | (mem.readByte(0x2005) << 8) |
                    (mem.readByte(0x2006) << 16) | (mem.readByte(0x2007) << 24);
    uint32_t val2 = mem.readByte(0x2008) | (mem.readByte(0x2009) << 8) |
                    (mem.readByte(0x200A) << 16) | (mem.readByte(0x200B) << 24);

    REQUIRE(val0 == 0x11111111);
    REQUIRE(val1 == 0x22222222);
    REQUIRE(val2 == 0x33333333);
    REQUIRE(cpu.getRegister(13) == 0x2000); // No write-back
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteMemory == true);
}

TEST_CASE("Block Data Transfer - LDMIA basic", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up memory
    mem.writeByte(0x2000, 0x11);
    mem.writeByte(0x2001, 0x11);
    mem.writeByte(0x2002, 0x11);
    mem.writeByte(0x2003, 0x11);
    mem.writeByte(0x2004, 0x22);
    mem.writeByte(0x2005, 0x22);
    mem.writeByte(0x2006, 0x22);
    mem.writeByte(0x2007, 0x22);

    cpu.setRegister(13, 0x2000); // Base address

    // LDMIA R13, {R0, R1}
    // 1110 1000 1001 1101 0000000000000011
    AArch32Instruction instr;
    instr.raw = 0xE89D0003; // LDMIA R13, {R0-R1}

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 0x11111111);
    REQUIRE(cpu.getRegister(1) == 0x22222222);
    REQUIRE(cpu.getRegister(13) == 0x2000); // No write-back
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.wroteRegister == true);
    REQUIRE(result.registersWritten.count(0) == 1);
    REQUIRE(result.registersWritten.count(1) == 1);
}

TEST_CASE("Block Data Transfer - STMDB with write-back (PUSH)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(0, 0xAAAAAAAA);
    cpu.setRegister(1, 0xBBBBBBBB);
    cpu.setRegister(13, 0x2010); // SP

    // STMDB R13!, {R0, R1} (equivalent to PUSH {R0, R1})
    // 1110 1001 0010 1101 0000000000000011
    AArch32Instruction instr;
    instr.raw = 0xE92D0003; // STMDB R13!, {R0-R1}

    uint32_t initial_pc = cpu.getRegister(15);
    cpu.executeInstruction(instr);

    // Check that SP was decremented
    REQUIRE(cpu.getRegister(13) == 0x2008); // SP -= 8 (2 registers * 4 bytes)

    // Check stored values
    uint32_t val1 = mem.readByte(0x200C) | (mem.readByte(0x200D) << 8) |
                    (mem.readByte(0x200E) << 16) | (mem.readByte(0x200F) << 24);
    uint32_t val0 = mem.readByte(0x2008) | (mem.readByte(0x2009) << 8) |
                    (mem.readByte(0x200A) << 16) | (mem.readByte(0x200B) << 24);

    REQUIRE(val0 == 0xAAAAAAAA);
    REQUIRE(val1 == 0xBBBBBBBB);
    REQUIRE(cpu.getRegister(15) == initial_pc);
}

TEST_CASE("Block Data Transfer - LDMIA with write-back (POP)", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up stack with values
    mem.writeByte(0x2000, 0xAA);
    mem.writeByte(0x2001, 0xAA);
    mem.writeByte(0x2002, 0xAA);
    mem.writeByte(0x2003, 0xAA);
    mem.writeByte(0x2004, 0xBB);
    mem.writeByte(0x2005, 0xBB);
    mem.writeByte(0x2006, 0xBB);
    mem.writeByte(0x2007, 0xBB);

    cpu.setRegister(13, 0x2000); // SP

    // LDMIA R13!, {R0, R1} (equivalent to POP {R0, R1})
    // 1110 1000 1011 1101 0000000000000011
    AArch32Instruction instr;
    instr.raw = 0xE8BD0003; // LDMIA R13!, {R0-R1}

    uint32_t initial_pc = cpu.getRegister(15);
    ExecutionResult result = cpu.executeInstruction(instr);

    REQUIRE(cpu.getRegister(0) == 0xAAAAAAAA);
    REQUIRE(cpu.getRegister(1) == 0xBBBBBBBB);
    REQUIRE(cpu.getRegister(13) == 0x2008); // SP += 8
    REQUIRE(cpu.getRegister(15) == initial_pc);
    REQUIRE(result.registersWritten.count(13) == 1); // SP was written
}

TEST_CASE("Block Data Transfer - STMIB increment before", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    cpu.setRegister(0, 0x12345678);
    cpu.setRegister(5, 0x2000); // Base register

    // STMIB R5, {R0}
    // 1110 1001 1000 0101 0000000000000001
    AArch32Instruction instr;
    instr.raw = 0xE9850001; // STMIB R5, {R0}

    cpu.executeInstruction(instr);

    // Check that value was stored at base + 4 (increment before)
    uint32_t val = mem.readByte(0x2004) | (mem.readByte(0x2005) << 8) |
                   (mem.readByte(0x2006) << 16) | (mem.readByte(0x2007) << 24);

    REQUIRE(val == 0x12345678);
    REQUIRE(cpu.getRegister(5) == 0x2000); // Base unchanged
}

TEST_CASE("Block Data Transfer - Multiple registers", "[cpu_ee]") {
    Memory mem;
    TestableExecutionEngine cpu(&mem, 0x1000);

    // Set up multiple registers
    for (int i = 0; i < 8; i++) {
        cpu.setRegister(i, 0x10000000 + i);
    }
    cpu.setRegister(13, 0x3000);

    // STMIA R13!, {R0-R7}
    // 1110 1000 1010 1101 0000000011111111
    AArch32Instruction instr;
    instr.raw = 0xE8AD00FF; // STMIA R13!, {R0-R7}

    cpu.executeInstruction(instr);

    // Verify all 8 registers were stored
    for (int i = 0; i < 8; i++) {
        uint32_t addr = 0x3000 + (i * 4);
        uint32_t val = mem.readByte(addr) | (mem.readByte(addr + 1) << 8) |
                       (mem.readByte(addr + 2) << 16) | (mem.readByte(addr + 3) << 24);
        REQUIRE(val == (0x10000000 + i));
    }

    REQUIRE(cpu.getRegister(13) == 0x3020); // SP += 32 (8 registers)
}
