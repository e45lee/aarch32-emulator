/**
    mmio.cpp

    Tests for the MMIO subsystem of the AArch32 emulator.
 */
#include "../mmio.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MMIO read/write with handlers", "[mmio]") {
    MemoryMappedIO mmio;
    mmio.setReadHandlerB(0xFF000000, [](uint32_t addr) { return 0x42; });
    mmio.setWriteHandlerB(0xFF000001, [](uint32_t addr, uint8_t value) { REQUIRE(value == 0x99); });

    REQUIRE(mmio.readByte(0xFF000000) == 0x42);
    mmio.writeByte(0xFF000001, 0x99);
}

TEST_CASE("MMIO read/write handlers for words", "[mmio]") {
    MemoryMappedIO mmio;
    mmio.setReadHandlerW(0xFF000004, [](uint32_t addr) { return 0xDEADBEEF; });
    mmio.setWriteHandlerW(0xFF000008, [](uint32_t addr, uint32_t value) { REQUIRE(value == 0xCAFEBABE); });

    REQUIRE(mmio.readWord(0xFF000004) == 0xDEADBEEF);
    mmio.writeWord(0xFF000008, 0xCAFEBABE);
}