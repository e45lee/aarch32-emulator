target_sources(aarch32-runtime PRIVATE 
  memory.cpp
  mmio.cpp
  decode_instruction.cpp
  cpu_ee.cpp
  cpu.cpp
)

target_sources(aarch32-emulator-ui PRIVATE
  ui-main.cpp
  emulator_ui.cpp
  ui_renderer.cpp
)

target_sources(aarch32-cli PRIVATE
  cli-main.cpp
)
