/**
    main.cpp

    Entry point for the AArch32 emulator with FTXUI interface.
*/

#include <chrono>
#include <cstdint>
#include <fstream>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <thread>

#include "CPU.hpp"
#include "MemoryMappedIO.hpp"
#include "emulator_ui.h"
#include "ui_renderer.h"

using namespace ftxui;

bool loadIntListFile(MemoryMappedIO *memory, const std::string &filename,
                     uint32_t start_address, size_t &loaded_count) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open int list file: " << filename
              << std::endl;
    return false;
  }

  loaded_count = 0;
  std::string line;
  while (std::getline(file, line)) {
    for (char &ch : line) {
      if (ch == ',') {
        ch = ' ';
      }
    }

    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
      try {
        long long parsed = std::stoll(token, nullptr, 0);
        if (parsed < std::numeric_limits<int32_t>::min() ||
            parsed > std::numeric_limits<int32_t>::max()) {
          std::cerr << "Error: Integer out of 32-bit signed range in int list "
                       "file: "
                    << token << std::endl;
          return false;
        }

        if (loaded_count >
            (std::numeric_limits<uint32_t>::max() - start_address) / 4) {
          std::cerr << "Error: Address overflow while loading int list file"
                    << std::endl;
          return false;
        }

        uint32_t value = static_cast<uint32_t>(static_cast<int32_t>(parsed));
        memory->writeWord(start_address + static_cast<uint32_t>(loaded_count * 4),
                          value);
        loaded_count++;
      } catch (const std::exception &) {
        std::cerr << "Error: Invalid integer token in int list file: " << token
                  << std::endl;
        return false;
      }
    }
  }

  return true;
}

/**
 *
 * Memory map
 *  Lower half: 0x00000000 to 0x00FFFFFF (16 MiB) - General memory
 *  MMIO: 0x9000000 to 0x900000F - Memory-mapped I/O for console
 *  Upper half: 0xFF000000 to 0xFFFFFFFF (16 MiB) - Upper memory (for
 * heap/stack)
 */

int main(int argc, char *argv[]) {
  // Create emulator state
  EmulatorState state;
  state.memory = std::make_shared<MemoryMappedIO>();
  state.cpu = std::make_shared<CPU>(state.memory.get(), 0x00000000);

  // Set up console I/O handlers
  setupConsoleIO(state);

  // Parse command line arguments
  std::string binary_filename = "";
  std::string int_list_filename;
  uint32_t int_list_start_address = 0;
  bool has_int_file = false;
  bool has_int_addr = false;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    // Check for register flags --r0 through --r12
    if (arg.substr(0, 3) == "--r" && arg.length() >= 4) {
      std::string reg_num_str = arg.substr(3);
      try {
        int reg_num = std::stoi(reg_num_str);
        if (reg_num >= 0 && reg_num <= 12) {
          // Next argument should be the value
          if (i + 1 < argc) {
            i++;
            std::string value_str = argv[i];
            uint32_t value = std::stoul(
                value_str, nullptr, 0); // Auto-detect base (0x for hex, etc.)
            state.initial_registers[reg_num] = value;
            state.has_initial_registers[reg_num] = true;
          } else {
            std::cerr << "Error: " << arg << " requires a value" << std::endl;
            return 1;
          }
        } else {
          std::cerr << "Error: Invalid register number: " << arg << std::endl;
          std::cerr << "Valid registers are --r0 through --r12" << std::endl;
          return 1;
        }
      } catch (...) {
        // Not a register flag, treat as filename
        if (binary_filename.empty() && arg[0] != '-') {
          binary_filename = arg;
        }
      }
    } else if (arg == "--int-file") {
      if (i + 1 >= argc) {
        std::cerr << "Error: --int-file requires a file path" << std::endl;
        return 1;
      }
      i++;
      int_list_filename = argv[i];
      has_int_file = true;
    } else if (arg == "--int-addr") {
      if (i + 1 >= argc) {
        std::cerr << "Error: --int-addr requires an address value" << std::endl;
        return 1;
      }
      i++;
      try {
        int_list_start_address = std::stoul(argv[i], nullptr, 0);
        has_int_addr = true;
      } catch (...) {
        std::cerr << "Error: Invalid address value for --int-addr: " << argv[i]
                  << std::endl;
        return 1;
      }
    } else if (arg[0] == '-') {
      std::cerr << "Error: Unknown option: " << arg << std::endl;
      std::cerr << "Usage: " << argv[0]
                << " [binary_file] [--r0 VALUE] ... [--r12 VALUE] [--int-file "
                   "FILE --int-addr ADDRESS]"
                << std::endl;
      return 1;
    } else if (arg[0] != '-') {
      // Positional argument - binary filename
      if (binary_filename.empty()) {
        binary_filename = arg;
      }
    }
  }

  if (has_int_file != has_int_addr) {
    std::cerr << "Error: --int-file and --int-addr must be provided together"
              << std::endl;
    return 1;
  }

  // Load program from file or use test program
  if (!binary_filename.empty()) {
    // Load binary file from command line argument
    if (!loadBinaryFile(state, binary_filename, 0x00000000)) {
      std::cerr << "Failed to load binary file: " << binary_filename
                << std::endl;
      std::cerr << "Usage: " << argv[0]
                << " [binary_file] [--r0 VALUE] ... [--r12 VALUE]" << std::endl;
      return 1;
    }
    state.loaded_filename = binary_filename;
    state.status_message = "Loaded: " + binary_filename;
  } else {
    // No file provided, load test program
    loadTestProgram(state);
    state.loaded_filename = ""; // Empty string indicates test program
    state.status_message = "Test program loaded";
  }

  if (has_int_file) {
    size_t loaded_count = 0;
    if (!loadIntListFile(state.memory.get(), int_list_filename,
                         int_list_start_address, loaded_count)) {
      return 1;
    }

    std::ostringstream status;
    status << state.status_message << " | loaded " << loaded_count
           << " ints @ 0x" << std::hex << int_list_start_address;
    state.status_message = status.str();
  }

  // Apply initial register values
  applyInitialRegisters(state);

  // Create screen
  auto screen = ScreenInteractive::Fullscreen();

  // Create UI components
  auto renderer = createUIRenderer(state);
  auto component = createEventHandler(renderer, state, screen);

  // Create main loop
  Loop loop(&screen, component);

  // Start the loop
  loop.RunOnce();

  // Main execution loop
  while (!loop.HasQuitted()) {
    // Execute instruction if running
    if (state.running && !state.cpu->isHalted()) {
      try {
        ExecutionResult result = state.cpu->step();
        state.last_written_registers = result.registersWritten;
        if (state.cpu->isHalted()) {
          state.running = false;
          state.status_message = "CPU halted";
        }
      } catch (const NoInputException &e) {
        // No input available, just continue
        state.status_message = std::string("Waiting on input...");
        state.running = false;
      } catch (const std::exception &e) {
        state.status_message = std::string("Error: ") + e.what();
        state.running = false;
      }
      screen.PostEvent(ftxui::Event::Custom);
    }

    // Update UI
    loop.RunOnce();
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50)); // Prevent High CPU Usage.
  }

  return 0;
}
