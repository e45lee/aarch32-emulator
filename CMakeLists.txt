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

# targets
add_library(aarch32-runtime)
add_executable(aarch32-emulator-ui)
add_executable(aarch32-cli)
add_executable(aarch32-disasm)

set(TESTS tests/memory.cpp tests/mmio.cpp tests/decode_instruction.cpp tests/cpu_ee.cpp tests/cpu.cpp)

# test targets 
add_executable(aarch32-emulator-tests ${TESTS})

add_subdirectory(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/src)


# Build the main executable
target_link_libraries(aarch32-emulator-ui
  PRIVATE aarch32-runtime
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

# Build the CLI executable (no UI, uses stdin/stdout)
target_link_libraries(aarch32-cli
  PRIVATE aarch32-runtime
)

# Build the disassembler
target_link_libraries(aarch32-disasm
  PRIVATE aarch32-runtime
)

# And the tests
target_link_libraries(aarch32-emulator-tests 
  PRIVATE aarch32-runtime
  PRIVATE Catch2::Catch2WithMain
)

catch_discover_tests(aarch32-emulator-tests)
