/**
    ui_renderer.cpp

    FTXUI rendering implementation for the emulator UI.
*/

#include "ui_renderer.h"
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/event.hpp>
#include <sstream>

using namespace ftxui;

// Create an interactive console component
Component createConsoleComponent(EmulatorState& state) {
    auto component = Renderer([&state] {
        Elements console_elements;

        // Display output
        if (state.console_output.empty()) {
            console_elements.push_back(text("<no output>") | dim);
        } else {
            for (const auto& line : state.console_output) {
                console_elements.push_back(text(line));
            }
        }

        // Add current input line when focused
        if (state.console_focused) {
            std::string input_line = "> " + state.console_current_input + "_";
            console_elements.push_back(text(input_line) | color(Color::Green) | bold);
        }

        return window(text("Console (MMIO)") | bold,
                     vbox(console_elements) | frame | size(HEIGHT, GREATER_THAN, 5));
    });

    // Add event handler - only process events when focused
    component = CatchEvent(component, [&state](Event event) {
        if (!state.console_focused) {
            return false;
        }

        // Handle character input
        if (event.is_character()) {
            char c = event.character()[0];
            state.console_current_input += c;
            state.console_input.push_back(c);
            return true;
        }

        // Handle Enter key
        if (event == Event::Return) {
            state.console_current_input += '\n';
            state.console_input.push_back('\n');
            state.console_current_input.clear();
            return true;
        }

        // Handle Backspace
        if (event == Event::Backspace) {
            if (!state.console_current_input.empty()) {
                state.console_current_input.pop_back();
                if (!state.console_input.empty()) {
                    state.console_input.pop_back();
                }
            }
            return true;
        }

        return false;
    });

    return component;
}

Component createUIRenderer(EmulatorState& state) {
    // Create interactive console component
    auto console_component = createConsoleComponent(state);

    // Create a dummy component for the main UI (non-focusable)
    auto main_ui = Renderer([&state] {
        // Get current state
        auto disasm = getDisassembly(state);
        auto registers = getRegisterView(state);
        auto stack = getStackView(state);

        // Build disassembly pane
        Elements disasm_elements;
        for (const auto& line : disasm) {
            if (line.find(">>>") != std::string::npos) {
                disasm_elements.push_back(text(line) | color(Color::Yellow) | bold);
            } else {
                disasm_elements.push_back(text(line));
            }
        }
        auto disasm_pane = window(text("Disassembly") | bold,
                                 vbox(disasm_elements) | frame | size(HEIGHT, GREATER_THAN, 10));

        // Build register pane
        Elements reg_elements;
        for (const auto& line : registers) {
            reg_elements.push_back(text(line));
        }
        auto reg_pane = window(text("Registers") | bold,
                              vbox(reg_elements) | frame);

        // Build stack pane
        Elements stack_elements;
        for (const auto& line : stack) {
            if (line.find("SP>") != std::string::npos) {
                stack_elements.push_back(text(line) | color(Color::Cyan) | bold);
            } else {
                stack_elements.push_back(text(line));
            }
        }
        auto stack_pane = window(text("Stack") | bold,
                                vbox(stack_elements) | frame | size(HEIGHT, GREATER_THAN, 10));

        // Layout: top row with disassembly and registers, bottom row with stack
        auto top_row = hbox({
            disasm_pane | flex,
            separator(),
            vbox({
                reg_pane,
                separator(),
                stack_pane
            }) | size(WIDTH, EQUAL, 25)
        });

        return top_row;
    });

    // Create container with main UI and console
    auto container = Container::Vertical({
        main_ui,
        console_component
    });

    // Wrap in renderer to add status bar
    auto full_ui = Renderer(container, [&state, main_ui, console_component] {
        // Build status bar
        std::ostringstream status_oss;
        status_oss << "Status: " << state.status_message;
        if (state.cpu->isHalted()) {
            status_oss << " [HALTED]";
        } else if (state.running) {
            status_oss << " [RUNNING]";
        } else {
            status_oss << " [PAUSED]";
        }

        std::string help_text;
        if (state.console_focused) {
            help_text = " Console focused - Type to input, [Esc] to exit ";
        } else {
            help_text = " [S]tep | [C]ontinue | [P]ause | [R]eset | [Q]uit | [/] Console ";
        }

        auto status_bar = hbox({
            text(status_oss.str()) | bold,
            separator(),
            text(help_text) | dim
        });

        return vbox({
            main_ui->Render() | flex,
            separator(),
            console_component->Render() | size(HEIGHT, EQUAL, 8),
            separator(),
            status_bar
        });
    });

    return full_ui;
}

Component createEventHandler(Component base, EmulatorState& state, ScreenInteractive& screen) {
    return CatchEvent(base, [&state, &screen](Event event) {
        // Handle Escape to exit console
        if (event == Event::Escape && state.console_focused) {
            state.console_focused = false;
            state.status_message = "Console unfocused";
            return true;
        }

        // Handle / key to enter console
        if (event == Event::Character('/') && !state.console_focused) {
            state.console_focused = true;
            state.status_message = "Console focused";
            return true;
        }

        // If console is focused, let it handle all other events
        if (state.console_focused) {
            return false;
        }

        // Global shortcuts (only when console not focused)
        if (event == Event::Character('q') || event == Event::Character('Q')) {
            screen.Exit();
            return true;
        } else if (event == Event::Character('s') || event == Event::Character('S')) {
            // Step one instruction
            if (!state.cpu->isHalted()) {
                try {
                    state.cpu->step();
                    state.status_message = "Stepped";
                } catch (const std::exception& e) {
                    state.status_message = std::string("Error: ") + e.what();
                }
            } else {
                state.status_message = "CPU is halted";
            }
            return true;
        } else if (event == Event::Character('c') || event == Event::Character('C')) {
            // Continue execution
            state.running = true;
            state.status_message = "Running";
            return true;
        } else if (event == Event::Character('p') || event == Event::Character('P')) {
            // Pause execution
            state.running = false;
            state.status_message = "Paused";
            return true;
        } else if (event == Event::Character('r') || event == Event::Character('R')) {
            // Reset CPU
            state.cpu = std::make_shared<CPU>(state.memory.get(), 0x00000000);
            state.running = false;
            state.console_output.clear();
            state.console_input.clear();
            state.console_current_input.clear();
            state.status_message = "Reset";
            return true;
        }

        return false;
    });
}
