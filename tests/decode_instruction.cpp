#include "Instruction.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Data processing instructions", "[instruction][decode]") {
  SECTION("ADD instruction with immediate") {
    // ADD R1, R2, #5
    // Opcode: 0100 (ADD), Rd=1, Rn=2, imm=5, rotate=0, I=1, S=0, cond=1110 (AL)
    uint32_t instr = 0xE2821005; // ADD R1, R2, #5
    REQUIRE(decodeInstruction(instr) == "ADD R1, R2, #5");
  }

  SECTION("SUB instruction with immediate") {
    // SUB R3, R4, #10
    uint32_t instr = 0xE244300A; // SUB R3, R4, #10
    REQUIRE(decodeInstruction(instr) == "SUB R3, R4, #10");
  }

  SECTION("MOV instruction with immediate") {
    // MOV R0, #42
    // Opcode: 1101 (MOV), Rd=0, imm=42, I=1, S=0, cond=1110 (AL)
    uint32_t instr = 0xE3A0002A; // MOV R0, #42
    REQUIRE(decodeInstruction(instr) == "MOV R0, #42");
  }

  SECTION("MOV instruction with register") {
    // MOV R1, R2
    uint32_t instr = 0xE1A01002; // MOV R1, R2
    REQUIRE(decodeInstruction(instr) == "MOV R1, R2");
  }

  SECTION("ADD instruction with register operand") {
    // ADD R0, R1, R2
    uint32_t instr = 0xE0810002; // ADD R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "ADD R0, R1, R2");
  }

  SECTION("ADD instruction with shifted register") {
    // ADD R0, R1, R2, LSL #4
    uint32_t instr = 0xE0810202; // ADD R0, R1, R2, LSL #4
    REQUIRE(decodeInstruction(instr) == "ADD R0, R1, R2, LSL #4");
  }

  SECTION("SUB instruction with LSR shift") {
    // SUB R5, R6, R7, LSR #8
    uint32_t instr = 0xE0465427; // SUB R5, R6, R7, LSR #8
    REQUIRE(decodeInstruction(instr) == "SUB R5, R6, R7, LSR #8");
  }

  SECTION("AND instruction") {
    // AND R0, R1, R2
    uint32_t instr = 0xE0010002; // AND R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "AND R0, R1, R2");
  }

  SECTION("ORR instruction") {
    // ORR R3, R4, R5
    uint32_t instr = 0xE1843005; // ORR R3, R4, R5
    REQUIRE(decodeInstruction(instr) == "ORR R3, R4, R5");
  }

  SECTION("CMP instruction with immediate") {
    // CMP R0, #10
    uint32_t instr = 0xE350000A; // CMP R0, #10
    REQUIRE(decodeInstruction(instr) == "CMP R0, #10");
  }

  SECTION("ADDS instruction (with S flag)") {
    // ADDS R0, R1, R2
    uint32_t instr = 0xE0910002; // ADDS R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "ADDS R0, R1, R2");
  }

  SECTION("MVN instruction") {
    // MVN R0, #0
    uint32_t instr = 0xE3E00000; // MVN R0, #0
    REQUIRE(decodeInstruction(instr) == "MVN R0, #0");
  }
}

TEST_CASE("Data processing with condition codes", "[instruction][decode]") {
  SECTION("ADDEQ - ADD with equal condition") {
    // ADDEQ R0, R1, #5
    uint32_t instr = 0x02810005; // ADDEQ R0, R1, #5
    REQUIRE(decodeInstruction(instr) == "ADDEQ R0, R1, #5");
  }

  SECTION("MOVNE - MOV with not equal condition") {
    // MOVNE R2, #10
    uint32_t instr = 0x13A0200A; // MOVNE R2, #10
    REQUIRE(decodeInstruction(instr) == "MOVNE R2, #10");
  }

  SECTION("SUBGT - SUB with greater than condition") {
    // SUBGT R3, R4, R5
    uint32_t instr = 0xC0443005; // SUBGT R3, R4, R5
    REQUIRE(decodeInstruction(instr) == "SUBGT R3, R4, R5");
  }
}

TEST_CASE("Load/Store instructions", "[instruction][decode]") {
  SECTION("LDR with immediate offset") {
    // LDR R0, [R1, #4]
    uint32_t instr = 0xE5910004; // LDR R0, [R1, #4]
    REQUIRE(decodeInstruction(instr) == "LDR R0, [R1, #4]");
  }

  SECTION("LDR with register offset") {
    // LDR R2, [R3, R4]
    uint32_t instr = 0xE7932004; // LDR R2, [R3, R4]
    REQUIRE(decodeInstruction(instr) == "LDR R2, [R3, R4]");
  }

  SECTION("LDR with register offset and shift") {
    // LDR R2, [R3, R4, LSL #2]
    uint32_t instr = 0xE7932104; // LDR R2, [R3, R4, LSL #2]
    REQUIRE(decodeInstruction(instr) == "LDR R2, [R3, R4, LSL #2]");
  }

  SECTION("STR with immediate offset") {
    // STR R2, [R3, #8]
    uint32_t instr = 0xE5832008; // STR R2, [R3, #8]
    REQUIRE(decodeInstruction(instr) == "STR R2, [R3, #8]");
  }

  SECTION("LDR with negative offset") {
    // LDR R4, [R5, #-12]
    uint32_t instr = 0xE515400C; // LDR R4, [R5, #-12]
    REQUIRE(decodeInstruction(instr) == "LDR R4, [R5, #-12]");
  }

  SECTION("LDR with zero offset") {
    // LDR R0, [R1]
    uint32_t instr = 0xE5910000; // LDR R0, [R1]
    REQUIRE(decodeInstruction(instr) == "LDR R0, [R1]");
  }

  SECTION("LDR with write-back") {
    // LDR R0, [R1, #4]!
    uint32_t instr = 0xE5B10004; // LDR R0, [R1, #4]!
    REQUIRE(decodeInstruction(instr) == "LDR R0, [R1, #4]!");
  }

  SECTION("LDR with post-indexing") {
    // LDR R0, [R1], #4
    uint32_t instr = 0xE4910004; // LDR R0, [R1], #4
    REQUIRE(decodeInstruction(instr) == "LDR R0, [R1], #4");
  }

  SECTION("LDRB - byte load") {
    // LDRB R2, [R3, #1]
    uint32_t instr = 0xE5D32001; // LDRB R2, [R3, #1]
    REQUIRE(decodeInstruction(instr) == "LDRB R2, [R3, #1]");
  }

  SECTION("STRB - byte store") {
    // STRB R4, [R5, #2]
    uint32_t instr = 0xE5C54002; // STRB R4, [R5, #2]
    REQUIRE(decodeInstruction(instr) == "STRB R4, [R5, #2]");
  }

  SECTION("STR with post-indexing and negative offset") {
    // STR R6, [R7], #-16
    uint32_t instr = 0xE4076010; // STR R6, [R7], #-16
    REQUIRE(decodeInstruction(instr) == "STR R6, [R7], #-16");
  }
}

TEST_CASE("Load/Store with condition codes", "[instruction][decode]") {
  SECTION("LDREQ - LDR with equal condition") {
    // LDREQ R0, [R1, #4]
    uint32_t instr = 0x05910004; // LDREQ R0, [R1, #4]
    REQUIRE(decodeInstruction(instr) == "LDREQ R0, [R1, #4]");
  }

  SECTION("STRNE - STR with not equal condition") {
    // STRNE R2, [R3]
    uint32_t instr = 0x15832000; // STRNE R2, [R3]
    REQUIRE(decodeInstruction(instr) == "STRNE R2, [R3]");
  }
}

TEST_CASE("Branch instructions", "[instruction][decode]") {
  SECTION("B - unconditional branch forward") {
    // B +16 (offset = 2, which becomes 16 after <<2 and +8)
    uint32_t instr = 0xEA000002; // B 16
    REQUIRE(decodeInstruction(instr) == "B 16");
  }

  SECTION("B - unconditional branch forward") {
    // B +8 (offset = 0, which becomes 0 after <<2 and +8)
    uint32_t instr = 0xEA000000; // B 8
    REQUIRE(decodeInstruction(instr) == "B 8");
  }

  SECTION("BL - branch with link") {
    // BL +16
    uint32_t instr = 0xEB000002; // BL 16
    REQUIRE(decodeInstruction(instr) == "BL 16");
  }

  SECTION("B - branch backward") {
    // B -8 (offset = -4 in encoded form, -4 * 4 = -16, -16 + 8 = -8)
    uint32_t instr = 0xEAFFFFFC; // B -8
    REQUIRE(decodeInstruction(instr) == "B -8");
  }

  SECTION("BEQ - branch if equal") {
    // BEQ +12
    uint32_t instr = 0x0A000001; // BEQ 12
    REQUIRE(decodeInstruction(instr) == "BEQ 12");
  }

  SECTION("BNE - branch if not equal") {
    // BNE +20
    uint32_t instr = 0x1A000003; // BNE 20
    REQUIRE(decodeInstruction(instr) == "BNE 20");
  }

  SECTION("BGT - branch if greater than") {
    // BGT +8
    uint32_t instr = 0xCA000000; // BGT 8
    REQUIRE(decodeInstruction(instr) == "BGT 8");
  }

  SECTION("BLLT - branch with link if less than") {
    // BLLT +24
    uint32_t instr = 0xBB000004; // BLLT 24
    REQUIRE(decodeInstruction(instr) == "BLLT 24");
  }
}

TEST_CASE("Branch and exchange instructions", "[instruction][decode]") {
  SECTION("BX - branch and exchange") {
    // BX R14 (LR)
    uint32_t instr = 0xE12FFF1E; // BX R14
    REQUIRE(decodeInstruction(instr) == "BX R14");
  }

  SECTION("BX R0") {
    // BX R0
    uint32_t instr = 0xE12FFF10; // BX R0
    REQUIRE(decodeInstruction(instr) == "BX R0");
  }

  SECTION("BLX - branch with link and exchange") {
    // BLX R3
    uint32_t instr = 0xE12FFF33; // BLX R3
    REQUIRE(decodeInstruction(instr) == "BLX R3");
  }

  SECTION("BXNE - BX with not equal condition") {
    // BXNE R14
    uint32_t instr = 0x112FFF1E; // BXNE R14
    REQUIRE(decodeInstruction(instr) == "BXNE R14");
  }
}

TEST_CASE("Edge cases and special instructions", "[instruction][decode]") {
  SECTION("NOP equivalent - MOV R0, R0") {
    // MOV R0, R0 (often used as NOP)
    uint32_t instr = 0xE1A00000; // MOV R0, R0
    REQUIRE(decodeInstruction(instr) == "MOV R0, R0");
  }

  SECTION("Data processing with rotated immediate") {
    // MOV R0, #0xFF000000 (imm=0xFF, rotate=4 -> rotate by 8)
    uint32_t instr = 0xE3A004FF; // MOV R0, #0xFF000000
    REQUIRE(decodeInstruction(instr) == "MOV R0, #4278190080");
  }

  SECTION("EOR - Exclusive OR") {
    // EOR R1, R2, R3
    uint32_t instr = 0xE0221003; // EOR R1, R2, R3
    REQUIRE(decodeInstruction(instr) == "EOR R1, R2, R3");
  }

  SECTION("RSB - Reverse subtract") {
    // RSB R0, R1, #0
    uint32_t instr = 0xE2610000; // RSB R0, R1, #0
    REQUIRE(decodeInstruction(instr) == "RSB R0, R1, #0");
  }

  SECTION("BIC - Bit clear") {
    // BIC R4, R5, R6
    uint32_t instr = 0xE1C54006; // BIC R4, R5, R6
    REQUIRE(decodeInstruction(instr) == "BIC R4, R5, R6");
  }
}

TEST_CASE("Multiply instructions", "[instruction][decode]") {
  SECTION("MUL - basic multiply") {
    // MUL R0, R1, R2
    uint32_t instr = 0xE0000291; // MUL R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "MUL R0, R1, R2");
  }

  SECTION("MULS - multiply with flags") {
    // MULS R0, R1, R2
    uint32_t instr = 0xE0100291; // MULS R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "MULS R0, R1, R2");
  }

  SECTION("MUL with different registers") {
    // MUL R3, R4, R5
    uint32_t instr = 0xE0030594; // MUL R3, R4, R5
    REQUIRE(decodeInstruction(instr) == "MUL R3, R4, R5");
  }

  SECTION("MLA - multiply-accumulate") {
    // MLA R0, R1, R2, R3
    uint32_t instr = 0xE0203291; // MLA R0, R1, R2, R3
    REQUIRE(decodeInstruction(instr) == "MLA R0, R1, R2, R3");
  }

  SECTION("MLAS - multiply-accumulate with flags") {
    // MLAS R0, R1, R2, R3
    uint32_t instr = 0xE0303291; // MLAS R0, R1, R2, R3
    REQUIRE(decodeInstruction(instr) == "MLAS R0, R1, R2, R3");
  }

  SECTION("MLA with different registers") {
    // MLA R4, R1, R2, R3
    uint32_t instr = 0xE0243291; // MLA R4, R1, R2, R3
    REQUIRE(decodeInstruction(instr) == "MLA R4, R1, R2, R3");
  }

  SECTION("MULEQ - multiply with equal condition") {
    // MULEQ R0, R1, R2
    uint32_t instr = 0x00000291; // MULEQ R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "MULEQ R0, R1, R2");
  }

  SECTION("MLANE - multiply-accumulate with not equal condition") {
    // MLANE R0, R1, R2, R3
    uint32_t instr = 0x10203291; // MLANE R0, R1, R2, R3
    REQUIRE(decodeInstruction(instr) == "MLANE R0, R1, R2, R3");
  }
}

TEST_CASE("Divide instructions", "[instruction][decode]") {
  SECTION("UDIV - unsigned divide") {
    // UDIV R0, R1, R2
    uint32_t instr = 0xE730F211; // UDIV R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "UDIV R0, R1, R2");
  }

  SECTION("SDIV - signed divide") {
    // SDIV R0, R1, R2
    uint32_t instr = 0xE710F211; // SDIV R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "SDIV R0, R1, R2");
  }

  SECTION("UDIV with different registers") {
    // UDIV R4, R3, R6
    uint32_t instr = 0xE734F613; // UDIV R4, R3, R6
    REQUIRE(decodeInstruction(instr) == "UDIV R4, R3, R6");
  }

  SECTION("SDIV with different registers") {
    // SDIV R4, R3, R5
    uint32_t instr = 0xE714F513; // SDIV R4, R3, R5
    REQUIRE(decodeInstruction(instr) == "SDIV R4, R3, R5");
  }

  SECTION("UDIVEQ - unsigned divide with equal condition") {
    // UDIVEQ R0, R1, R2
    uint32_t instr = 0x0730F211; // UDIVEQ R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "UDIVEQ R0, R1, R2");
  }

  SECTION("SDIVNE - signed divide with not equal condition") {
    // SDIVNE R0, R1, R2
    uint32_t instr = 0x1710F211; // SDIVNE R0, R1, R2
    REQUIRE(decodeInstruction(instr) == "SDIVNE R0, R1, R2");
  }

  SECTION("UDIVGT - unsigned divide with greater than condition") {
    // UDIVGT R5, R6, R7
    uint32_t instr = 0xC735F716; // UDIVGT R5, R6, R7
    REQUIRE(decodeInstruction(instr) == "UDIVGT R5, R6, R7");
  }

  SECTION("SDIVLT - signed divide with less than condition") {
    // SDIVLT R2, R8, R9
    uint32_t instr = 0xB712F918; // SDIVLT R2, R8, R9
    REQUIRE(decodeInstruction(instr) == "SDIVLT R2, R8, R9");
  }
}

TEST_CASE("Block data transfer instructions (LDM/STM)",
          "[instruction][decode]") {
  SECTION("STMIA - store multiple increment after") {
    // STMIA R13, {R0, R1, R2}
    uint32_t instr = 0xE88D0007; // STMIA R13, {R0-R2}
    REQUIRE(decodeInstruction(instr) == "STMIA R13, {R0, R1, R2}");
  }

  SECTION("LDMIA - load multiple increment after") {
    // LDMIA R13, {R0, R1}
    uint32_t instr = 0xE89D0003; // LDMIA R13, {R0-R1}
    REQUIRE(decodeInstruction(instr) == "LDMIA R13, {R0, R1}");
  }

  SECTION("STMIA with write-back") {
    // STMIA R13!, {R0-R7}
    uint32_t instr = 0xE8AD00FF; // STMIA R13!, {R0-R7}
    REQUIRE(decodeInstruction(instr) ==
            "STMIA R13!, {R0, R1, R2, R3, R4, R5, R6, R7}");
  }

  SECTION("LDMIA with write-back (POP)") {
    // LDMIA R13!, {R0, R1}
    uint32_t instr = 0xE8BD0003; // LDMIA R13!, {R0, R1}
    REQUIRE(decodeInstruction(instr) == "LDMIA R13!, {R0, R1}");
  }

  SECTION("STMDB - store multiple decrement before (PUSH)") {
    // STMDB R13!, {R0, R1}
    uint32_t instr = 0xE92D0003; // STMDB R13!, {R0, R1}
    REQUIRE(decodeInstruction(instr) == "STMDB R13!, {R0, R1}");
  }

  SECTION("STMDB with multiple registers (PUSH)") {
    // STMDB R13!, {R4-R11, LR}
    uint32_t instr = 0xE92D4FF0; // STMDB R13!, {R4-R11, R14}
    REQUIRE(decodeInstruction(instr) ==
            "STMDB R13!, {R4, R5, R6, R7, R8, R9, R10, R11, R14}");
  }

  SECTION("LDMIA with PC (POP and return)") {
    // LDMIA R13!, {R4-R11, PC}
    uint32_t instr = 0xE8BD8FF0; // LDMIA R13!, {R4-R11, R15}
    REQUIRE(decodeInstruction(instr) ==
            "LDMIA R13!, {R4, R5, R6, R7, R8, R9, R10, R11, R15}");
  }

  SECTION("STMIB - store multiple increment before") {
    // STMIB R5, {R0}
    uint32_t instr = 0xE9850001; // STMIB R5, {R0}
    REQUIRE(decodeInstruction(instr) == "STMIB R5, {R0}");
  }

  SECTION("STMDA - store multiple decrement after") {
    // STMDA R10, {R1, R3, R5}
    uint32_t instr = 0xE80A002A; // STMDA R10, {R1, R3, R5}
    REQUIRE(decodeInstruction(instr) == "STMDA R10, {R1, R3, R5}");
  }

  SECTION("LDMIB - load multiple increment before") {
    // LDMIB R4, {R2, R6, R7}
    uint32_t instr = 0xE99400C4; // LDMIB R4, {R2, R6, R7} (no write-back)
    REQUIRE(decodeInstruction(instr) == "LDMIB R4, {R2, R6, R7}");
  }

  SECTION("LDMDB - load multiple decrement before") {
    // LDMDB R8, {R0, R1, R2}
    uint32_t instr = 0xE9180007; // LDMDB R8, {R0, R1, R2}
    REQUIRE(decodeInstruction(instr) == "LDMDB R8, {R0, R1, R2}");
  }

  SECTION("STMIAEQ - conditional store multiple") {
    // STMIAEQ R13, {R0}
    uint32_t instr = 0x088D0001; // STMIAEQ R13, {R0}
    REQUIRE(decodeInstruction(instr) == "STMIAEQ R13, {R0}");
  }

  SECTION("LDMIANE - conditional load multiple") {
    // LDMIANE R13!, {R0, R1, R2}
    uint32_t instr = 0x18BD0007; // LDMIANE R13!, {R0, R1, R2}
    REQUIRE(decodeInstruction(instr) == "LDMIANE R13!, {R0, R1, R2}");
  }
}
