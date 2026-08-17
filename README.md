 NFC OTA Update via IAP Bootloader

This project implements an Over-The-Air (OTA) firmware update mechanism via NFC, utilizing an In-Application Programming (IAP) bootloader architecture.

 Project Structure

*   `nfc_RX_noFreeRTOS`: Example Main Application. You can replace this with your custom firmware.
*   `nfc_RX_noFreeRTOS/Bootloader`: The core bootloader module. Simply place this directory into your project and add `#include "iap_ota.h"` in the private includes section of your `main.c` to use it.
*   `TX`: The host transmitter firmware and tool used to transfer the binary file via NFC.
*   `bin_to_c_array.py`: A Python utility script to convert compiled `.bin` files into C header arrays (`.h`).
*   `OTA_Transfer_Architecture.md`: Documentation detailing the system's communication and transfer architecture.
*   `CRC32_Implementation.md`: Documentation covering the CRC32 checksum implementation and validation process.

 Getting Started

 1. Dual-Bank Configuration
Before flashing, ensure the target MCU is configured for Dual-Bank memory:
1. Open STM32CubeProgrammer.
2. Navigate to Option Bytes (OB).
3. Verify that the `DBANK` option is checked. If it is unchecked, enable it and apply the changes.

 2. RX Setup & Flashing
Open your project and import the target firmware. The following instructions use `nfc_RX_noFreeRTOS` as an example:
1. Copy the `Bootloader` folder into your project directory.
2. In your `main.c`, add the following line in the Private Includes section:
   ```c
   #include "iap_ota.h"
   ```
3. Compile the project and click Run to flash the firmware to the RX board.

### 3. TX Setup & Flashing
After compiling your target firmware into a `.bin` file, place it in the same directory as the `bin_to_c_array.py` script.

**Convert .bin to .h:**
Run the following command in your terminal:
```bash
# Usage:
python bin_to_c_array.py input.bin output.h array_name

# Example:
python bin_to_c_array.py red.bin OTA_App_V5_Green.h OTA_App_V5_Green
```
*Note: If you modify the `array_name`, you must update the `current_fw_data` pointer inside the `static void demoNfcv()` function within the `project file/X-CUBE-NFC6/demo_polling.c` file to match the new array name.*

**Flash the TX Board:**
1. Move the generated `.h` file into the `TX/Core/Inc` directory.
2. Compile and flash the TX project.

 Demonstration

1. Ensure both TX and RX boards are successfully flashed and powered on.
2. Place the TX and RX NFC antennas within communication range (Distance <= 8 cm).
3. Press Button 1 or Button 2 on the TX board's Mikro Key Click module to trigger the OTA transmission.

 Hardware Environment

This example is developed and validated using the following hardware setup:
*   **TX (Transmitter):** STM32L476RG + X-NUCLEO-NFC06A1 NFC Expansion Board + Mikro 2x2 Key Click.
*   **RX (Receiver):** STM32 Nucleo-L552ZE-Q + NXP OM2NTP5332 (NTAG 5).


 Settings

 TX Module
| Description |
| :--- |
| <ul><li>**Power:** Insert mini USB power.</li><li>**Button 1:** Transfer firmware 1 (Example: `red.bin`)</li><li>**Button 2:** Transfer firmware 2 (Example: `green.bin`)</li></ul> |

 RX Module
| Description |
| :--- |
| <ul><li>**Power:** Insert mini USB power.</li><li>**Operation:** Place the RX device antenna close to the TX antenna to begin the update process.</li></ul> |
