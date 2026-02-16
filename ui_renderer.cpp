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

    // Create a renderer that displays all panes
    auto renderer = Renderer(console_input_wrapped, [&state, console_input_wrapped] {
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

        // Layout: 2x2 grid for the 4 panes, plus console input at bottom
        auto top_row = hbox({
            vbox({
                reg_pane | size(HEIGHT, LESS_THAN, 20) | size(WIDTH, LESS_THAN, 40),
                stack_pane | size(WIDTH, LESS_THAN, 40) | flex
            }),
            separator(),
            disasm_pane | flex
        });

        auto bottom_area = vbox({
            console_pane | size(HEIGHT, LESS_THAN, 12),
            separator(),
            console_input_box
        });

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
            top_row | flex,
            separator(),
            bottom_area,
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
            loadTestProgram(state);
            state.status_message = "CPU reset";
            return true;
        }

        // Q: Quit
        if (event == Event::F12) {
            screen.Exit();
            return true;
        }

        return false;
    });
}
