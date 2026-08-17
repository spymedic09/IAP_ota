/**
  ******************************************************************************
  * @file           : crc32.h
  * @brief          : CRC32 calculation header for OTA firmware verification
  ******************************************************************************
  * @attention
  *
  * CRC32 implementation using lookup table for fast calculation
  * Used for OTA firmware integrity verification
  *
  ******************************************************************************
  */

#ifndef __CRC32_H
#define __CRC32_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Calculate CRC32 checksum using lookup table
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return CRC32 checksum value
 */
uint32_t crc32_calculate(const uint8_t *data, uint32_t length);

/**
 * @brief Initialize CRC32 calculation (optional, for incremental calculation)
 * @return Initial CRC32 value
 */
uint32_t crc32_init(void);

/**
 * @brief Update CRC32 with new data (for incremental calculation)
 * @param crc Current CRC32 value
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return Updated CRC32 value
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t length);

/**
 * @brief Finalize CRC32 calculation
 * @param crc Current CRC32 value
 * @return Final CRC32 checksum
 */
uint32_t crc32_finalize(uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif /* __CRC32_H */
