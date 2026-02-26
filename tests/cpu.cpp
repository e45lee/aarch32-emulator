/**
    Tests for the CPU class of the AArch32 emulator.
    This includes tests for:
        - Single instruction stepping (step())
        - Multi-instruction execution (stepUntilPC())
        - CPU state inspection (getRegister(), getCPSR())
        - Halt detection (isHalted())
        - PC management and instruction fetching
 */

#include "CPU.hpp"
#include "Memory.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("CPU construction with initial PC", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x1000);

  // Verify initial PC is set correctly
  REQUIRE(cpu.getRegister(15) == 0x1000);
  REQUIRE(cpu.isHalted() == false);
}

TEST_CASE("CPU step executes single instruction", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x1000);

  // Write MOV R0, #42 instruction to memory
  // Encoding: 0xE3A0002A (MOV R0, #42)
  mem.writeByte(0x1000, 0x2A);
  mem.writeByte(0x1001, 0x00);
  mem.writeByte(0x1002, 0xA0);
  mem.writeByte(0x1003, 0xE3);

  ExecutionResult result = cpu.step();

  // Verify instruction was executed
  REQUIRE(cpu.getRegister(0) == 42);
  // Verify PC was updated to next instruction
  REQUIRE(cpu.getRegister(15) == 0x1004);
  // Verify execution result
  REQUIRE(result.wroteRegister == true);
  REQUIRE(result.registersWritten.count(0) == 1);
  REQUIRE(result.wroteMemory == false);
}

TEST_CASE("CPU step multiple instructions", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x2000);

  // Write sequence: MOV R0, #10, then ADD R1, R0, #10 (R1 = 20)
  // MOV R0, #10: 0xE3A0000A
  mem.writeByte(0x2000, 0x0A);
  mem.writeByte(0x2001, 0x00);
  mem.writeByte(0x2002, 0xA0);
  mem.writeByte(0x2003, 0xE3);

  // ADD R1, R0, #10: 0xE280100A
  mem.writeByte(0x2004, 0x0A);
  mem.writeByte(0x2005, 0x10);
  mem.writeByte(0x2006, 0x80);
  mem.writeByte(0x2007, 0xE2);

  cpu.step();
  REQUIRE(cpu.getRegister(0) == 10);
  REQUIRE(cpu.getRegister(15) == 0x2004);

  cpu.step();
  REQUIRE(cpu.getRegister(1) == 20);
  REQUIRE(cpu.getRegister(15) == 0x2008);
}

TEST_CASE("CPU step with branch instruction", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x3000);

  // Write B #8 (branch forward 8 bytes)
  // Encoding: 0xEA000002 (offset=2, shifted left by 2 = 8)
  mem.writeByte(0x3000, 0x02);
  mem.writeByte(0x3001, 0x00);
  mem.writeByte(0x3002, 0x00);
  mem.writeByte(0x3003, 0xEA);

  ExecutionResult result = cpu.step();

  // PC should be at 0x3000 + 8 (instruction bytes) + 8 (branch offset) = 0x3010
  REQUIRE(cpu.getRegister(15) == 0x3010);
  REQUIRE(result.wroteRegister == true);
  REQUIRE(result.registersWritten.count(15) == 1);
}

TEST_CASE("CPU step with load instruction", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x4000);

  // Set up test data in memory
  mem.writeByte(0x5000, 0x12);
  mem.writeByte(0x5001, 0x34);
  mem.writeByte(0x5002, 0x56);
  mem.writeByte(0x5003, 0x78);

  // Write MOV R1, #0x5000 to set up base address
  // We'll use immediate encoding: MOV R1, #20480 (0x5000)
  // For simplicity, let's manually set R1 first by using multiple instructions

  // MOV R1, #0x5000 using movw-like encoding (simplified)
  // Actually, let's write LDR R0, [R1, #0] where R1 contains 0x5000
  // First instruction: MOV R1, #0x50 with LSL #8 -> 0x5000
  // This is complex, so let's use a different approach

  // Set R1 to 0x5000 using MOV with rotated immediate
  // MOV R1, #0x50: 0xE3A01050
  mem.writeByte(0x4000, 0x50);
  mem.writeByte(0x4001, 0x10);
  mem.writeByte(0x4002, 0xA0);
  mem.writeByte(0x4003, 0xE3);

  cpu.step();
  REQUIRE(cpu.getRegister(1) == 0x50);

  // Now shift it left to get 0x5000
  // MOV R1, R1, LSL #8: 0xE1A01401
  mem.writeByte(0x4004, 0x01);
  mem.writeByte(0x4005, 0x14);
  mem.writeByte(0x4006, 0xA0);
  mem.writeByte(0x4007, 0xE1);

  cpu.step();
  REQUIRE(cpu.getRegister(1) == 0x5000);

  // LDR R0, [R1]: 0xE5910000
  mem.writeByte(0x4008, 0x00);
  mem.writeByte(0x4009, 0x00);
  mem.writeByte(0x400A, 0x91);
  mem.writeByte(0x400B, 0xE5);

  ExecutionResult result = cpu.step();
  REQUIRE(cpu.getRegister(0) == 0x78563412);
  REQUIRE(result.wroteRegister == true);
  REQUIRE(result.wroteMemory == false);
}

TEST_CASE("CPU step with store instruction", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x6000);

  // MOV R0, #0xAB: 0xE3A000AB
  mem.writeByte(0x6000, 0xAB);
  mem.writeByte(0x6001, 0x00);
  mem.writeByte(0x6002, 0xA0);
  mem.writeByte(0x6003, 0xE3);

  // MOV R1, #0x70: 0xE3A01070
  mem.writeByte(0x6004, 0x70);
  mem.writeByte(0x6005, 0x10);
  mem.writeByte(0x6006, 0xA0);
  mem.writeByte(0x6007, 0xE3);

  // STRB R0, [R1]: 0xE5C10000
  mem.writeByte(0x6008, 0x00);
  mem.writeByte(0x6009, 0x00);
  mem.writeByte(0x600A, 0xC1);
  mem.writeByte(0x600B, 0xE5);

  cpu.step();                          // MOV R0, #0xAB
  cpu.step();                          // MOV R1, #0x70
  ExecutionResult result = cpu.step(); // STRB R0, [R1]

  REQUIRE(mem.readByte(0x70) == 0xAB);
  REQUIRE(result.wroteMemory == true);
  REQUIRE(result.memoryAddress == 0x70);
  REQUIRE(result.memorySize == 1);
}

TEST_CASE("CPU stepUntilPC with simple loop", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x7000);

  // Write a sequence of instructions that increment a counter
  // MOV R0, #0: 0xE3A00000
  mem.writeByte(0x7000, 0x00);
  mem.writeByte(0x7001, 0x00);
  mem.writeByte(0x7002, 0xA0);
  mem.writeByte(0x7003, 0xE3);

  // ADD R0, R0, #1: 0xE2800001
  mem.writeByte(0x7004, 0x01);
  mem.writeByte(0x7005, 0x00);
  mem.writeByte(0x7006, 0x80);
  mem.writeByte(0x7007, 0xE2);

  // ADD R0, R0, #1: 0xE2800001
  mem.writeByte(0x7008, 0x01);
  mem.writeByte(0x7009, 0x00);
  mem.writeByte(0x700A, 0x80);
  mem.writeByte(0x700B, 0xE2);

  // ADD R0, R0, #1: 0xE2800001
  mem.writeByte(0x700C, 0x01);
  mem.writeByte(0x700D, 0x00);
  mem.writeByte(0x700E, 0x80);
  mem.writeByte(0x700F, 0xE2);

  ExecutionResult result = cpu.stepUntilPC(0x7010);

  REQUIRE(cpu.getRegister(15) == 0x7010);
  REQUIRE(cpu.getRegister(0) == 3); // Three ADD instructions executed
}

TEST_CASE("CPU stepUntilPC stops at target", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x8000);

  // Create a simple sequence
  for (int i = 0; i < 10; i++) {
    // NOP (MOV R0, R0): 0xE1A00000
    mem.writeByte(0x8000 + i * 4 + 0, 0x00);
    mem.writeByte(0x8000 + i * 4 + 1, 0x00);
    mem.writeByte(0x8000 + i * 4 + 2, 0xA0);
    mem.writeByte(0x8000 + i * 4 + 3, 0xE1);
  }

  cpu.stepUntilPC(0x8010); // Skip to 4th instruction
  REQUIRE(cpu.getRegister(15) == 0x8010);
}

TEST_CASE("CPU getCPSR returns status register", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0x9000);

  // Write ADDS R0, R0, #0 with S bit (should set Z flag)
  // Set R0 to 0 first
  // MOV R0, #0: 0xE3A00000
  mem.writeByte(0x9000, 0x00);
  mem.writeByte(0x9001, 0x00);
  mem.writeByte(0x9002, 0xA0);
  mem.writeByte(0x9003, 0xE3);

  // ADDS R0, R0, #0 (0+0=0, Z flag set): 0xE2900000
  mem.writeByte(0x9004, 0x00);
  mem.writeByte(0x9005, 0x00);
  mem.writeByte(0x9006, 0x90);
  mem.writeByte(0x9007, 0xE2);

  cpu.step(); // MOV R0, #0
  cpu.step(); // ADDS R0, R0, #0

  uint32_t cpsr = cpu.getCPSR();
  // Z flag (bit 30) should be set
  REQUIRE((cpsr & (1 << 30)) != 0);
}

TEST_CASE("CPU isHalted returns false when running", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0xA000);

  REQUIRE(cpu.isHalted() == false);

  // Write and execute a NOP
  mem.writeByte(0xA000, 0x00);
  mem.writeByte(0xA001, 0x00);
  mem.writeByte(0xA002, 0xA0);
  mem.writeByte(0xA003, 0xE1);

  cpu.step();
  REQUIRE(cpu.isHalted() == false);
}

TEST_CASE("CPU isHalted returns true when PC is at halted_pc", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0xB000);

  // Write a branch to the halted PC address
  // Construct 0x3FFF0000 = (0x3F << 8) | 0xFF, then << 16

  // MOV R0, #0x3F: 0xE3A0003F
  mem.writeByte(0xB000, 0x3F);
  mem.writeByte(0xB001, 0x00);
  mem.writeByte(0xB002, 0xA0);
  mem.writeByte(0xB003, 0xE3);

  // MOV R0, R0, LSL #8: 0xE1A00400
  mem.writeByte(0xB004, 0x00);
  mem.writeByte(0xB005, 0x04);
  mem.writeByte(0xB006, 0xA0);
  mem.writeByte(0xB007, 0xE1);

  // ADD R0, R0, #0xFF: 0xE28000FF
  mem.writeByte(0xB008, 0xFF);
  mem.writeByte(0xB009, 0x00);
  mem.writeByte(0xB00A, 0x80);
  mem.writeByte(0xB00B, 0xE2);

  // MOV R0, R0, LSL #16: 0xE1A00800
  mem.writeByte(0xB00C, 0x00);
  mem.writeByte(0xB00D, 0x08);
  mem.writeByte(0xB00E, 0xA0);
  mem.writeByte(0xB00F, 0xE1);

  // Now R0 should contain 0x3FFF0000
  // MOV PC, R0: 0xE1A0F000
  mem.writeByte(0xB010, 0x00);
  mem.writeByte(0xB011, 0xF0);
  mem.writeByte(0xB012, 0xA0);
  mem.writeByte(0xB013, 0xE1);

  cpu.step(); // MOV R0, #0x3F
  cpu.step(); // LSL #8 -> R0 = 0x3F00
  cpu.step(); // ADD #0xFF -> R0 = 0x3FFF
  cpu.step(); // LSL #16 -> R0 = 0x3FFF0000
  cpu.step(); // MOV PC, R0

  REQUIRE(cpu.getRegister(15) == halted_pc);
  REQUIRE(cpu.isHalted() == true);
}

TEST_CASE("CPU step returns empty result when halted", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0xC000);

  // Set up instructions to reach halted state
  // Construct 0x3FFF0000 = (0x3F << 8) | 0xFF, then << 16

  // MOV R0, #0x3F: 0xE3A0003F
  mem.writeByte(0xC000, 0x3F);
  mem.writeByte(0xC001, 0x00);
  mem.writeByte(0xC002, 0xA0);
  mem.writeByte(0xC003, 0xE3);

  // MOV R0, R0, LSL #8: 0xE1A00400
  mem.writeByte(0xC004, 0x00);
  mem.writeByte(0xC005, 0x04);
  mem.writeByte(0xC006, 0xA0);
  mem.writeByte(0xC007, 0xE1);

  // ADD R0, R0, #0xFF: 0xE28000FF
  mem.writeByte(0xC008, 0xFF);
  mem.writeByte(0xC009, 0x00);
  mem.writeByte(0xC00A, 0x80);
  mem.writeByte(0xC00B, 0xE2);

  // MOV R0, R0, LSL #16: 0xE1A00800
  mem.writeByte(0xC00C, 0x00);
  mem.writeByte(0xC00D, 0x08);
  mem.writeByte(0xC00E, 0xA0);
  mem.writeByte(0xC00F, 0xE1);

  // MOV PC, R0: 0xE1A0F000
  mem.writeByte(0xC010, 0x00);
  mem.writeByte(0xC011, 0xF0);
  mem.writeByte(0xC012, 0xA0);
  mem.writeByte(0xC013, 0xE1);

  cpu.step(); // MOV R0, #0x3F
  cpu.step(); // LSL #8 -> R0 = 0x3F00
  cpu.step(); // ADD #0xFF -> R0 = 0x3FFF
  cpu.step(); // LSL #16 -> R0 = 0x3FFF0000
  cpu.step(); // MOV PC, R0

  REQUIRE(cpu.isHalted() == true);

  // Try stepping again - should return empty result and not execute
  // Note: halted_pc is in an unmapped memory region, so CPU should detect halt
  // before trying to fetch
  ExecutionResult result = cpu.step();

  REQUIRE(result.wroteRegister == false);
  REQUIRE(result.wroteMemory == false);
  REQUIRE(result.wroteCPSR == false);
  REQUIRE(cpu.getRegister(1) == 0); // R1 should remain at initial value
}

TEST_CASE("CPU stepUntilPC stops when halted", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0xD000);

  // Create sequence that branches to halt address
  // Construct 0x3FFF0000 = (0x3F << 8) | 0xFF, then << 16

  // MOV R0, #0x3F
  mem.writeByte(0xD000, 0x3F);
  mem.writeByte(0xD001, 0x00);
  mem.writeByte(0xD002, 0xA0);
  mem.writeByte(0xD003, 0xE3);

  // MOV R0, R0, LSL #8
  mem.writeByte(0xD004, 0x00);
  mem.writeByte(0xD005, 0x04);
  mem.writeByte(0xD006, 0xA0);
  mem.writeByte(0xD007, 0xE1);

  // ADD R0, R0, #0xFF
  mem.writeByte(0xD008, 0xFF);
  mem.writeByte(0xD009, 0x00);
  mem.writeByte(0xD00A, 0x80);
  mem.writeByte(0xD00B, 0xE2);

  // MOV R0, R0, LSL #16
  mem.writeByte(0xD00C, 0x00);
  mem.writeByte(0xD00D, 0x08);
  mem.writeByte(0xD00E, 0xA0);
  mem.writeByte(0xD00F, 0xE1);

  // MOV PC, R0
  mem.writeByte(0xD010, 0x00);
  mem.writeByte(0xD011, 0xF0);
  mem.writeByte(0xD012, 0xA0);
  mem.writeByte(0xD013, 0xE1);

  // Try to step until a PC that will never be reached
  ExecutionResult result = cpu.stepUntilPC(0xFFFFFFFF);

  // Should stop because it hit the halt address
  REQUIRE(cpu.getRegister(15) == halted_pc);
  REQUIRE(cpu.isHalted() == true);
}

TEST_CASE("CPU correctly fetches 4-byte instruction", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0xE000);

  // Write a specific instruction with known byte pattern
  // ADD R5, R7, R9: 0xE0875009
  mem.writeByte(0xE000, 0x09);
  mem.writeByte(0xE001, 0x50);
  mem.writeByte(0xE002, 0x87);
  mem.writeByte(0xE003, 0xE0);

  // Set up registers
  // We need to set R7 and R9 first
  // MOV R7, #10: 0xE3A0700A
  mem.writeByte(0xE000, 0x0A);
  mem.writeByte(0xE001, 0x70);
  mem.writeByte(0xE002, 0xA0);
  mem.writeByte(0xE003, 0xE3);

  // MOV R9, #20: 0xE3A09014
  mem.writeByte(0xE004, 0x14);
  mem.writeByte(0xE005, 0x90);
  mem.writeByte(0xE006, 0xA0);
  mem.writeByte(0xE007, 0xE3);

  // ADD R5, R7, R9: 0xE0875009
  mem.writeByte(0xE008, 0x09);
  mem.writeByte(0xE009, 0x50);
  mem.writeByte(0xE00A, 0x87);
  mem.writeByte(0xE00B, 0xE0);

  cpu.step(); // MOV R7, #10
  cpu.step(); // MOV R9, #20
  cpu.step(); // ADD R5, R7, R9

  REQUIRE(cpu.getRegister(5) == 30);
  REQUIRE(cpu.getRegister(15) == 0xE00C);
}

TEST_CASE("CPU getRegister returns correct values", "[cpu]") {
  Memory mem;
  CPU cpu(&mem, 0xF000);

  // Verify all registers start at 0 (except PC)
  for (int i = 0; i < 14; i++) {
    REQUIRE(cpu.getRegister(i) == 0);
  }
  REQUIRE(cpu.getRegister(14) ==
          halted_pc); // 14 starts at the halted PC value by default
  REQUIRE(cpu.getRegister(15) == 0xF000);

  // Write instructions to set multiple registers
  // MOV R3, #77: 0xE3A0304D
  mem.writeByte(0xF000, 0x4D);
  mem.writeByte(0xF001, 0x30);
  mem.writeByte(0xF002, 0xA0);
  mem.writeByte(0xF003, 0xE3);

  // MOV R8, #88: 0xE3A08058
  mem.writeByte(0xF004, 0x58);
  mem.writeByte(0xF005, 0x80);
  mem.writeByte(0xF006, 0xA0);
  mem.writeByte(0xF007, 0xE3);

  cpu.step();
  cpu.step();

  REQUIRE(cpu.getRegister(3) == 77);
  REQUIRE(cpu.getRegister(8) == 88);
}
