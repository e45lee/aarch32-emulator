/**
    mmio.h

    Headers for the MMIO subsystem of the AArch32 emulator.
    This includes:
        - a basic MMIO class that provides read/write access to a byte-addressable memory space.
            that can be overriden with custom read/write handlers for specific address ranges (e.g., for memory-mapped I/O devices).
 */
 #include "memory.h"
 #include <map>
 #include <functional>

class MemoryMappedIO : public Memory {
public:
    using ReadHandler = std::function<uint8_t(uint32_t)>;
    using WriteHandler = std::function<void(uint32_t, uint8_t)>;
private:
    std::map<uint32_t, ReadHandler> read_handlers;
    std::map<uint32_t, WriteHandler> write_handlers;
public:
    MemoryMappedIO(uint32_t lower_end = 0x0100000, uint32_t upper_start = 0xFF000000);

    void setReadHandler(uint32_t address, ReadHandler handler);
    void setWriteHandler(uint32_t address, WriteHandler handler);

    virtual uint8_t readByte(uint32_t address) override;
    virtual void writeByte(uint32_t address, uint8_t value) override;
 };