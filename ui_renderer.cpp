/**
    ui_renderer.cpp

    FTXUI rendering components for the emulator UI.
    Implements a 4-pane layout with disassembly, register state, stack view,
    console output, and a console input line.
*/

#include "ui_renderer.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <iomanip>

using namespace ftxui;

/**
 * Create the main UI renderer component
 */
Component createUIRenderer(EmulatorState& state) {
    // Console input component
    auto console_input = Input(&state.console_current_input, "Type here...");

    // Wrap console_input to handle Enter key
    auto console_input_wrapped = CatchEvent(console_input, [&state](Event event) {
        if (event == Event::Return) {
            // Add newline and all characters to the console input queue
            for (char c : state.console_current_input) {
                state.console_input.push_back(c);
            }
            state.console_input.push_back('\n');
            state.console_current_input.clear();
            return true;
        }
        return false;
    });

    // Memory address input component
    auto memory_addr_input = Input(&state.memory_address_input, "0x00000000");

    // Wrap memory address input to handle Enter key
    auto memory_addr_wrapped = CatchEvent(memory_addr_input, [&state](Event event) {
        if (event == Event::Return) {
            try {
                // Parse the hex address
                state.memory_view_address = std::stoul(state.memory_address_input, nullptr, 16);
                state.memory_view_address &= ~0xF; // Align to 16 bytes
                state.status_message = "Memory address updated";
            } catch (...) {
                state.status_message = "Invalid memory address";
            }
            return true;
        }
        return false;
    });

    // Create a container that switches between inputs based on active tab
    auto input_container = Container::Tab(
        {
            console_input_wrapped,
            memory_addr_wrapped
        },
        &state.current_tab
    );

    // Create a renderer that displays all panes
    auto renderer = Renderer(input_container, [&state, console_input_wrapped, memory_addr_wrapped] {
        // Get data for each pane
        auto disassembly = getDisassembly(state, 8, 8);
        auto registers = getRegisterView(state);
        auto stack = getStackView(state, 12);

        // Build disassembly pane
        Elements disasm_elements;
        disasm_elements.push_back(text("Disassembly") | bold | center);
        disasm_elements.push_back(separator());
        for (const auto& line : disassembly) {
            if (line.substr(0, 3) == ">>>") {
                disasm_elements.push_back(text(line) | color(Color::Green) | bold);
            } else {
                disasm_elements.push_back(text(line));
            }
        }
        auto disasm_pane = window(text("Disassembly"),
                                  vbox(std::move(disasm_elements)) | vscroll_indicator | frame);

        // Build register pane with two columns
        Elements reg_elements;
        reg_elements.push_back(text("Registers") | bold | center);
        reg_elements.push_back(separator());

        // Display R0-R12 in two columns (7 per column, then 6)
        for (int i = 0; i < 7; i++) {
            auto left_reg = text(registers[i]);
            auto right_reg = i + 7 < 13 ? text(registers[i + 7]) : text("");
            reg_elements.push_back(hbox({
                left_reg | size(WIDTH, EQUAL, 18),
                text(" | "),
                right_reg
            }));
        }

        reg_elements.push_back(separator());

        // Display special registers and flags
        for (size_t i = 13; i < registers.size(); i++) {
            if (registers[i].substr(0, 2) == "PC" ||
                registers[i].substr(0, 2) == "Fl" ||
                registers[i].substr(0, 2) == "CP") {
                reg_elements.push_back(text(registers[i]) | color(Color::Cyan));
            } else {
                reg_elements.push_back(text(registers[i]));
            }
        }

        auto reg_pane = window(text("Registers"),
                               vbox(std::move(reg_elements)) | frame);

        // Build stack pane
        Elements stack_elements;
        stack_elements.push_back(text("Stack") | bold | center);
        stack_elements.push_back(separator());
        for (const auto& line : stack) {
            if (line.substr(0, 3) == "SP>") {
                stack_elements.push_back(text(line) | color(Color::Yellow) | bold);
            } else {
                stack_elements.push_back(text(line));
            }
        }
        auto stack_pane = window(text("Stack"),
                                 vbox(std::move(stack_elements)) | vscroll_indicator | frame);

        // Build console output pane
        Elements console_elements;
        console_elements.push_back(text("Console Output") | bold | center);
        console_elements.push_back(separator());
        if (state.console_output.empty()) {
            console_elements.push_back(text("<no output>") | dim);
        } else {
            for (const auto& line : state.console_output) {
                console_elements.push_back(text(line));
            }
        }
        auto console_pane = window(text("Console"),
                                   vbox(std::move(console_elements)) | vscroll_indicator | frame);

        // Build console input area
        auto console_input_box = hbox({
            text("Input: ") | bold,
            console_input_wrapped->Render()
        });

        // Build memory view pane
        auto memory_view = getMemoryView(state, state.memory_view_address, 128, 128);
        Elements memory_elements;
        memory_elements.push_back(text("Memory View") | bold | center);
        memory_elements.push_back(separator());
        for (const auto& line : memory_view) {
            if (line.substr(0, 3) == ">>>") {
                memory_elements.push_back(text(line) | color(Color::Magenta) | bold);
            } else if (line.substr(0, 6) == "Memory") {
                memory_elements.push_back(text(line) | bold);
            } else {
                memory_elements.push_back(text(line));
            }
        }
        auto memory_pane = window(text("Memory"),
                                  vbox(std::move(memory_elements)) | vscroll_indicator | frame);

        // Build memory address input area
        auto memory_addr_box = hbox({
            text("Address: ") | bold,
            memory_addr_wrapped->Render() | border,
            text("  ") | dim,
            text("(Press Enter to jump) | PgUp/PgDn: ±256 bytes | Up/Down: ±16 bytes") | dim
        });

        // Tab bar
        auto tab_bar = hbox({
            text(state.current_tab == 0 ? " [CPU View] " : "  CPU View  ") |
                (state.current_tab == 0 ? bgcolor(Color::Blue) : bgcolor(Color::GrayDark)) | bold,
            text(" | "),
            text(state.current_tab == 1 ? " [Memory View] " : "  Memory View  ") |
                (state.current_tab == 1 ? bgcolor(Color::Blue) : bgcolor(Color::GrayDark)) | bold,
            text(" ") | flex,
            text("F1: CPU | F2: Memory") | dim
        }) | border;

        Element main_content;

        if (state.current_tab == 0) {
            // CPU View (original layout)
            auto top_row = hbox({
                vbox({
                    reg_pane | size(HEIGHT, LESS_THAN, 20) | size(WIDTH, LESS_THAN, 40),
                    stack_pane | size(WIDTH, LESS_THAN, 40) | flex
                }),
                separator(),
                vbox({
                    disasm_pane | flex,
                    console_pane | size(HEIGHT, GREATER_THAN, 10) | flex,
                }) | flex
            });

            auto bottom_area = vbox({
                separator(),
                console_input_box
            });

            main_content = vbox({
                top_row | flex,
                separator(),
                bottom_area
            });
        } else {
            // Memory View
            main_content = vbox({
                memory_pane | flex,
                separator(),
                memory_addr_box
            });
        }

        // Status bar
        auto status_bar = hbox({
            text(state.running ? " RUNNING " : " STOPPED ") |
                (state.running ? bgcolor(Color::Green) : bgcolor(Color::Red)) | bold,
            separator(),
            text(" " + state.status_message + " ") | flex,
            separator(),
            text(" F5: Run/Stop | F6: Step | F7: Reset | F12: Quit ") | dim
        }) | border;

        // Combine everything
        return vbox({
            tab_bar,
            separator(),
            main_content | flex,
            separator(),
            status_bar
        });
    });

    return renderer;
}

/**
 * Create the event handler component for keyboard controls
 */
Component createEventHandler(Component base, EmulatorState& state, ScreenInteractive& screen) {
    return CatchEvent(base, [&state, &screen](Event event) {
        // F1: Switch to CPU view
        if (event == Event::F1) {
            state.current_tab = 0;
            state.status_message = "Switched to CPU view";
            return true;
        }

        // F2: Switch to Memory view
        if (event == Event::F2) {
            state.current_tab = 1;
            state.status_message = "Switched to Memory view";
            return true;
        }

        // Memory navigation (only in memory view)
        if (state.current_tab == 1) {
            if (event == Event::PageDown) {
                state.memory_view_address += 256;
                std::ostringstream oss;
                oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << state.memory_view_address;
                state.memory_address_input = oss.str();
                return true;
            }
            if (event == Event::PageUp) {
                state.memory_view_address -= 256;
                std::ostringstream oss;
                oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << state.memory_view_address;
                state.memory_address_input = oss.str();
                return true;
            }
            if (event == Event::ArrowDown) {
                state.memory_view_address += 16;
                std::ostringstream oss;
                oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << state.memory_view_address;
                state.memory_address_input = oss.str();
                return true;
            }
            if (event == Event::ArrowUp) {
                state.memory_view_address -= 16;
                std::ostringstream oss;
                oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << state.memory_view_address;
                state.memory_address_input = oss.str();
                return true;
            }
        }

        // F5: Toggle run/stop
        if (event == Event::F5) {
            state.running = !state.running;
            state.status_message = state.running ? "Running" : "Stopped";
            return true;
        }

        // F6: Step one instruction
        if (event == Event::F6) {
            if (!state.cpu->isHalted()) {
                try {
                    state.cpu->step();
                    state.status_message = "Stepped one instruction";
                } catch (const std::exception& e) {
                    state.status_message = std::string("Error: ") + e.what();
                }
            } else {
                state.status_message = "CPU is halted";
            }
            return true;
        }

        // F7: Reset CPU
        if (event == Event::F7) {
            state.cpu = std::make_shared<CPU>(state.memory.get(), 0x00000000);
            state.running = false;
            state.console_output.clear();
            state.console_input.clear();
            state.console_current_input.clear();

            // Reload program (from file if available, otherwise test program)
            if (!state.loaded_filename.empty()) {
                if (loadBinaryFile(state, state.loaded_filename, 0x00000000)) {
                    state.status_message = "CPU reset - reloaded: " + state.loaded_filename;
                } else {
                    state.status_message = "CPU reset - failed to reload file";
                }
            } else {
                loadTestProgram(state);
                state.status_message = "CPU reset";
            }

            // Reapply initial register values
            applyInitialRegisters(state);
            return true;
        }

        // F12: Quit
        if (event == Event::F12) {
            screen.Exit();
            return true;
        }

        return false;
    });
}
