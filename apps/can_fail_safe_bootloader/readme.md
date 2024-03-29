# CAN Fail Safe Bootloader

This example application shows how to use the Bootloader Library to bootload an application on device having dual flash bank feature using CAN-FD protocol

**Bootloader Application**

-   This is a fail safe bootloader application which resides from starting location of both the banks of device flash memory region for SAM devices with dual bank support

-   It uses CAN peripheral library in non-interrupt mode

-   Trigger Methods

    -   It uses the On board Switch as bootloader trigger pin to force enter the bootloader at reset of device

    -   It checks for bootloader request pattern **\(0x5048434D\)** from the starting 16 Bytes of RAM to force enter bootloader at reset of device


**NVM Host Application**

-   This is a embedded CAN host application which sends the application image stored in it's internal flash \(NVM\) to the inactive bank of the target board over the CAN bus

-   The user application binary must be converted to a header file containing the application image in HEX format in a C style array. A [btl\_bin\_to\_c\_array.py](../../docs/GUID-AA3E00A3-26EF-40CA-8811-8E1D00F4C227.md) utility is provided to do this conversion

    ![can_bootloader_host_nvm](../../docs/GUID-87F27F19-D77F-49A4-B642-34515BBBF63C-low.png)

-   To program the bootloader to the inactive flash bank, the user application binary may be combined with the bootloader using the [btl\_app\_merge\_bin.py](../../docs/GUID-B7DA02C7-050A-4FA6-BDD6-7474C61C1083.md) utility. The combined binary file must be converted to a header file using the [btl\_bin\_to\_c\_array.py](../../docs/GUID-AA3E00A3-26EF-40CA-8811-8E1D00F4C227.md) utility

    ![can_bootloader_host_nvm_btl_app_merge](../../docs/GUID-7981E68F-38A9-43F6-B556-B68EAFB2F9BE-low.png)

-   Add the generated image header file of the application or bootloader and application combined to the NVM host application project. Rebuild and program the NVM host application. This results in the application image being copied in the host MCU's flash \(NVM\)


**Test Application**

-   This is a test application which resides from end of bootloader size in device flash memory

-   It will be loaded into flash memory by bootloader application

-   It blinks an LED and provides console output

-   It uses the On board Switch to trigger the bootloader from firmware

    -   Once the switch is pressed it loads first 16 bytes of RAM with bootloader request pattern **\(0x5048434D\)** and resets the device


**Development Kits**<br />The following table provides links to documentation on how to build and run CAN Fail Safe bootloader on different development kits

-   **[SAM E54 Xplained Pro Evaluation Kit: Building and Running the CAN Fail Safe Bootloader applications](../../docs/GUID-D0185C3B-C96E-46AF-AD33-9863BCB43F51.md)**  


**Parent topic:**[MPLAB® Harmony 3 CAN Bootloader Application Examples](../../docs/GUID-6312510D-4637-4F9B-8A18-D2CCA5507E90.md)

