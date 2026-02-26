/**
    mmio.h

    Headers for the MMIO subsystem of the AArch32 emulator.
    This includes:
        - a basic MMIO class that provides read/write access to a
   byte-addressable memory space. that can be overriden with custom read/write
   handlers for specific address ranges (e.g., for memory-mapped I/O devices).
 */

#ifndef MMIO_H
#define MMIO_H

#include "Memory.hpp"
#include <functional>
#include <map>

class MemoryMappedIO : public Memory {
public:
  using ReadHandlerB = std::function<uint8_t(uint32_t)>;
  using WriteHandlerB = std::function<void(uint32_t, uint8_t)>;
  using ReadHandlerW = std::function<uint32_t(uint32_t)>;
  using WriteHandlerW = std::function<void(uint32_t, uint32_t)>;

private:
  std::map<uint32_t, ReadHandlerB> read_handlersB;
  std::map<uint32_t, WriteHandlerB> write_handlersB;
  std::map<uint32_t, ReadHandlerW> read_handlersW;
  std::map<uint32_t, WriteHandlerW> write_handlersW;

public:
  MemoryMappedIO(uint32_t lower_end = 0x1000000,
                 uint32_t upper_start = 0xFF000000);

  void setReadHandlerB(uint32_t address, ReadHandlerB handler);
  void setWriteHandlerB(uint32_t address, WriteHandlerB handler);
  void setReadHandlerW(uint32_t address, ReadHandlerW handler);
  void setWriteHandlerW(uint32_t address, WriteHandlerW handler);

  virtual uint8_t readByte(uint32_t address) override;
  virtual void writeByte(uint32_t address, uint8_t value) override;

  virtual void writeWord(uint32_t address, uint32_t value) override;
  virtual uint32_t readWord(uint32_t address) override;
};

#endif // MMIO_H
