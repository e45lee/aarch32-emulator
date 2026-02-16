/**
    main.cpp

    Entry point for the AArch32 emulator with FTXUI interface.
*/

#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <thread>
#include <chrono>

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

    // Load test program
    loadTestProgram(state);

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
                }
            } catch (const std::exception& e) {
                state.status_message = std::string("Error: ") + e.what();
                state.running = false;
            }
        }

        // Update UI
        loop.RunOnce();
    }

    return 0;
}
