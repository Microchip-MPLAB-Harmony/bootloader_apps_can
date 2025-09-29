/*******************************************************************
  Company:
    Microchip Technology Inc.
    Memory System Service SMC Initialization File

  File Name:
    plib_hsmc.h

  Summary:
    Static Memory Controller (SMC) peripheral library interface.
    This file contains the source code to initialize the SMC controller

  Description
    This file defines the interface to the SMC peripheral library.  This
    library provides access to and control of the associated peripheral
    instance.

  Remarks:
    This header is for documentation only.  The actual smc<x> headers will be
    generated as required by the MCC (where <x> is the peripheral instance
    number).

    Every interface symbol has a lower-case 'x' in it following the "SMC"
    abbreviation.  This 'x' will be replaced by the peripheral instance number
    in the generated headers.  These are the actual functions that should be
    used.
*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef PLIB_HSMC_H
#define PLIB_HSMC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif

/* NAND Flash Macros */
/* Address for transferring command bytes, CLE A22 */
#define COMMAND_ADDR    0x400000U
/* Address for transferring address bytes, ALE A21 */
#define ADDRESS_ADDR    0x200000U

void HSMC_Initialize( void );

uint32_t HSMC_DataAddressGet(uint8_t chipSelect);

static inline void HSMC_CommandWrite(uint32_t dataAddress, uint8_t command)
{
    /* Send command */
    *((volatile uint8_t *)(dataAddress | COMMAND_ADDR)) = command;
}

static inline void HSMC_CommandWrite16(uint32_t dataAddress, uint16_t command)
{
    /* Send command */
    *((volatile uint16_t *)(dataAddress | COMMAND_ADDR)) = command;
}

static inline void HSMC_AddressWrite(uint32_t dataAddress, uint8_t address)
{
    /* Send address */
    *((volatile uint8_t *)(dataAddress | ADDRESS_ADDR)) = address;
}

static inline void HSMC_AddressWrite16(uint32_t dataAddress, uint16_t address)
{
    /* Send address */
    *((volatile uint16_t *)(dataAddress | ADDRESS_ADDR)) = address;
}

static inline void HSMC_DataWrite(uint32_t dataAddress, uint8_t data)
{
    *((volatile uint8_t *)dataAddress) = data;
}

static inline void HSMC_DataWrite16(uint32_t dataAddress, uint16_t data)
{
    *((volatile uint16_t *)dataAddress) = data;
}

static inline uint8_t HSMC_DataRead(uint32_t dataAddress)
{
    return *((volatile uint8_t *)dataAddress);
}

static inline uint16_t HSMC_DataRead16(uint32_t dataAddress)
{
    return *((volatile uint16_t *)dataAddress);
}

void HSMC_DataPhaseStart(bool writeEnable);
bool HSMC_StatusIsBusy(void);
uint8_t HSMC_ErrorGet(void);
int16_t HSMC_RemainderGet(uint32_t sector, uint32_t remainderIndex);
uint8_t HSMC_ECCGet(uint32_t sector, uint32_t byteIndex);
uint32_t HSMC_ErrorLocationGet(uint8_t position);
void HSMC_ErrorLocationDisable(void);
void HSMC_SigmaSet(uint32_t sigmaVal, uint32_t sigmaNum);
uint32_t HSMC_ErrorLocationFindNumOfRoots(uint32_t sectorSizeInBits, uint32_t errorNumber);

#ifdef __cplusplus // Provide C++ Compatibility
}
#endif

#endif // PLIB_HSMC_H

/*******************************************************************************
 End of File
*/
