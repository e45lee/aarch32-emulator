#include <catch2/catch_test_macros.hpp>
#include "../instruction.h"

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
