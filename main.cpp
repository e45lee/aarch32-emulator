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

    // Load program from file or use test program
    if (argc >= 2) {
        // Load binary file from command line argument
        std::string filename = argv[1];
        if (!loadBinaryFile(state, filename, 0x00000000)) {
            std::cerr << "Failed to load binary file: " << filename << std::endl;
            std::cerr << "Usage: " << argv[0] << " [binary_file]" << std::endl;
            return 1;
        }
        state.loaded_filename = filename;
        state.status_message = "Loaded: " + filename;
    } else {
        // No file provided, load test program
        loadTestProgram(state);
        state.loaded_filename = ""; // Empty string indicates test program
        state.status_message = "Test program loaded";
    }

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
