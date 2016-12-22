#ifndef CRC32_h
#define CRC32_h

#include <stdint.h>
#include <stddef.h>

inline uint32_t crc32(const void* data, size_t length, const uint32_t previousHash = 0)
{
  uint32_t crc = ~previousHash;
  unsigned char* current = (unsigned char*) data;
  static uint32_t lut[16] = {
    0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
    0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C
  };

  while (length--) {
    crc = lut[(crc ^  *current      ) & 0x0F] ^ (crc >> 4);
    crc = lut[(crc ^ (*current >> 4)) & 0x0F] ^ (crc >> 4);
    current++;
  }
  return ~crc;
}

#endif /* CRC32_h */
