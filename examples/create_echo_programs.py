#!/usr/bin/env python3
"""
Creates AArch32 binaries for echo programs using MMIO console I/O.

MMIO addresses:
- 0x9000000: Console output (write byte)
- 0x9000004: Console input (read byte)
- 0x9000008: Console status (1 if EOF, 0 if not EOF)
"""

import struct

def write_instructions(filename, instructions):
    """Write list of 32-bit instructions to binary file"""
    with open(filename, 'wb') as f:
        for instr in instructions:
            f.write(struct.pack('<I', instr))
    print(f"Created {filename} with {len(instructions)} instructions")


# Program 1: Echo until EOF (using status register)
echo_until_eof = [
    # Initialize R4 = 0x09000004 (CONSOLE_IN_ADDR)
    0xE3A04009,  # MOV R4, #0x09
    0xE1A04C04,  # MOV R4, R4, LSL #24  (R4 = 0x09000000)
    0xE2844004,  # ADD R4, R4, #4       (R4 = 0x09000004)

    # Initialize R5 = 0x09000000 (CONSOLE_OUT_ADDR)
    0xE3A05009,  # MOV R5, #0x09
    0xE1A05C05,  # MOV R5, R5, LSL #24  (R5 = 0x09000000)

    # Initialize R6 = 0x09000008 (CONSOLE_STATUS_ADDR)
    0xE3A06009,  # MOV R6, #0x09
    0xE1A06C06,  # MOV R6, R6, LSL #24  (R6 = 0x09000000)
    0xE2866008,  # ADD R6, R6, #8       (R6 = 0x09000008)

    # loop: (starts at instruction 9)
    0xE5F61000,  # LDRB R1, [R6]        ; Read status
    0xE3510001,  # CMP R1, #1           ; Check if EOF (status == 1)
    0x0A000002,  # BEQ done             ; Exit if EOF (offset +2 to position 15)
    0xE5F40000,  # LDRB R0, [R4]        ; Read character
    0xE5C50000,  # STRB R0, [R5]        ; Echo character
    0xEAFFFFF9,  # B loop               ; Loop back (offset -7 to position 9)

    # done:
    0xE1A0F00E   # MOV PC, LR           ; Halt
]

write_instructions('echo_until_eof.bin', echo_until_eof)


# Program 2: Echo until 'Q' or EOF
echo_until_q = [
    # Initialize R4 = 0x09000004 (CONSOLE_IN_ADDR)
    0xE3A04009,  # MOV R4, #0x09
    0xE1A04C04,  # MOV R4, R4, LSL #24
    0xE2844004,  # ADD R4, R4, #4

    # Initialize R5 = 0x09000000 (CONSOLE_OUT_ADDR)
    0xE3A05009,  # MOV R5, #0x09
    0xE1A05C05,  # MOV R5, R5, LSL #24

    # Initialize R6 = 0x09000008 (CONSOLE_STATUS_ADDR)
    0xE3A06009,  # MOV R6, #0x09
    0xE1A06C06,  # MOV R6, R6, LSL #24
    0xE2866008,  # ADD R6, R6, #8

    # Initialize R2 = 0x51 ('Q')
    0xE3A02051,  # MOV R2, #0x51

    # loop: (starts at instruction 10)
    0xE5F61000,  # LDRB R1, [R6]        ; Read status
    0xE3510001,  # CMP R1, #1           ; Check if EOF
    0x0A000004,  # BEQ done             ; Exit if EOF (offset +4 to position 18)
    0xE5F40000,  # LDRB R0, [R4]        ; Read character
    0xE1500002,  # CMP R0, R2           ; Compare with 'Q'
    0x0A000001,  # BEQ done             ; Exit if 'Q' (offset +1 to position 18)
    0xE5C50000,  # STRB R0, [R5]        ; Echo character
    0xEAFFFFF8,  # B loop               ; Loop back (offset -8 to position 10)

    # done:
    0xE1A0F00E   # MOV PC, LR           ; Halt
]

write_instructions('echo_until_q.bin', echo_until_q)


# Program 3: Echo with newline after each character, until EOF
echo_with_newline = [
    # Initialize R4 = 0x09000004 (CONSOLE_IN_ADDR)
    0xE3A04009,  # MOV R4, #0x09
    0xE1A04C04,  # MOV R4, R4, LSL #24
    0xE2844004,  # ADD R4, R4, #4

    # Initialize R5 = 0x09000000 (CONSOLE_OUT_ADDR)
    0xE3A05009,  # MOV R5, #0x09
    0xE1A05C05,  # MOV R5, R5, LSL #24

    # Initialize R6 = 0x09000008 (CONSOLE_STATUS_ADDR)
    0xE3A06009,  # MOV R6, #0x09
    0xE1A06C06,  # MOV R6, R6, LSL #24
    0xE2866008,  # ADD R6, R6, #8

    # Initialize R7 = 0x0A ('\n')
    0xE3A0700A,  # MOV R7, #0x0A

    # loop: (starts at instruction 11)
    0xE5F61000,  # LDRB R1, [R6]        ; Read status
    0xE3510001,  # CMP R1, #1           ; Check if EOF
    0x0A000003,  # BEQ done             ; Exit if EOF (offset +3 to position 17)
    0xE5F40000,  # LDRB R0, [R4]        ; Read character
    0xE5C50000,  # STRB R0, [R5]        ; Echo character
    0xE5C57000,  # STRB R7, [R5]        ; Write newline
    0xEAFFFFF8,  # B loop               ; Loop back (offset -8 to position 11)

    # done:
    0xE1A0F00E   # MOV PC, LR           ; Halt
]

write_instructions('echo_with_newline.bin', echo_with_newline)


print("\nUsage:")
print("  echo 'Hello World' | ./build/aarch32-cli echo_until_eof.bin")
print("  echo 'Hello World Q After' | ./build/aarch32-cli echo_until_q.bin")
print("  echo -e 'A\\nB\\nC' | ./build/aarch32-cli echo_with_newline.bin")
