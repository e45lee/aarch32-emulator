/**
    mmio.cpp

    Tests for the MMIO subsystem of the AArch32 emulator.
 */
#include "../mmio.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MMIO read/write with handlers", "[mmio]") {
    MemoryMappedIO mmio;
    mmio.setReadHandler(0xFF000000, [](uint32_t addr) { return 0x42; });
    mmio.setWriteHandler(0xFF000001, [](uint32_t addr, uint8_t value) { REQUIRE(value == 0x99); });

    REQUIRE(mmio.readByte(0xFF000000) == 0x42);
    mmio.writeByte(0xFF000001, 0x99);
}