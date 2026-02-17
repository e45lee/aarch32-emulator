#!/usr/bin/env python3
"""
Creates an AArch32 binary that echoes input characters until 'Q' is received.

Uses MMIO addresses:
- 0x9000000: Console output (write byte)
- 0x9000004: Console input (read byte)
"""

import struct

def write_instructions(filename, instructions):
    """Write list of 32-bit instructions to binary file"""
    with open(filename, 'wb') as f:
        for instr in instructions:
            f.write(struct.pack('<I', instr))
    print(f"Created {filename} with {len(instructions)} instructions")

# Build the echo program
instructions = [
    # Initialize R4 = 0x09000004 (CONSOLE_IN_ADDR)
    0xE3A04009,  # MOV R4, #0x09
    0xE1A04C04,  # MOV R4, R4, LSL #24  (R4 = 0x09000000)
    0xE2844004,  # ADD R4, R4, #4       (R4 = 0x09000004)

    # Initialize R5 = 0x09000000 (CONSOLE_OUT_ADDR)
    0xE3A05009,  # MOV R5, #0x09
    0xE1A05C05,  # MOV R5, R5, LSL #24  (R5 = 0x09000000)

    # Initialize R1 = 0x51 ('Q')
    0xE3A01051,  # MOV R1, #0x51

    # loop: (starts at instruction 6)
    0xE5F40000,  # LDRB R0, [R4]        ; Read byte from console input
    0xE3500000,  # CMP R0, #0           ; Compare with null
    0x0A000003,  # BEQ done             ; Exit if null (skip to instruction 13)
    0xE1500001,  # CMP R0, R1           ; Compare with 'Q'
    0x0A000001,  # BEQ done             ; Exit if equal (skip to instruction 13)
    0xE5C50000,  # STRB R0, [R5]        ; Write byte to console output
    0xEAFFFFF8,  # B loop               ; Jump back to loop (instruction 6)

    # done:
    0xE1A0F00E   # MOV PC, LR           ; Halt
]

write_instructions('echo_until_q.bin', instructions)

# Also create a version that echoes and adds newline after each character
instructions_with_newline = [
    # Initialize R4 = 0x09000004 (CONSOLE_IN_ADDR)
    0xE3A04009,  # MOV R4, #0x09
    0xE1A04C04,  # MOV R4, R4, LSL #24
    0xE2844004,  # ADD R4, R4, #4

    # Initialize R5 = 0x09000000 (CONSOLE_OUT_ADDR)
    0xE3A05009,  # MOV R5, #0x09
    0xE1A05C05,  # MOV R5, R5, LSL #24

    # Initialize R1 = 0x51 ('Q'), R6 = 0x0A ('\n')
    0xE3A01051,  # MOV R1, #0x51
    0xE3A0600A,  # MOV R6, #0x0A        ; Newline character

    # loop: (starts at instruction 7)
    0xE5F40000,  # LDRB R0, [R4]        ; Read byte from console input
    0xE3500000,  # CMP R0, #0           ; Compare with null
    0x0A000004,  # BEQ done             ; Exit if null (skip to instruction 15)
    0xE1500001,  # CMP R0, R1           ; Compare with 'Q'
    0x0A000002,  # BEQ done             ; Exit if 'Q' (skip to instruction 15)
    0xE5C50000,  # STRB R0, [R5]        ; Write byte (echo character)
    0xE5C56000,  # STRB R6, [R5]        ; Write byte (newline)
    0xEAFFFFF7,  # B loop               ; Loop back to instruction 7

    # done:
    0xE1A0F00E   # MOV PC, LR           ; Halt
]

write_instructions('echo_with_newline.bin', instructions_with_newline)

print("\nUsage:")
print("  echo 'Hello World Q' | ./build/aarch32-cli echo_until_q.bin")
print("  echo -e 'A\\nB\\nC\\nQ' | ./build/aarch32-cli echo_with_newline.bin")
