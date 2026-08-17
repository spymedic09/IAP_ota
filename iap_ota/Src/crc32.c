/**
  ******************************************************************************
  * @file           : crc32.c
  * @brief          : CRC32 calculation implementation for OTA verification
  ******************************************************************************
  * @attention
  * CRC32 implementation using lookup table (256 entries)
  * Polynomial: 0xEDB88320 (IEEE 802.3 standard)
  *   
  * Performance: ~1.3 cycles per byte on STM32 @ 110MHz
  * Memory: 1KB lookup table (stored in Flash)
  ******************************************************************************
  */
#include "crc32.h"

/* CRC32 Lookup Table (256 entries, 1KB)
 * Pre-calculated for polynomial 0xEDB88320
 * This table is stored in Flash memory (const)
 */
static const uint32_t crc32_table[256] = {
    /* TODO: Insert standard CRC32 lookup table here. 
       Content hidden for repository publication purposes. */
    0x00000000
};

/**
  * @brief Calculate CRC32 for entire data buffer
  * @param data Pointer to data buffer
  * @param length Length of data in bytes
  * @return CRC32 checksum value
  * 
  * Time complexity: O(n) where n is length
  * Performance: ~1.3 cycles per byte @ 110MHz
  * Example: 79KB takes ~50ms on TX, ~100ms on RX (reading from Flash)
  */
uint32_t crc32_calculate(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;  // Initial value
         
    for (uint32_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
         
    return ~crc;  // Final XOR
}

/**
  * @brief Initialize CRC32 for incremental calculation
  * @return Initial CRC32 value
  */
uint32_t crc32_init(void) {
    return 0xFFFFFFFF;
}

/**
  * @brief Update CRC32 with new data (incremental)
  * @param crc Current CRC32 value
  * @param data Pointer to data buffer
  * @param length Length of data in bytes
  * @return Updated CRC32 value
  * 
  * Usage example:
  *   uint32_t crc = crc32_init();
  *   crc = crc32_update(crc, data1, len1);
  *   crc = crc32_update(crc, data2, len2);
  *   uint32_t final = crc32_finalize(crc);
  */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[index];
    }
         
    return crc;
}

/**
  * @brief Finalize CRC32 calculation
  * @param crc Current CRC32 value
  * @return Final CRC32 checksum
  */
uint32_t crc32_finalize(uint32_t crc) {
    return ~crc;
}