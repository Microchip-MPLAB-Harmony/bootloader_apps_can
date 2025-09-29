/*******************************************************************
  Company:
    Microchip Technology Inc.
    Memory System Service SMC Initialization File

  File Name:
    plib_hsmc.c

  Summary:
    Static Memory Controller (SMC)
    This file contains the source code to initialize the SMC controller

  Description:
    SMC configuration interface
    The SMC System Memory interface provides a simple interface to manage 8-bit and 16-bit
    parallel devices.

  *******************************************************************/

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
#include "plib_hsmc.h"
#include "device.h"
#include <stddef.h>         // NULL

/* Function:
    void HSMC_Initialize( void )

  Summary:
    Initializes hardware and data for the given instance of the SMC module.

  Description:
    This function initializes the SMC timings according to the external parallel
    device requirements.

  Returns:
    None
 */

void HSMC_Initialize( void )
{
   // ------------------------------------------------------------------------
   // Disable Write Protection
   HSMC_REGS->HSMC_WPMR = HSMC_WPMR_WPKEY_PASSWD;

   // ------------------------------------------------------------------------
   // NAND Flash Controller (NFC) Configuration
   HSMC_REGS->HSMC_CFG = HSMC_CFG_PAGESIZE_PS512
                           ;
   // disable NAND Flash controller
   HSMC_REGS->HSMC_CTRL = HSMC_CTRL_NFCDIS_Msk;

   HSMC_REGS->HSMC_ADDR = HSMC_ADDR_ADDR_CYCLE0( 0 );

   HSMC_REGS->HSMC_BANK = 0;

   // ------------------------------------------------------------------------
   // Programmable Multi-bit Error Correction Code
   HSMC_REGS->HSMC_PMECCFG = HSMC_PMECCFG_BCH_ERR(1)
                               | HSMC_PMECCFG_SECTORSZ(0)
                               | HSMC_PMECCFG_PAGESIZE(3)
                               | HSMC_PMECCFG_AUTO_Msk
                               ;
   // PMECC Spare Area Size
   HSMC_REGS->HSMC_PMECCSAREA = HSMC_PMECCSAREA_SPARESIZE( 57U ); // 56 bytes
   // PMECC Start Address
   HSMC_REGS->HSMC_PMECCSADDR = HSMC_PMECCSADDR_STARTADDR( 2 );
   // PMECC End Address
   HSMC_REGS->HSMC_PMECCEADDR = HSMC_PMECCEADDR_ENDADDR( 57 );
   // Enable PMECC
   HSMC_REGS->HSMC_PMECCTRL = HSMC_PMECCTRL_ENABLE_Msk;

   // ------------------------------------------------------------------------
   // PMECC Error Location
       HSMC_REGS->HSMC_ELEN = HSMC_ELEN_ENINIT( 0 );

   // ------------------------------------------------------------------------
   // Chip Selection and Settings
   // ***** SMC CS0 *****
   // Setup Register
   HSMC_REGS->SMC_CS_NUMBER[ 0 ].HSMC_SETUP = HSMC_SETUP_NCS_RD_SETUP( 4 )
                                           | HSMC_SETUP_NRD_SETUP(     4 )
                                           | HSMC_SETUP_NCS_WR_SETUP(  4 )
                                           | HSMC_SETUP_NWE_SETUP(     4 )
                                           ;
   // Pulse Register
   HSMC_REGS->SMC_CS_NUMBER[ 0 ].HSMC_PULSE = HSMC_PULSE_NCS_RD_PULSE( 20 )
                                           | HSMC_PULSE_NRD_PULSE(     10 )
                                           | HSMC_PULSE_NCS_WR_PULSE(  20 )
                                           | HSMC_PULSE_NWE_PULSE(     10 )
                                           ;
   // Cycle Register
   HSMC_REGS->SMC_CS_NUMBER[ 0 ].HSMC_CYCLE = HSMC_CYCLE_NRD_CYCLE( 20 )
                                           | HSMC_CYCLE_NWE_CYCLE( 20 )
                                           ;
   // Timings Register
   HSMC_REGS->SMC_CS_NUMBER[ 0 ].HSMC_TIMINGS = HSMC_TIMINGS_NFSEL_Msk
                                           | HSMC_TIMINGS_TWB(  8 )
                                           | HSMC_TIMINGS_TRR(  8 )
                                           | HSMC_TIMINGS_TAR(  5 )
                                           | HSMC_TIMINGS_TADL( 15 )
                                           | HSMC_TIMINGS_TCLR( 4 )
                                           ;
   // Mode Register
    HSMC_REGS->SMC_CS_NUMBER[ 0 ].HSMC_MODE = HSMC_MODE_TDF_MODE_Msk
                                           | HSMC_MODE_TDF_CYCLES( 15 )
                                           | HSMC_MODE_DBW_BIT_8
                                           | HSMC_MODE_BAT_BYTE_SELECT
                                           | HSMC_MODE_EXNW_MODE_DISABLED
                                           | HSMC_MODE_WRITE_MODE_Msk
                                           | HSMC_MODE_READ_MODE_Msk
                                           ;
}

uint32_t HSMC_DataAddressGet(uint8_t chipSelect)
{
    uint32_t dataAddress = 0;

    switch (chipSelect)
    {
        case 0:
            dataAddress = EBI_CS0_ADDR;
            break;

        case 1:
            dataAddress = EBI_CS1_ADDR;
            break;

        default:
            /*default*/
            break;
    }
    return dataAddress;
}

void HSMC_DataPhaseStart(bool writeEnable)
{
    HSMC_REGS->HSMC_PMECCTRL = HSMC_PMECCTRL_RST_Msk;

    if (writeEnable)
    {
        HSMC_REGS->HSMC_PMECCFG &= ~HSMC_PMECCFG_AUTO_Msk;
        HSMC_REGS->HSMC_PMECCFG |= HSMC_PMECCFG_NANDWR_Msk;
    }
    else
    {
        HSMC_REGS->HSMC_PMECCFG &= ~HSMC_PMECCFG_NANDWR_Msk;
        if ((HSMC_REGS->HSMC_PMECCFG & HSMC_PMECCFG_SPAREEN_Msk) != HSMC_PMECCFG_SPAREEN_Msk)
        {
            HSMC_REGS->HSMC_PMECCFG |= HSMC_PMECCFG_AUTO_Msk;
        }
    }

    HSMC_REGS->HSMC_PMECCTRL = HSMC_PMECCTRL_DATA_Msk;
}

bool HSMC_StatusIsBusy(void)
{
    return ((HSMC_REGS->HSMC_PMECCSR & HSMC_PMECCSR_BUSY_Msk) != 0U);
}

uint8_t HSMC_ErrorGet(void)
{
    return (uint8_t)(HSMC_REGS->HSMC_PMECCISR & HSMC_PMECCISR_ERRIS_Msk);
}

int16_t HSMC_RemainderGet(uint32_t sector, uint32_t remainderIndex)
{
    uint8_t lowByte = ((volatile const uint8_t *)&HSMC_REGS->SMC_REM[sector].HSMC_REM[0])[remainderIndex * 2U];
    uint8_t highByte = ((volatile const uint8_t *)&HSMC_REGS->SMC_REM[sector].HSMC_REM[0])[remainderIndex * 2U + 1U];
    uint32_t retVal = ((uint32_t)highByte << 8) + (uint32_t)lowByte;
    return (int16_t)retVal;
}

uint8_t HSMC_ECCGet(uint32_t sector, uint32_t byteIndex)
{
    return ((volatile const uint8_t *)HSMC_REGS->SMC_PMECC[sector].HSMC_PMECC)[byteIndex];
}

uint32_t HSMC_ErrorLocationGet(uint8_t position)
{
    return HSMC_REGS->HSMC_ERRLOC[position];
}

void HSMC_ErrorLocationDisable(void)
{
    HSMC_REGS->HSMC_ELDIS = HSMC_ELDIS_DIS_Msk;
}

void HSMC_SigmaSet(uint32_t sigmaVal, uint32_t sigmaNum)
{
    volatile uint32_t* sigma_base_address = (volatile uint32_t*)(HSMC_BASE_ADDRESS + HSMC_SIGMA0_REG_OFST);
    sigma_base_address[sigmaNum] = sigmaVal;
}

uint32_t HSMC_ErrorLocationFindNumOfRoots(uint32_t sectorSizeInBits, uint32_t errorNumber)
{
    /* Configure and enable error location process */
    HSMC_REGS->HSMC_ELCFG = (HSMC_REGS->HSMC_ELCFG & ~HSMC_ELCFG_ERRNUM_Msk) | HSMC_ELCFG_ERRNUM(errorNumber);
    HSMC_REGS->HSMC_ELEN = sectorSizeInBits;

    while ((HSMC_REGS->HSMC_ELISR & HSMC_ELISR_DONE_Msk) == 0U)
    {
        /* Wait for completion */
    }

    return ((HSMC_REGS->HSMC_ELISR & HSMC_ELISR_ERR_CNT_Msk) >> HSMC_ELISR_ERR_CNT_Pos);
}

/*******************************************************************************
 End of File
*/
