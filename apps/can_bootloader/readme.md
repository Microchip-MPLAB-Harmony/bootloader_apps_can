---
parent: CAN Bootloader Applications
title: CAN Bootloader
has_children: false
has_toc: false
nav_order: 1
---

[![MCHP](https://www.microchip.com/ResourcePackages/Microchip/assets/dist/images/logo.png)](https://www.microchip.com)

To clone or download this application from Github,go to the [main page of this repository](https://github.com/Microchip-MPLAB-Harmony/bootloader_apps_can) and then click Clone button to clone this repo or download as zip file. This content can also be download using content manager by following [these instructions](https://github.com/Microchip-MPLAB-Harmony/contentmanager/wiki)

# CAN Bootloader

This example application shows how to use the Bootloader Library to bootload an application using

- CAN-FD protocol for SAM devices
- CAN 2.0 and CAN-FD protocol for PIC32MK, PIC32MX and PIC32MZ devices

### Bootloader Application

- This is a bootloader application which resides from 
	- Starting location of the device flash memory for SAM devices
    - Starting location of the Boot flash memory region for PIC32MK, PIC32MX and PIC32MZ devices
- It uses CAN peripheral library in non-interrupt mode
- Trigger Methods
    - It uses the On board Switch as bootloader trigger pin to force enter the bootloader at reset of device
    - It checks for bootloader request pattern **(0x5048434D)** from the starting 16 Bytes of RAM to force enter bootloader at reset of device

### NVM Host Application

- This is a embedded CAN host application which sends the application image stored in it's internal flash (NVM) to the target board over the CAN bus
- The user application binary must be converted to a header file containing the application image in HEX format in a C style array. A [btl_bin_to_c_array.py](../../tools/docs/readme_btl_bin_to_c_array.md) utility is provided to do this conversion

    ![can_bootloader_host_nvm](../docs/images/can_bootloader_host_nvm.png)

- Add the generated application image header file to the NVM host application project. Rebuild and program the NVM host application. This results in the application image being copied in the host MCU's flash (NVM)

### Test Application

- This is a test application which resides from:
	- End of bootloader size in device flash memory for SAM devices
	- Starting location of the device Program memory for PIC32MK, PIC32MX and PIC32MZ devices
- It will be loaded into flash memory by bootloader application
- It blinks an LED and provides console output
- It uses the On board Switch to trigger the bootloader from firmware **(May not be supported on all devices)**
    - Once the switch is pressed it loads first 16 bytes of RAM with bootloader request pattern **(0x5048434D)** and resets the device

## Development Kits
The following table provides links to documentation on how to build and run CAN bootloader on different development kits

| Development Kit |
|:---------|
|[PIC32CZ CA80 Curiosity Ultra board](docs/readme_pic32cz_ca80_curiosity_ultra.md) |
|[PIC32MK GPE Development Kit](docs/readme_pic32mk_gp_db.md)		               |
|[PIC32MK MCJ Curiosity Pro Board](docs/readme_pic32mk_mcj_curiosity_pro.md)       |
|[PIC32MK MCM Curiosity Pro Board](docs/readme_pic32mk_mcm_curiosity_pro.md)       |
|[PIC32MZ EF Curiosity 2.0 Developement Kit](docs/readme_pic32mz_ef_curiosity.md)  |
|[PIC32MZ W1 Curiosity Board](docs/readme_pic32mz_w1_curiosity.md)                 |
|[SAM C21N Xplained Pro Evaluation Kit](docs/readme_sam_c21n_xpro.md)              |
|[SAM E54 Xplained Pro Evaluation Kit](docs/readme_sam_e54_xpro.md)                |
|[SAM E70 Xplained Ultra Evaluation Kit](docs/readme_sam_e70_xult.md)              |
|[SAM V71 Xplained Ultra Evaluation Kit](docs/readme_sam_v71_xult.md)              |
