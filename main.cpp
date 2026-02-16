/**
    main.cpp

    Entry point for the AArch32 emulator with FTXUI interface.
*/

#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

#include "cpu.h"
#include "mmio.h"
#include "emulator_ui.h"
#include "ui_renderer.h"

using namespace ftxui;

int main(int argc, char* argv[]) {
    // Create emulator state
    EmulatorState state;
    state.memory = std::make_shared<MemoryMappedIO>();
    state.cpu = std::make_shared<CPU>(state.memory.get(), 0x00000000);

    // Set up console I/O handlers
    setupConsoleIO(state);

    // Parse command line arguments
    std::string binary_filename = "";
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
                        uint32_t value = std::stoul(value_str, nullptr, 0); // Auto-detect base (0x for hex, etc.)
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
        } else if (arg[0] != '-') {
            // Positional argument - binary filename
            if (binary_filename.empty()) {
                binary_filename = arg;
            }
        }
    }

    // Load program from file or use test program
    if (!binary_filename.empty()) {
        // Load binary file from command line argument
        if (!loadBinaryFile(state, binary_filename, 0x00000000)) {
            std::cerr << "Failed to load binary file: " << binary_filename << std::endl;
            std::cerr << "Usage: " << argv[0] << " [binary_file] [--r0 VALUE] ... [--r12 VALUE]" << std::endl;
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
                state.cpu->step();
                if (state.cpu->isHalted()) {
                    state.running = false;
                    state.status_message = "CPU halted";
                    screen.PostEvent(ftxui::Event::Custom);                
                }
            } catch (const std::exception& e) {
                state.status_message = std::string("Error: ") + e.what();
                state.running = false;
                screen.PostEvent(ftxui::Event::Custom);                
            }
        }

        // Update UI
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Prevent High CPU Usage.
    }

    return 0;
}
