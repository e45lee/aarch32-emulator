cmake_minimum_required(VERSION 3.22)
project(teaching-emulator)
include(FetchContent)

# Dependencies -- FTXUI
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9  # Replace with a version, tag, or commit hash
)
FetchContent_MakeAvailable(ftxui)

# Dependencies -- Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# Enable tests
include(CTest)
include(Catch)

# Set C++ version
set(CMAKE_CXX_STANDARD 20)
set(SOURCES memory.cpp mmio.cpp decode_instruction.cpp cpu_ee.cpp cpu.cpp)
set(UI_SOURCES emulator_ui.cpp ui_renderer.cpp)
set(TESTS tests/memory.cpp tests/mmio.cpp tests/decode_instruction.cpp tests/cpu_ee.cpp tests/cpu.cpp)

# Build the main executable
add_executable(aarch32-emulator main.cpp ${SOURCES} ${UI_SOURCES})
target_link_libraries(aarch32-emulator
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

# And the tests
add_executable(aarch32-emulator-tests ${SOURCES} ${TESTS})
target_link_libraries(aarch32-emulator-tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(aarch32-emulator-tests)