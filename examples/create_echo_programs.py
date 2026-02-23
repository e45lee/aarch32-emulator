#!/usr/bin/env python3
"""
Creates AArch32 binaries for echo programs using MMIO console I/O.

MMIO addresses:
- 0x9000000: Console output (write byte)
- 0x9000004: Console input (read word - returns character 0x00-0xFF, or -1 on EOF/failure)
"""

import struct

def write_instructions(filename, instructions):
    """Write list of 32-bit instructions to binary file"""
    with open(filename, 'wb') as f:
        for instr in instructions:
            f.write(struct.pack('<I', instr))
    print(f"Created {filename} with {len(instructions)} instructions")


# Program 1: Echo until EOF
echo_until_eof = [
    # Initialize R4 = 0x09000004 (CONSOLE_IN_ADDR)
    0xE3A04009,  # MOV R4, #0x09
    0xE1A04C04,  # MOV R4, R4, LSL #24  (R4 = 0x09000000)
    0xE2844004,  # ADD R4, R4, #4       (R4 = 0x09000004)

    # Initialize R5 = 0x09000000 (CONSOLE_OUT_ADDR)
    0xE3A05009,  # MOV R5, #0x09
    0xE1A05C05,  # MOV R5, R5, LSL #24  (R5 = 0x09000000)

    # loop: (starts at instruction 5)
    0xE5940000,  # LDR R0, [R4]         ; Read word (char or -1 on EOF)
    0xE3700001,  # CMN R0, #1           ; R0 + 1; sets Z if R0 == -1
    0x0A000001,  # BEQ done             ; Exit if EOF/failure (offset +1 to instruction 9)
    0xE5C50000,  # STRB R0, [R5]        ; Echo character
    0xEAFFFFFA,  # B loop               ; Loop back (offset -6 to instruction 5)

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

    # Initialize R2 = 0x51 ('Q')
    0xE3A02051,  # MOV R2, #0x51

    # loop: (starts at instruction 6)
    0xE5940000,  # LDR R0, [R4]         ; Read word (char or -1 on EOF)
    0xE3700001,  # CMN R0, #1           ; R0 + 1; sets Z if R0 == -1
    0x0A000003,  # BEQ done             ; Exit if EOF/failure (offset +3 to instruction 12)
    0xE1500002,  # CMP R0, R2           ; Compare with 'Q'
    0x0A000001,  # BEQ done             ; Exit if 'Q' (offset +1 to instruction 12)
    0xE5C50000,  # STRB R0, [R5]        ; Echo character
    0xEAFFFFF8,  # B loop               ; Loop back (offset -8 to instruction 6)

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

    # Initialize R7 = 0x0A ('\n')
    0xE3A0700A,  # MOV R7, #0x0A

    # loop: (starts at instruction 6)
    0xE5940000,  # LDR R0, [R4]         ; Read word (char or -1 on EOF)
    0xE3700001,  # CMN R0, #1           ; R0 + 1; sets Z if R0 == -1
    0x0A000002,  # BEQ done             ; Exit if EOF/failure (offset +2 to instruction 11)
    0xE5C50000,  # STRB R0, [R5]        ; Echo character
    0xE5C57000,  # STRB R7, [R5]        ; Write newline
    0xEAFFFFF9,  # B loop               ; Loop back (offset -7 to instruction 6)

    # done:
    0xE1A0F00E   # MOV PC, LR           ; Halt
]

write_instructions('echo_with_newline.bin', echo_with_newline)


print("\nUsage:")
print("  echo 'Hello World' | ./build/aarch32-cli echo_until_eof.bin")
print("  echo 'Hello World Q After' | ./build/aarch32-cli echo_until_q.bin")
print("  echo -e 'A\\nB\\nC' | ./build/aarch32-cli echo_with_newline.bin")
