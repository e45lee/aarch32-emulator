/**
    emulator_ui.h

    Header file for the emulator UI components, including state management
    and display functions for registers, disassembly, stack, and console.
*/

#ifndef EMULATOR_UI_H
#define EMULATOR_UI_H

#include "cpu.h"
#include "mmio.h"
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>

// MMIO addresses for console I/O
const uint32_t CONSOLE_OUT_ADDR = 0x9000000;
const uint32_t CONSOLE_IN_ADDR = 0x9000004;
const uint32_t CONSOLE_STATUS_ADDR =
    0x9000008; // bit 0: has input, bit 1: can output

/**
 * Structure to hold the emulator's runtime state for the UI
 */
struct EmulatorState {
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<MemoryMappedIO> memory;
  std::deque<std::string> console_output;
  std::deque<char> console_input;
  bool console_eof = false;
  std::string console_current_input;
  bool console_focused = false;
  bool running = false;
  bool step_requested = false;
  int max_console_lines = 20;
  std::string status_message = "Ready";
  uint32_t memory_view_address = 0x00000000;
  std::string memory_address_input = "0x00000000";
  int current_tab = 0; // 0 = main view, 1 = memory view
  std::string loaded_filename =
      ""; // Filename of loaded binary (empty if test program)
  uint32_t initial_registers[13] = {0}; // Initial values for R0-R12
  bool has_initial_registers[13] = {
      false}; // Track which registers have custom initial values
  std::set<int>
      last_written_registers; // Registers written by the last instruction
};

/**
 * Set up MMIO handlers for console I/O
 */
void setupConsoleIO(EmulatorState &state);

/**
 * Load a simple test program into memory
 */
void loadTestProgram(EmulatorState &state);

/**
 * Load a binary file into memory at the specified address
 * Returns true on success, false on failure
 */
bool loadBinaryFile(EmulatorState &state, const std::string &filename,
                    uint32_t load_address = 0x00000000);

/**
 * Get disassembly view around the current PC
 */
std::vector<std::string>
getDisassembly(EmulatorState &state, int lines_before = 5, int lines_after = 5);

/**
 * Get stack view starting from the stack pointer
 */
std::vector<std::string> getStackView(EmulatorState &state,
                                      int num_entries = 16);

/**
 * Get register file display
 */
std::vector<std::string> getRegisterView(EmulatorState &state);

/**
 * Format a register value as a string
 */
std::string formatRegister(const std::string &name, uint32_t value);

/**
 * Get memory view around a specified address
 */
std::vector<std::string> getMemoryView(EmulatorState &state, uint32_t address,
                                       int bytes_before = 64,
                                       int bytes_after = 64);

/**
 * Apply initial register values from state to the CPU
 */
void applyInitialRegisters(EmulatorState &state);

/**
 * Exception that is thrown when there's no input
 */
class NoInputException : public std::exception {
public:
  const char *what() const noexcept override { return "No input available"; }
};

#endif // EMULATOR_UI_H
