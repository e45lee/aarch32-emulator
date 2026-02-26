/**
    instruction.cpp
    Code for decoding an ARM instruction to a human-readable string.
 */
#include "Instruction.hpp"

std::string decodeInstruction(uint32_t raw_instruction) {
  AArch32Instruction instr;
  instr.raw = raw_instruction;

  // Condition code names
  const char *cond_names[] = {"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
                              "HI", "LS", "GE", "LT", "GT", "LE", "",   "NV"};

  std::string cond_suffix = cond_names[instr.common.cond];

  // Check for SDIV/UDIV instruction (special case within kind 01)
  if (instr.common.kind == 0b01 && instr.div.fixed110 == 0b110 &&
      instr.div.fixed1111 == 0b1111 && instr.div.fixed0001 == 0b0001) {
    // SDIV or UDIV instruction
    std::string result = (instr.div.op == 0b001) ? "SDIV" : "UDIV";
    result += cond_suffix;
    result += " R" + std::to_string(instr.div.rd);
    result += ", R" + std::to_string(instr.div.rn);
    result += ", R" + std::to_string(instr.div.rm);
    return result;
  }

  // Check for MUL/MLA instruction (special case within kind 00)
  if (instr.common.kind == 0b00 && instr.mul.fixed000000 == 0b000000 &&
      instr.mul.fixed1001 == 0b1001) {
    // MUL or MLA instruction
    std::string result = instr.mul.a ? "MLA" : "MUL";
    result += cond_suffix;
    if (instr.mul.s) {
      result += "S";
    }
    result += " R" + std::to_string(instr.mul.rd);
    result += ", R" + std::to_string(instr.mul.rm);
    result += ", R" + std::to_string(instr.mul.rs);
    if (instr.mul.a) {
      result += ", R" + std::to_string(instr.mul.rn);
    }
    return result;
  }

  // Check for BX/BLX instruction (special case within kind 00)
  if (instr.common.kind == 0b00 && instr.bx.op == 0b010 &&
      instr.bx.op0 == 0b01 && instr.bx.b20zero == 0 &&
      instr.bx.b819one == 0xFFF && instr.bx.b7zero == 0) {
    // BX or BLX instruction
    if (instr.bx.bx_kind == 0b001) {
      return "BX" + cond_suffix + " R" + std::to_string(instr.bx.rm);
    } else if (instr.bx.bx_kind == 0b011) {
      return "BLX" + cond_suffix + " R" + std::to_string(instr.bx.rm);
    }
  }

  // Decode based on instruction kind (bits 27-26)
  switch (instr.common.kind) {
  case 0b00: { // Data processing
    const char *opcodes[] = {"AND", "EOR", "SUB", "RSB", "ADD", "ADC",
                             "SBC", "RSC", "TST", "TEQ", "CMP", "CMN",
                             "ORR", "MOV", "BIC", "MVN"};

    // Check if immediate or register operand
    bool is_immediate = instr.data_imm.i;
    uint32_t opcode =
        is_immediate ? instr.data_imm.opcode : instr.data_reg.opcode;
    uint32_t rd = is_immediate ? instr.data_imm.rd : instr.data_reg.rd;
    uint32_t rn = is_immediate ? instr.data_imm.rn : instr.data_reg.rn;
    bool s_flag = is_immediate ? instr.data_imm.s : instr.data_reg.s;

    std::string result = opcodes[opcode];
    result += cond_suffix;

    // Comparison instructions (TST, TEQ, CMP, CMN) don't show S flag or Rd
    bool is_comparison = (opcode >= 8 && opcode <= 11);

    if (!is_comparison && s_flag) {
      result += "S";
    }

    if (!is_comparison) {
      result += " R" + std::to_string(rd);
    } else {
      // For comparison instructions, first operand is Rn
      result += " R" + std::to_string(rn);
    }

    // For MOV, MVN, and comparison instructions, we don't add Rn again
    if (!is_comparison && opcode != 13 && opcode != 15) {
      result += ", R" + std::to_string(rn);
    }

    // Operand2
    if (is_immediate) {
      // Immediate operand
      uint32_t imm = instr.data_imm.imm;
      uint32_t rotate = instr.data_imm.rotate * 2;
      uint32_t rotated_imm = (imm >> rotate) | (imm << (32 - rotate));
      result += ", #" + std::to_string(rotated_imm);
    } else {
      // Register operand
      result += ", R" + std::to_string(instr.data_reg.rm);
      if (instr.data_reg.shift_imm != 0) {
        const char *shift_names[] = {"LSL", "LSR", "ASR", "ROR"};
        result += ", " + std::string(shift_names[instr.data_reg.shift_type]);
        result += " #" + std::to_string(instr.data_reg.shift_imm);
      }
    }
    return result;
  }

  case 0b01: { // Load/Store
    std::string result = instr.ls.load ? "LDR" : "STR";
    result += cond_suffix;
    if (instr.ls.byte) {
      result += "B";
    }
    result += " R" + std::to_string(instr.ls.rd);
    result += ", [R" + std::to_string(instr.ls.rn);

    bool is_immediate = instr.ls.immediate == 0;

    if (is_immediate) {
      if (instr.ls.pre_indexed) {
        // Pre-indexed: [Rn, offset]
        if (instr.ls.offset != 0) {
          result += ", #" + std::string(instr.ls.up ? "" : "-") +
                    std::to_string(instr.ls.offset);
        }
        result += "]";
        if (instr.ls.write_back) {
          result += "!";
        }
      } else {
        // Post-indexed: [Rn], offset
        result += "]";
        if (instr.ls.offset != 0) {
          result += ", #" + std::string(instr.ls.up ? "" : "-") +
                    std::to_string(instr.ls.offset);
        }
      }
    } else {
      std::string offset_reg = "R" + std::to_string(instr.ls_reg.rm);
      if (instr.ls_reg.shift_imm != 0) {
        const char *shift_names[] = {"LSL", "LSR", "ASR", "ROR"};
        offset_reg += ", " + std::string(shift_names[instr.ls_reg.shift_type]);
        offset_reg += " #" + std::to_string(instr.ls_reg.shift_imm);
      }
      if (instr.ls.pre_indexed) {
        // Pre-indexed: [Rn, offset]
        result += ", " + std::string(instr.ls.up ? "" : "-") + offset_reg;
        result += "]";
        if (instr.ls.write_back) {
          result += "!";
        }
      } else {
        // Post-indexed: [Rn], offset
        result += "]";
        result += ", " + std::string(instr.ls.up ? "" : "-") + offset_reg;
      }
    }
    return result;
  }

  case 0b10: { // Branch or Block Data Transfer
    // Check if it's LDM/STM (bits [27:25] = 100) vs Branch (bits [27:25] = 101)
    if (instr.ldm_stm.fixed100 == 0b100) {
      // LDM/STM instruction
      std::string result;

      // Determine addressing mode suffix
      if (instr.ldm_stm.pre_indexed) {
        result = instr.ldm_stm.up ? (instr.ldm_stm.load ? "LDMIB" : "STMIB")
                                  : (instr.ldm_stm.load ? "LDMDB" : "STMDB");
      } else {
        result = instr.ldm_stm.up ? (instr.ldm_stm.load ? "LDMIA" : "STMIA")
                                  : (instr.ldm_stm.load ? "LDMDA" : "STMDA");
      }

      result += cond_suffix;
      result += " R" + std::to_string(instr.ldm_stm.rn);
      if (instr.ldm_stm.write_back) {
        result += "!";
      }
      result += ", {";

      // Build register list
      bool first = true;
      for (int i = 0; i < 16; i++) {
        if (instr.ldm_stm.register_list & (1 << i)) {
          if (!first)
            result += ", ";
          result += "R" + std::to_string(i);
          first = false;
        }
      }
      result += "}";

      return result;
    } else {
      // Branch instruction
      std::string result = instr.branch.link ? "BL" : "B";
      result += cond_suffix;

      // Sign-extend the 24-bit offset to 32 bits and multiply by 4
      int32_t offset = instr.branch.offset;
      // Check if the sign bit (bit 23) is set
      if (offset & 0x800000) {
        offset |= static_cast<int32_t>(0xFF000000); // Sign extend
      }
      offset <<= 2; // Multiply by 4 (PC-relative offset is in words)
      offset += 8;  // Account for PC+8 in ARM pipeline

      result += " " + std::to_string(offset);
      return result;
    }
  }

  default:
    return "UNKNOWN";
  }
}
