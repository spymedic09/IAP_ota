# NFC OTA Update via IAP Bootloader

This project implements an Over-The-Air (OTA) firmware update mechanism via NFC, utilizing an In-Application Programming (IAP) bootloader architecture.

## Project Structure

*   `nfc_RX_noFreeRTOS`: Example Main Application. You can replace this with your custom firmware.
*   `nfc_RX_noFreeRTOS/Bootloader`: The core bootloader module. Simply place this directory into your project and add `#include "iap_ota.h"` in the private includes section of your `main.c` to use it.
*   `TX`: The host transmitter firmware and tool used to transfer the binary file via NFC.
*   `bin_to_c_array.py`: A Python utility script to convert compiled `.bin` files into C header arrays (`.h`).
*   `OTA_Transfer_Architecture.md`: Documentation detailing the system's communication and transfer architecture.
*   `CRC32_Implementation.md`: Documentation covering the CRC32 checksum implementation and validation process.

## Getting Started

### 1. Dual-Bank Configuration
Before flashing, ensure the target MCU is configured for Dual-Bank memory:
1. Open STM32CubeProgrammer.
2. Navigate to Option Bytes (OB).
3. Verify that the `DBANK` option is checked. If it is unchecked, enable it and apply the changes.

### 2. RX Setup & Flashing
Open your project and import the target firmware. The following instructions use `nfc_RX_noFreeRTOS` as an example:
1. Copy the `Bootloader` folder into your project directory.
2. In your `main.c`, add the following line in the Private Includes section:
   ```c
   #include "iap_ota.h"