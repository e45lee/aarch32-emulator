/**
    cpu_ee.cpp
    Implementation of the CPU execution engine for the AArch32 emulator.
 */

#include "cpu_ee.h"
#include "memory.h"
#include <format>

CPU_ExecutionEngine::CPU_ExecutionEngine(Memory* mem, uint32_t initial_pc) : memory(mem), wrotePC(false), newPC(0) {
    // Initialize registers to 0
    for (int i = 0; i < 16; i++) {
        registers[i] = 0;
    }
    // Set the link register to the halted PC value by default (this is just a convention for our emulator)
    registers[14] = halted_pc;
    // Set the program counter to the initial value
    registers[15] = initial_pc;
    // Initialize CPSR to a default value (e.g., all flags cleared)
    cpsr = 0;
}

bool CPU_ExecutionEngine::didWritePC() const {
    return wrotePC;
}

uint32_t CPU_ExecutionEngine::getNextPC() const {
    if (wrotePC) {
        return newPC;
    } else {
        return registers[15] + 4;
    }
}

void CPU_ExecutionEngine::setNextPC() {
    registers[15] = getNextPC(); // Increment PC by 4 to point to the next instruction
    wrotePC = false; // Reset the flag after incrementing
}

bool CPU_ExecutionEngine::shouldExecuteInstruction(AArch32Instruction instr) {
    // Extract condition code from the instruction
    uint32_t cond = instr.common.cond;

    // Check condition codes against CPSR flags
    bool N = (cpsr >> 31) & 1; // Negative flag
    bool Z = (cpsr >> 30) & 1; // Zero flag
    bool C = (cpsr >> 29) & 1; // Carry flag
    bool V = (cpsr >> 28) & 1; // Overflow flag

    switch (cond) {
        case 0x0: return Z;              // EQ: Equal
        case 0x1: return !Z;             // NE: Not equal
        case 0x2: return C;              // CS/HS: Carry set / Unsigned higher or same
        case 0x3: return !C;             // CC/LO: Carry clear / Unsigned lower
        case 0x4: return N;              // MI: Negative
        case 0x5: return !N;             // PL: Positive or zero
        case 0x6: return V;              // VS: Overflow
        case 0x7: return !V;             // VC: No overflow
        case 0x8: return C && !Z;       // HI: Unsigned higher
        case 0x9: return !C || Z;      // LS: Unsigned lower or same
        case 0xA: return N == V;       // GE: Signed greater than or equal
        case 0xB: return N != V;       // LT: Signed less than
        case 0xC: return !Z && (N == V); // GT: Signed greater than
        case 0xD: return Z || (N != V);   // LE: Signed less than or equal
        case 0xE: return true;           // AL: Always execute
        default: return false;           // NV and undefined conditions do not execute
    }
}

ExecutionResult CPU_ExecutionEngine::executeInstruction(AArch32Instruction instr) {
    ExecutionResult result;

    if (!shouldExecuteInstruction(instr)) {
        return result; // Skip execution if condition is not met
    }

    // Decode and execute the instruction based on its type
    if (instr.common.kind == 0b00) {
        // Check for multiply instruction (special encoding within data processing space)
        if (instr.mul.fixed000000 == 0b000000 && instr.mul.fixed1001 == 0b1001) {
            result = executeMultiply(instr);
        }
        // Check for branch and exchange (special encoding within data processing space)
        // BX encoding: cccc 0001 0010 1111 1111 1111 0001 Rm
        // Check bits 27-4 (ignoring condition and Rm)
        else if (instr.common.kind == 0b00 &&
            instr.bx.op == 0b010 &&
            instr.bx.op0 == 0b01 &&
            instr.bx.b20zero == 0 &&
            instr.bx.b819one == 0xFFF &&
            instr.bx.b7zero == 0) {
            result = executeBranchExchange(instr);
        } else {
            // Handle data processing instructions
            result = executeDataProcessing(instr);
        }
    } else if (instr.common.kind == 0b01) {
        // Check for divide instruction (special encoding within load/store space)
        if (instr.div.fixed110 == 0b110 &&
            instr.div.fixed1111 == 0b1111 &&
            instr.div.fixed0001 == 0b0001) {
            result = executeDivide(instr);
        } else {
            // Handle load/store instructions
            result = executeLoadStore(instr);
        }
    } else if (instr.common.kind == 0b10) {
        // Check for LDM/STM (bits [27:25] = 100) vs Branch (bits [27:25] = 101)
        if (instr.ldm_stm.fixed100 == 0b100) {
            result = executeBlockDataTransfer(instr);
        } else {
            // Handle branch instructions
            result = executeBranch(instr);
        }
    } else {
        uint32_t kind = instr.common.kind;
        throw std::runtime_error(std::format("Unknown instruction type: {}", kind));
    }

    return result;
}

uint32_t CPU_ExecutionEngine::applyShift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, bool& carry_out) {
    if (shift_amount == 0) {
        carry_out = (cpsr >> 29) & 1; // Preserve carry flag
        return value;
    }

    switch (shift_type) {
        case 0b00: // LSL (Logical Shift Left)
            if (shift_amount > 0 && shift_amount <= 32) {
                carry_out = (value >> (32 - shift_amount)) & 1;
            }
            return value << shift_amount;

        case 0b01: // LSR (Logical Shift Right)
            if (shift_amount > 0 && shift_amount <= 32) {
                carry_out = (value >> (shift_amount - 1)) & 1;
            }
            return value >> shift_amount;

        case 0b10: // ASR (Arithmetic Shift Right)
            if (shift_amount > 0 && shift_amount <= 32) {
                carry_out = (value >> (shift_amount - 1)) & 1;
            }
            return (int32_t)value >> shift_amount;

        case 0b11: // ROR (Rotate Right)
            if (shift_amount > 0) {
                shift_amount = shift_amount & 0x1F;
                if (shift_amount > 0) {
                    carry_out = (value >> (shift_amount - 1)) & 1;
                    return (value >> shift_amount) | (value << (32 - shift_amount));
                }
            }
            return value;
    }
    return value;
}

void CPU_ExecutionEngine::updateFlags(uint32_t result, bool carry, bool overflow) {
    // Update N flag (bit 31)
    if (result & 0x80000000) {
        cpsr |= (1 << 31);
    } else {
        cpsr &= ~(1 << 31);
    }

    // Update Z flag (bit 30)
    if (result == 0) {
        cpsr |= (1 << 30);
    } else {
        cpsr &= ~(1 << 30);
    }

    // Update C flag (bit 29)
    if (carry) {
        cpsr |= (1 << 29);
    } else {
        cpsr &= ~(1 << 29);
    }

    // Update V flag (bit 28)
    if (overflow) {
        cpsr |= (1 << 28);
    } else {
        cpsr &= ~(1 << 28);
    }
}

ExecutionResult CPU_ExecutionEngine::executeDataProcessing(AArch32Instruction instr) {
    ExecutionResult execResult;
    uint32_t operand1, operand2, result;
    bool carry = (cpsr >> 29) & 1;
    bool overflow = false;
    bool carry_out = carry;

    // Get first operand (Rn)
    uint32_t rn = instr.data_imm.rn;
    operand1 = (rn == 15) ? (registers[15] + 8) : registers[rn];

    // Get second operand (immediate or register with shift)
    if (instr.data_imm.i == 1) {
        // Immediate operand
        uint32_t imm = instr.data_imm.imm;
        uint32_t rotate = instr.data_imm.rotate * 2;
        operand2 = (imm >> rotate) | (imm << (32 - rotate));
        if (rotate != 0) {
            carry_out = (operand2 >> 31) & 1;
        }
    } else {
        // Register operand with shift
        uint32_t rm = instr.data_reg.rm;
        uint32_t reg_value = (rm == 15) ? (registers[15] + 8) : registers[rm];
        operand2 = applyShift(reg_value, instr.data_reg.shift_type, instr.data_reg.shift_imm, carry_out);
    }

    // Execute operation based on opcode
    uint32_t opcode = instr.data_imm.opcode;
    uint32_t rd = instr.data_imm.rd;
    bool write_result = true;

    switch (opcode) {
        case 0x0: // AND
            result = operand1 & operand2;
            break;
        case 0x1: // EOR
            result = operand1 ^ operand2;
            break;
        case 0x2: // SUB
            result = operand1 - operand2;
            carry_out = operand1 >= operand2;
            overflow = ((operand1 ^ operand2) & (operand1 ^ result)) >> 31;
            break;
        case 0x3: // RSB
            result = operand2 - operand1;
            carry_out = operand2 >= operand1;
            overflow = ((operand2 ^ operand1) & (operand2 ^ result)) >> 31;
            break;
        case 0x4: // ADD
            result = operand1 + operand2;
            carry_out = result < operand1;
            overflow = (~(operand1 ^ operand2) & (operand1 ^ result)) >> 31;
            break;
        case 0x5: // ADC
            result = operand1 + operand2 + carry;
            carry_out = (result < operand1) || (carry && result == operand1);
            overflow = (~(operand1 ^ operand2) & (operand1 ^ result)) >> 31;
            break;
        case 0x6: // SBC
            result = operand1 - operand2 - (1 - carry);
            carry_out = (operand1 >= operand2) && ((operand1 - operand2) >= (1 - carry));
            overflow = ((operand1 ^ operand2) & (operand1 ^ result)) >> 31;
            break;
        case 0x7: // RSC
            result = operand2 - operand1 - (1 - carry);
            carry_out = (operand2 >= operand1) && ((operand2 - operand1) >= (1 - carry));
            overflow = ((operand2 ^ operand1) & (operand2 ^ result)) >> 31;
            break;
        case 0x8: // TST
            result = operand1 & operand2;
            write_result = false;
            break;
        case 0x9: // TEQ
            result = operand1 ^ operand2;
            write_result = false;
            break;
        case 0xA: // CMP
            result = operand1 - operand2;
            carry_out = operand1 >= operand2;
            overflow = ((operand1 ^ operand2) & (operand1 ^ result)) >> 31;
            write_result = false;
            break;
        case 0xB: // CMN
            result = operand1 + operand2;
            carry_out = result < operand1;
            overflow = (~(operand1 ^ operand2) & (operand1 ^ result)) >> 31;
            write_result = false;
            break;
        case 0xC: // ORR
            result = operand1 | operand2;
            break;
        case 0xD: // MOV
            result = operand2;
            break;
        case 0xE: // BIC
            result = operand1 & ~operand2;
            break;
        case 0xF: // MVN
            result = ~operand2;
            break;
        default:
            throw std::runtime_error(std::format("Unknown data processing opcode: {}", opcode));
    }

    // Write result to destination register if needed
    if (write_result) {
        execResult.wroteRegister = true;
        execResult.registersWritten.insert(rd);
        if (rd == 15) {
            wrotePC = true;
            newPC = result & ~3; // Align to 4 bytes
        } else {
            registers[rd] = result;
        }
    }

    // Update flags if S bit is set
    if (instr.data_imm.s == 1) {
        // For logical operations, overflow flag is unchanged
        if (opcode >= 0x0 && opcode <= 0x1 || opcode >= 0x8 && opcode <= 0x9 || opcode >= 0xC && opcode <= 0xF) {
            overflow = (cpsr >> 28) & 1;
        }
        updateFlags(result, carry_out, overflow);
        execResult.wroteCPSR = true;
    }

    return execResult;
}

ExecutionResult CPU_ExecutionEngine::executeLoadStore(AArch32Instruction instr) {
    ExecutionResult execResult;
    uint32_t rn = instr.ls.rn;
    uint32_t rd = instr.ls.rd;
    uint32_t offset = 0;

    // Calculate base address
    uint32_t base = (rn == 15) ? (registers[15] + 8) : registers[rn];
    uint32_t address;

    bool is_immediate = instr.ls.immediate == 0;
    // Calculate offset
    if (is_immediate) {
        offset = instr.ls.offset;
    } else {
        // Register offset with optional shift
        uint32_t rm = instr.ls_reg.rm;
        uint32_t reg_value = (rm == 15) ? (registers[15] + 8) : registers[rm];
        bool carry_out;
        offset = applyShift(reg_value, instr.ls_reg.shift_type, instr.ls_reg.shift_imm, carry_out);
    }

    // Calculate effective address based on addressing mode
    if (instr.ls.pre_indexed) {
        // Pre-indexed: address = base +/- offset
        address = instr.ls.up ? (base + offset) : (base - offset);
    } else {
        // Post-indexed: address = base, then base = base +/- offset
        address = base;
    }

    // Perform load or store
    if (instr.ls.load) {
        // Load instruction - writes to register
        execResult.wroteRegister = true;
        execResult.registersWritten.insert(rd);

        if (instr.ls.byte) {
            // Load byte
            uint8_t value = memory->readByte(address);
            if (rd == 15) {
                wrotePC = true;
                newPC = value & ~3;
            } else {
                registers[rd] = value;
            }
        } else {
            // Load word
            uint32_t value = memory->readWord(address);
            if (rd == 15) {
                wrotePC = true;
                newPC = value & ~3;
            } else {
                registers[rd] = value;
            }
        }
    } else {
        // Store instruction - writes to memory
        execResult.wroteMemory = true;
        execResult.memoryAddress = address;
        execResult.memorySize = instr.ls.byte ? 1 : 4;

        uint32_t value = (rd == 15) ? (registers[15] + 8) : registers[rd];
        if (instr.ls.byte) {
            // Store byte
            memory->writeByte(address, value & 0xFF);
        } else {
            // Store word
            memory->writeWord(address, value);
        }
    }

    // Write-back if needed
    if (instr.ls.write_back || !instr.ls.pre_indexed) {
        uint32_t new_base = instr.ls.up ? (base + offset) : (base - offset);
        if (rn != 15) { // Don't write back to PC
            registers[rn] = new_base;
            execResult.wroteRegister = true;
            execResult.registersWritten.insert(rn);
        }
    }

    return execResult;
}

ExecutionResult CPU_ExecutionEngine::executeBranch(AArch32Instruction instr) {
    ExecutionResult execResult;

    // Sign-extend the 24-bit offset to 32 bits
    int32_t offset = instr.branch.offset;
    if (offset & 0x800000) {
        offset |= 0xFF000000;
    }

    // Shift left by 2 (multiply by 4)
    offset <<= 2;

    // If link bit is set, save return address to R14
    if (instr.branch.link) {
        registers[14] = registers[15] + 4; // Save return address
        execResult.wroteRegister = true;
        execResult.registersWritten.insert(14);
    }

    // Calculate new PC (current PC + 8 + offset)
    // All branch instructions write to PC (R15)
    wrotePC = true;
    newPC = registers[15] + 8 + offset;
    execResult.wroteRegister = true;
    execResult.registersWritten.insert(15);

    return execResult;
}

ExecutionResult CPU_ExecutionEngine::executeBranchExchange(AArch32Instruction instr) {
    ExecutionResult execResult;
    uint32_t rm = instr.bx.rm;
    uint32_t target = registers[rm];

    // Branch to the address in Rm
    // In a full implementation, we would check bit 0 to switch to Thumb mode
    // For now, we'll just branch to the address (assuming ARM mode)
    wrotePC = true;
    newPC = target & ~3; // Align to 4 bytes for ARM mode

    // BX writes to PC (R15)
    execResult.wroteRegister = true;
    execResult.registersWritten.insert(15);

    return execResult;
}

ExecutionResult CPU_ExecutionEngine::executeMultiply(AArch32Instruction instr) {
    ExecutionResult execResult;

    // Get operands
    uint32_t rm = registers[instr.mul.rm];
    uint32_t rs = registers[instr.mul.rs];
    uint32_t rn = registers[instr.mul.rn];

    // Compute result
    uint32_t result;
    if (instr.mul.a) {
        // MLA: Multiply-accumulate (Rd = Rm * Rs + Rn)
        result = rm * rs + rn;
    } else {
        // MUL: Multiply (Rd = Rm * Rs)
        result = rm * rs;
    }

    // Write result to destination register
    registers[instr.mul.rd] = result;
    execResult.wroteRegister = true;
    execResult.registersWritten.insert(instr.mul.rd);

    // Update flags if S bit is set
    if (instr.mul.s) {
        // Update N and Z flags
        bool N = (result >> 31) & 1;
        bool Z = (result == 0);

        // C flag is UNPREDICTABLE after multiply
        // V flag is UNPREDICTABLE after multiply
        // We'll preserve existing C and V flags
        bool C = (cpsr >> 29) & 1;
        bool V = (cpsr >> 28) & 1;

        // Update CPSR
        cpsr = (N << 31) | (Z << 30) | (C << 29) | (V << 28) | (cpsr & 0x0FFFFFFF);
        execResult.wroteCPSR = true;
    }

    return execResult;
}

ExecutionResult CPU_ExecutionEngine::executeDivide(AArch32Instruction instr) {
    ExecutionResult execResult;

    // Get operands
    uint32_t rn = registers[instr.div.rn]; // Dividend
    uint32_t rm = registers[instr.div.rm]; // Divisor

    // Compute result
    uint32_t result;
    if (rm == 0) {
        // Division by zero: result is UNPREDICTABLE per ARM spec
        // We return 0 as is common in many implementations
        result = 0;
    } else if (instr.div.op == 0b001) {
        // SDIV: Signed divide (Rd = Rn / Rm)
        int32_t signed_dividend = static_cast<int32_t>(rn);
        int32_t signed_divisor = static_cast<int32_t>(rm);
        result = static_cast<uint32_t>(signed_dividend / signed_divisor);
    } else {
        // UDIV: Unsigned divide (Rd = Rn / Rm)
        result = rn / rm;
    }

    // Write result to destination register
    registers[instr.div.rd] = result;
    execResult.wroteRegister = true;
    execResult.registersWritten.insert(instr.div.rd);

    // Division instructions do not update flags

    return execResult;
}

ExecutionResult CPU_ExecutionEngine::executeBlockDataTransfer(AArch32Instruction instr) {
    ExecutionResult execResult;

    uint32_t base_reg = instr.ldm_stm.rn;
    uint32_t address = registers[base_reg];
    uint32_t register_list = instr.ldm_stm.register_list;

    // Count number of registers in the list
    int num_registers = 0;
    for (int i = 0; i < 16; i++) {
        if (register_list & (1 << i)) {
            num_registers++;
        }
    }

    // Calculate start address based on addressing mode
    uint32_t start_address = address;
    if (!instr.ldm_stm.up) {
        // Decrement modes: DA or DB
        if (instr.ldm_stm.pre_indexed) {
            // DB (Decrement Before): start at base - num_regs*4
            start_address = address - (num_registers * 4);
        } else {
            // DA (Decrement After): start at base - (num_regs-1)*4
            start_address = address - (num_registers * 4) + 4;
        }
    } else {
        // Increment modes: IA or IB
        if (instr.ldm_stm.pre_indexed) {
            // IB (Increment Before): start at base + 4
            start_address = address + 4;
        } else {
            // IA (Increment After): start at base
            start_address = address;
        }
    }

    // Perform load/store for each register in the list
    uint32_t current_address = start_address;
    for (int i = 0; i < 16; i++) {
        if (register_list & (1 << i)) {
            if (instr.ldm_stm.load) {
                // LDM: Load register from memory
                uint32_t value = memory->readWord(current_address);

                if (i == 15) {
                    // Loading PC
                    wrotePC = true;
                    newPC = value & ~3;
                } else {
                    registers[i] = value;
                }

                execResult.wroteRegister = true;
                execResult.registersWritten.insert(i);
            } else {
                // STM: Store register to memory
                uint32_t value = (i == 15) ? (registers[15] + 8) : registers[i];
                memory->writeWord(current_address, value);

                execResult.wroteMemory = true;
                execResult.memoryAddress = current_address;
                execResult.memorySize = 4;
            }

            current_address += 4;
        }
    }

    // Write-back: update base register
    if (instr.ldm_stm.write_back) {
        uint32_t final_address;
        if (instr.ldm_stm.up) {
            final_address = address + (num_registers * 4);
        } else {
            final_address = address - (num_registers * 4);
        }

        registers[base_reg] = final_address;
        execResult.wroteRegister = true;
        execResult.registersWritten.insert(base_reg);
    }

    return execResult;
}

uint32_t CPU_ExecutionEngine::getRegister(int reg) const {
    return registers[reg];
}

uint32_t CPU_ExecutionEngine::getCPSR() const {
    return cpsr;
}

void CPU_ExecutionEngine::setRegister(int reg, uint32_t value) {
    registers[reg] = value;
}