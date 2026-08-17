#include "iap_ota.h"
#include "ntag5.h"
#include "crc32.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

extern I2C_HandleTypeDef hi2c1;
uint8_t ota_in_progress = 0;

void OTA_ReInit_I2C1(void);

// =========================================================================
// 1. Erase Inactive Bank 
// =========================================================================
static HAL_StatusTypeDef Erase_OTA_Inactive_Bank(void) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    FLASH_OBProgramInitTypeDef OBInit;
    HAL_StatusTypeDef status = HAL_OK;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    HAL_FLASHEx_OBGetConfig(&OBInit);
         
    // Determine the current inactive bank
    if ((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) != 0) {
        EraseInitStruct.Banks = FLASH_BANK_1; 
    } else {
        EraseInitStruct.Banks = FLASH_BANK_2;
    }

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page      = 0;
    EraseInitStruct.NbPages   = 1;

    for (uint32_t i = 0; i < PAGES_PER_BANK; i++) {
        EraseInitStruct.Page = i;
                 
        __disable_irq();
        HAL_ICACHE_Disable();
                 
        status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
                 
        HAL_ICACHE_Enable();
        __enable_irq();

        if (status != HAL_OK) {
            printf("[OTA] Erase Error at Page: %lu\r\n", PageError);
            break;
        }
    }

    HAL_FLASH_Lock();
    return status;
}

// =========================================================================
// 2. Hardware Swap 
// =========================================================================
static void Execute_Hardware_Swap_And_Reset(void) {
    FLASH_OBProgramInitTypeDef OBInit;

    printf("\r\n[OTA] Executing Hardware Swap...\r\n");
    HAL_Delay(100);

    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBGetConfig(&OBInit);

    // Toggle SWAP_BANK option
    OBInit.USERConfig ^= OB_SWAP_BANK_ENABLE; 
    OBInit.OptionType = OPTIONBYTE_USER;
    OBInit.USERType   = OB_USER_SWAP_BANK;

    if (HAL_FLASHEx_OBProgram(&OBInit) == HAL_OK) {
        printf("[OTA] Swap programmed. Launching Reset!\r\n");
        HAL_Delay(100);
        // Launch Option Byte and trigger System Reset
        HAL_FLASH_OB_Launch(); 
    }

    // Lock and return if launch fails
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    NVIC_SystemReset();
}

// =========================================================================
// 3. OTA State Machine
// =========================================================================
void OTA_Process_Background(void) {
    static uint32_t flash_write_addr = INACTIVE_BANK_ADDR;
    static uint32_t last_activity_tick = 0;
    uint8_t header[4] = {0};
    uint8_t ack_msg[4] = {0x02, 0x00, 0x00, 0x00};
    uint8_t timeout_msg[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    if (HAL_I2C_Mem_Read(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, header, 4, 50) == HAL_OK)
    {
        // --- Timeout Check ---
        if (ota_in_progress && (HAL_GetTick() - last_activity_tick > 30000)) {
            printf("\r\n[OTA] ERROR: Timeout! Aborting...\r\n");
            HAL_I2C_Mem_Write(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, timeout_msg, 4, 100);
            ota_in_progress = 0;
            HAL_GPIO_WritePin(LED_Bootloader_GPIO_Port, LED_Bootloader_Pin, GPIO_PIN_RESET);
            return;
        }

        // --- Start OTA Command (AA 55 00 00) ---
        if (header[0] == 0xAA && header[1] == 0x55)
        {
            printf("\r\n[OTA] Received START Command. Erasing Inactive Bank...\r\n");
            ota_in_progress = 1;
            HAL_GPIO_WritePin(LED_Bootloader_GPIO_Port, LED_Bootloader_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOC, LED_GREEN_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOC, LED_RED_Pin, GPIO_PIN_RESET);

            last_activity_tick = HAL_GetTick();
            flash_write_addr = INACTIVE_BANK_ADDR;

            // Erase target memory bank
            Erase_OTA_Inactive_Bank();
                         
            // Send ACK via I2C
            uint8_t verify_buf[4] = {0};
            uint8_t retry = 0;
            while (verify_buf[0] != 0x02 && retry < 10) {
                HAL_I2C_Mem_Write(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, ack_msg, 4, 50);
                HAL_Delay(5);
                HAL_I2C_Mem_Read(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, verify_buf, 4, 50);
                retry++;
            }
            printf("[OTA] ACK Sent. Ready for Data.\r\n");
        }
                 
        else if (header[0] == 0x00)
        {
            if (ota_in_progress) {
                // Wait for RF data transmission
                HAL_Delay(10);
            } else {
                // Idle state
                HAL_Delay(50);
            }
        }
                 
        // --- Receive Data State (0x01) ---
        else if (header[0] == 0x01 && ota_in_progress)
        {
            last_activity_tick = HAL_GetTick();
            uint8_t actual_data_len = (header[1] == 0 || header[1] > 240) ? 240 : header[1];
            uint8_t full_buffer[244];
                         
            // Read incoming data block
            if (HAL_I2C_Mem_Read(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, full_buffer, actual_data_len + 4, 100) == HAL_OK)
            {
                HAL_FLASH_Unlock();
                __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
                for (int i = 0; i < actual_data_len; i += 8) {
                    uint64_t double_word = 0xFFFFFFFFFFFFFFFF;
                    uint32_t remaining = actual_data_len - i;
                    memcpy(&double_word, &full_buffer[i + 4], (remaining >= 8) ? 8 : remaining);
                                         
                    // Write data to Flash memory
                    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_write_addr, double_word);
                    flash_write_addr += 8;
                }
                HAL_FLASH_Lock();

                // Send ACK 
                uint8_t retry_ack = 0;
                while(HAL_I2C_Mem_Write(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, ack_msg, 4, 50) != HAL_OK) {
                    HAL_Delay(5);
                    retry_ack++;
                    if(retry_ack > 5){
                        OTA_ReInit_I2C1();
                    }
                    if (retry_ack > 10) {
                        printf("[OTA] Failed to send ACK after 10 attempts, aborting...\r\n");
                        break;
                    }
               }
            HAL_Delay(5);
        }
    }
    // --- End Command & CRC Check (0x03) ---
        else if (header[0] == 0x03 && ota_in_progress)
        {
            uint32_t total_received_size = flash_write_addr - INACTIVE_BANK_ADDR;
            printf("\r\n[OTA] END Command Received. Total Downloaded: %lu Bytes.\r\n", total_received_size);
                        
            if (total_received_size > 4) {
                HAL_ICACHE_Disable();
                HAL_ICACHE_Enable();
                                
                uint32_t actual_fw_size = total_received_size - 4;
                volatile uint8_t *crc_bytes = (volatile uint8_t*)(INACTIVE_BANK_ADDR + actual_fw_size);
                uint32_t received_crc32 = ((uint32_t)crc_bytes[3] << 24) | ((uint32_t)crc_bytes[2] << 16) | 
                                          ((uint32_t)crc_bytes[1] << 8) | ((uint32_t)crc_bytes[0]);
                                
                uint32_t calculated_crc32 = crc32_calculate((uint8_t*)INACTIVE_BANK_ADDR, actual_fw_size);

                if (calculated_crc32 == received_crc32) {
                    printf("[OTA] CRC32 VERIFIED (0x%08lX)!\r\n", calculated_crc32);
                                        
                    // Send End ACK
                    uint8_t ack_end[4] = {0xCC, 0x00, 0x00, 0x00};
                    HAL_I2C_Mem_Write(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, ack_end, 4, 100);
                    
                    // Proceed with Swap
                    Execute_Hardware_Swap_And_Reset();
                } else {
                    printf("[OTA] CRC32 MISMATCH (Calc: 0x%08lX, RX: 0x%08lX)!\r\n", calculated_crc32, received_crc32);
                                        
                    // ======== Dump First/Last 16 Bytes of Flash ========
                    printf("\r\n--- RX Flash Memory Dump ---\r\n");
                    printf("First 16 Bytes: ");
                    for(int i = 0; i < 16; i++) {
                        printf("%02X ", *((volatile uint8_t*)(INACTIVE_BANK_ADDR + i)));
                    }
                    printf("\r\nLast 16 Bytes:  ");
                    for(int i = 16; i > 0; i--) {
                        printf("%02X ", *((volatile uint8_t*)(INACTIVE_BANK_ADDR + actual_fw_size - i)));
                    }
                    printf("\r\n----------------------------\r\n");
                    
                    printf("[OTA] App continues running safely.\r\n");
                                        
                    uint8_t ack_fail[4] = {0xFF, 0xFF, 0xFF, 0xFF};
                    HAL_I2C_Mem_Write(&hi2c1, (0x54 << 1), 0x2000, I2C_MEMADD_SIZE_16BIT, ack_fail, 4, 100);
                }
                             
            }
            ota_in_progress = 0;
            HAL_GPIO_WritePin(LED_Bootloader_GPIO_Port, LED_Bootloader_Pin, GPIO_PIN_RESET);
        }
    } else {
        // Check I2C Error State
        if (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
            ntag5_Clear_I2C_Locked_Robust();
            OTA_ReInit_I2C1();
        }
    }
}