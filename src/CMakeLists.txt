target_sources(aarch32-runtime PRIVATE 
  Memory.cpp
  MemoryMappedIO.cpp
  InstructionDecoding.cpp
  ExecutionEngine.cpp
  CPU.cpp
)

target_sources(aarch32-emulator-ui PRIVATE
  ui-main.cpp
  emulator_ui.cpp
  ui_renderer.cpp
)

target_sources(aarch32-cli PRIVATE
  cli-main.cpp
)

target_sources(aarch32-disasm PRIVATE
  disasm-main.cpp
)
