---
title: Release notes
nav_order: 99
---

![Microchip logo](https://raw.githubusercontent.com/wiki/Microchip-MPLAB-Harmony/Microchip-MPLAB-Harmony.github.io/images/microchip_logo.png)
![Harmony logo small](https://raw.githubusercontent.com/wiki/Microchip-MPLAB-Harmony/Microchip-MPLAB-Harmony.github.io/images/microchip_mplab_harmony_logo_small.png)

# Microchip MPLAB® Harmony 3 Release Notes

## CAN Bootloader Applications Release v3.2.0

### New Features

- This release includes support of
    - CAN Bootloader Applications for PIC32M family of 32-bit microcontrollers.
    - CAN Bootloader Applications for PIC32CZ CA family of 32-bit microcontrollers.

### Development kit and demo application support
- The following table provides bootloader demo applications available for different development kits.

    | Product Family                 | Development Kits                                    | CAN              | CAN Fail Safe             |
    | ------------------------------ | --------------------------------------------------- | ---------------- | ------------------------- |
    | SAM C20/C21                    | [SAM C21N Xplained Pro Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAMC21-XPRO)   | Yes              | NA                        |
    | SAM D5x/E5x                    | [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/ATSAME54-XPRO)               | Yes              | Yes                       |
    | SAM E70/S70/V70/V71            | [SAM E70 Xplained Ultra Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAME70-XULT)   | Yes              | NA                        |
    | SAM E70/S70/V70/V71            | [SAM V71 Xplained Ultra Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/ATSAMV71-XULT)        | Yes              | NA                        |
    | PIC32MK GPD/GPE/MCF            | [PIC32MK GP Development Kit](https://www.microchip.com/developmenttools/ProductDetails/dm320106)                                                                                       | Yes              | No                       |
    | PIC32MK GPG/MCJ                | [PIC32MK MCJ Curiosity Pro](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/DT100113)                                                                                       | Yes              | NA                        |
    | PIC32MK GPK/MCM                | [PIC32MK MCM Curiosity Pro Development Board](https://www.microchip.com/Developmenttools/ProductDetails/EV31E34A)                    | Yes              | No                       |
    | PIC32MZ EF                     | [PIC32MZ EF Curiosity 2.0 Development Kit](https://www.microchip.com/en-us/development-tool/DM320209)                    | Yes              | No                       |
    | PIC32MZ W1                     | [PIC32 WFI32E Curiosity Board](https://www.microchip.com/Developmenttools/ProductDetails/EV12F11A)                                                                                       | Yes              | NA                        |
    | PIC32CZ CA                     | [PIC32CZ-CA80 Curiosity Ultra board](https://www.microchip.com/en-us/development-tool/ea61x20a)                    | Yes              | NA                        |

- **NA:** Lack of product capability

### Known Issues

- No changes from v3.0.0

### Development Tools

- [MPLAB® X IDE v6.15](https://www.microchip.com/mplab/mplab-x-ide)
- MPLAB® X IDE plug-ins:
  - MPLAB® Code Configurator (MCC) v5.4.1
- [MPLAB® XC32 C/C++ Compiler v4.35](https://www.microchip.com/mplab/compilers)


## CAN Bootloader Applications Release v3.1.0

### Development kit and demo application support
- The following table provides bootloader demo applications available for different development kits.

    | Product Family                 | Development Kits                                    | CAN              | CAN Fail Safe             |
    | ------------------------------ | --------------------------------------------------- | ---------------- | ------------------------- |
    | SAM C20/C21                    | [SAM C21N Xplained Pro Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAMC21-XPRO)   | Yes              | NA                        |
    | SAM D5x/E5x                    | [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/ATSAME54-XPRO)               | Yes              | Yes                       |
    | SAM E70/S70/V70/V71            | [SAM E70 Xplained Ultra Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAME70-XULT)   | Yes              | NA                        |
    | SAM E70/S70/V70/V71            | [SAM V71 Xplained Ultra Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/ATSAMV71-XULT)        | Yes              | NA                        |

- **NA:** Lack of product capability

### Known Issues

- No changes from v3.0.0

### Development Tools

* [MPLAB® X IDE v6.05](https://www.microchip.com/mplab/mplab-x-ide)
* [MPLAB® XC32 C/C++ Compiler v4.20](https://www.microchip.com/mplab/compilers)
* MPLAB® X IDE plug-ins:
  * MPLAB® Code Configurator (MCC) v5.1.17


## CAN Bootloader Applications Release v3.0.0

### New Features

- This release includes support of
    - CAN Bootloader Applications for SAM family of 32-bit microcontrollers.

    - CAN Fail Safe Bootloader for devices with dual flash bank support.

### Development kit and demo application support
- The following table provides bootloader demo applications available for different development kits.

    | Product Family                 | Development Kits                                    | CAN              | CAN Fail Safe             |
    | ------------------------------ | --------------------------------------------------- | ---------------- | ------------------------- |
    | SAM C20/C21                    | [SAM C21N Xplained Pro Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAMC21-XPRO)   | Yes              | NA                        |
    | SAM D5x/E5x                    | [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/ATSAME54-XPRO)               | Yes              | Yes                       |
    | SAM E70/S70/V70/V71            | [SAM E70 Xplained Ultra Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=ATSAME70-XULT)   | Yes              | NA                        |
    | SAM E70/S70/V70/V71            | [SAM V71 Xplained Ultra Evaluation Kit](https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/ATSAMV71-XULT)        | Yes              | NA                        |

- **NA:** Lack of product capability

### Known Issues

The current known issues are as follows:

- Initialized global variables will not be initialized at startup for CAN bootloaders.

### Development Tools

* [MPLAB® X IDE v5.50](https://www.microchip.com/mplab/mplab-x-ide)
* [MPLAB® XC32 C/C++ Compiler v3.00](https://www.microchip.com/mplab/compilers)
* MPLAB® X IDE plug-ins:
    * MPLAB® Harmony 3 Launcher v3.6.4 and above.
