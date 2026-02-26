/**
    ui_renderer.h

    FTXUI rendering components for the emulator UI.
*/

#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "emulator_ui.h"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

// Forward declaration
namespace ftxui {
class ScreenInteractive;
}

/**
 * Create the main UI renderer component
 */
ftxui::Component createUIRenderer(EmulatorState &state);

/**
 * Create the event handler component for keyboard controls
 */
ftxui::Component createEventHandler(ftxui::Component base, EmulatorState &state,
                                    ftxui::ScreenInteractive &screen);

#endif // UI_RENDERER_H
