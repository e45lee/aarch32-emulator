/**
    Tests for the memory subsystem of the AArch32 emulator. This includes:
        - a basic Memory class that provides read/write access to a byte-addressable memory space.
 */

 #include "../memory.h"
 #include <catch2/catch_test_macros.hpp>

TEST_CASE("Memory read/write lower half", "[memory]") {
    Memory mem;
    mem.writeByte(0x00000000, 0xAA);
    mem.writeByte(0x000FFFFF, 0xBB);
    REQUIRE(mem.readByte(0x00000000) == 0xAA);
    REQUIRE(mem.readByte(0x000FFFFF) == 0xBB);
}

TEST_CASE("Memory read/write upper half", "[memory]") {
    Memory mem;
    mem.writeByte(0xFF000000, 0xCC);
    mem.writeByte(0xFFFFFFFF, 0xDD);
    REQUIRE(mem.readByte(0xFF000000) == 0xCC);
    REQUIRE(mem.readByte(0xFFFFFFFF) == 0xDD);
}

TEST_CASE("Memory out of bounds access", "[memory]") {
    Memory mem;
    REQUIRE_THROWS_AS(mem.readByte(0x10000000), std::out_of_range);
    REQUIRE_THROWS_AS(mem.writeByte(0x10000000, 0xEE), std::out_of_range);
}