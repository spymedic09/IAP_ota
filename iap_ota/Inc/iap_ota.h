#ifndef IAP_OTA_H
#define IAP_OTA_H

#include "stm32l5xx_hal.h"

/* Inactive Bank Address (Hardware Swap Target) */
#define INACTIVE_BANK_ADDR  0x08040000
#define PAGES_PER_BANK      128    // 256KB / 2KB

extern uint8_t ota_in_progress;

void OTA_Process_Background(void);

#endif /* IAP_OTA_H */