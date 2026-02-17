/**
    Code for decoding/encoding instructions and displaying in a human readable
    format.
 */

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>
#include <string>

std::string decodeInstruction(uint32_t raw_instruction);

union AArch32Instruction {
    uint32_t raw; // The raw 32-bit instruction

    // Data processing instructions (immediate operand)
    struct {
        uint32_t imm : 8;
        uint32_t rotate : 4;
        uint32_t rd : 4;
        uint32_t rn : 4;
        uint32_t s : 1;
        uint32_t opcode : 4;
        uint32_t i : 1;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } data_imm;

    // Data processing instructions (register operand with shift)
    struct {
        uint32_t rm : 4;
        uint32_t unused_bit4 : 1;
        uint32_t shift_type : 2;
        uint32_t shift_imm : 5;
        uint32_t rd : 4;
        uint32_t rn : 4;
        uint32_t s : 1;
        uint32_t opcode : 4;
        uint32_t i : 1;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } data_reg;

    // Branch instructions
    struct {
        uint32_t offset : 24;
        uint32_t link : 1;
        uint32_t unused_bit25 : 1;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } branch;

    // Load/Store instructions
    struct {
        uint32_t offset : 12;
        uint32_t rd : 4;
        uint32_t rn : 4;
        uint32_t load : 1;
        uint32_t write_back : 1;
        uint32_t byte : 1;
        uint32_t up : 1;
        uint32_t pre_indexed : 1;
        uint32_t immediate : 1;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } ls;

    // Load/Store instructions (offset-register variant)
    struct {
        uint32_t rm : 4;
        uint32_t unused_bit4 : 1;
        uint32_t shift_type : 2;
        uint32_t shift_imm : 5;
        uint32_t rd : 4;
        uint32_t rn : 4;
        uint32_t load : 1;
        uint32_t write_back : 1;
        uint32_t byte : 1;
        uint32_t up : 1;
        uint32_t pre_indexed : 1;
        uint32_t immediate : 1;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } ls_reg;


    // Branch and exchange instructions
    struct {
        uint32_t rm : 4;
        uint32_t bx_kind : 3;
        uint32_t b7zero : 1;
        uint32_t b819one : 12;
        uint32_t b20zero : 1;
        uint32_t op0 : 2;
        uint32_t op : 3;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } bx;

    // Multiply instructions (MUL, MLA)
    struct {
        uint32_t rm : 4;       // Multiplier
        uint32_t fixed1001 : 4; // Should be 0b1001
        uint32_t rs : 4;        // Multiplicand
        uint32_t rn : 4;        // Accumulate register (for MLA)
        uint32_t rd : 4;        // Destination
        uint32_t s : 1;         // Set flags
        uint32_t a : 1;         // Accumulate bit (0=MUL, 1=MLA)
        uint32_t fixed000000 : 6; // Should be 0b000000
        uint32_t cond : 4;
    } mul;

    // Divide instructions (SDIV, UDIV)
    struct {
        uint32_t rn : 4;        // Dividend
        uint32_t fixed0001 : 4; // Should be 0b0001
        uint32_t rm : 4;        // Divisor
        uint32_t fixed1111 : 4; // Should be 0b1111
        uint32_t rd : 4;        // Destination
        uint32_t op : 3;        // 001 for SDIV, 011 for UDIV
        uint32_t fixed110 : 3;  // Should be 0b110
        uint32_t kind : 2;      // Should be 0b01
        uint32_t cond : 4;
    } div;

    // Common fields accessible from any instruction
    struct {
        uint32_t low_bits : 26;
        uint32_t kind : 2;
        uint32_t cond : 4;
    } common;
};

#endif
