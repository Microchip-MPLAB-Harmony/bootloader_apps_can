/*******************************************************************************
  CAN Bootloader Source File

  File Name:
    bootloader_can.c

  Summary:
    This file contains source code necessary to execute CAN bootloader.

  Description:
    This file contains source code necessary to execute CAN bootloader.
    It implements bootloader protocol which uses CAN peripheral to download
    application firmware into internal flash from HOST.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
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

// *****************************************************************************
// *****************************************************************************
// Section: Include Files
// *****************************************************************************
// *****************************************************************************

#include "definitions.h"
#include <device.h>

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

#define ADDR_OFFSET              1U
#define SIZE_OFFSET              2
#define CRC_OFFSET               1

#define HEADER_CMD_OFFSET        0
#define HEADER_SEQ_OFFSET        1
#define HEADER_MAGIC_OFFSET      2
#define HEADER_SIZE_OFFSET       3

#define CRC_SIZE                 4U
#define HEADER_SIZE              4U
#define OFFSET_SIZE              4U
#define SIZE_SIZE                4U
#define MAX_DATA_SIZE            60U


#define HEADER_MAGIC              0xE2U
#define CAN_FILTER_ID  0x45AUL

/* Standard identifier id[28:18]*/
#define WRITE_ID(id)             ((id) << (18U))
#define READ_ID(id)              ((id) >> (18U))

#define WORDS(x)                 ((int)((x) / sizeof(uint32_t)))

#define OFFSET_ALIGN_MASK        ((~ERASE_BLOCK_SIZE) + (1U))
#define SIZE_ALIGN_MASK          ((~PAGE_SIZE) + (1U))


#define    BL_CMD_UNLOCK         (0xa0U)
#define    BL_CMD_DATA           (0xa1U)
#define    BL_CMD_VERIFY         (0xa2U)
#define    BL_CMD_RESET          (0xa3U)
#define    BL_CMD_READ_VERSION   (0xa6U)


enum
{
    BL_RESP_OK          = 0x50,
    BL_RESP_ERROR       = 0x51,
    BL_RESP_INVALID     = 0x52,
    BL_RESP_CRC_OK      = 0x53,
    BL_RESP_CRC_FAIL    = 0x54,
    BL_RESP_SEQ_ERROR   = 0x55
};

// *****************************************************************************
// *****************************************************************************
// Section: Global objects
// *****************************************************************************
// *****************************************************************************

static uint8_t  flash_data[PAGE_SIZE];
static uint32_t flash_addr;
static uint32_t flash_size;
static uint32_t flash_ptr;

static uint32_t unlock_begin;
static uint32_t unlock_end;

static uint8_t CACHE_ALIGN can1MessageRAM[CAN1_MESSAGE_RAM_CONFIG_SIZE];
static uint8_t rxFiFo0[CAN1_RX_FIFO0_SIZE];
static uint8_t txFiFo[CAN1_TX_FIFO_BUFFER_SIZE];
static uint8_t data_seq;

static bool canBLInitDone      = false;

static bool canBLActive        = false;

// *****************************************************************************
// *****************************************************************************
// Section: Bootloader Local Functions
// *****************************************************************************
// *****************************************************************************

/* Data length code to Message Length */
static uint8_t CANDlcToLengthGet(uint8_t dlc)
{
    uint8_t msgLength[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
    return msgLength[dlc];
}

/* Function to program received application firmware data into internal flash */
static void flash_write(void)
{
    if (0U == (flash_addr % ERASE_BLOCK_SIZE))
    {
        /* Lock region size is always bigger than the row size */
        NVMCTRL_RegionUnlock(flash_addr);

        while(NVMCTRL_IsBusy() == true)
        {
            /* Do Nothing */
        }

        /* Erase the Current sector */
        (void) NVMCTRL_BlockErase(flash_addr);

        while(NVMCTRL_IsBusy() == true)
        {
        }
    }

    /* Write Page */
    (void) NVMCTRL_PageWrite((void *)&flash_data[0], flash_addr);

    while(NVMCTRL_IsBusy() == true)
    {
    }
}

/* Function to process command from the received message */
static void process_command(uint8_t *rx_message, uint8_t rx_messageLength)
{
    uint32_t command = rx_message[HEADER_CMD_OFFSET];
    uint32_t size = rx_message[HEADER_SIZE_OFFSET];
    uint32_t *data = (uint32_t *)(uintptr_t)rx_message;
    CAN_TX_BUFFER *txBuffer = NULL;

    (void) memset(txFiFo, 0, CAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
    txBuffer = (CAN_TX_BUFFER *)(uintptr_t)txFiFo;
    txBuffer->id = WRITE_ID(CAN_FILTER_ID);
    txBuffer->dlc = 1U;
    txBuffer->fdf = 1U;
    txBuffer->brs = 1U;

    if ((rx_messageLength < HEADER_SIZE) || (size > MAX_DATA_SIZE) ||
        (rx_messageLength < (HEADER_SIZE + size)) || (HEADER_MAGIC != rx_message[HEADER_MAGIC_OFFSET]))
    {
        txBuffer->data[0] = BL_RESP_ERROR;
        (void) CAN1_MessageTransmitFifo(1U, txBuffer);
    }
    else if (BL_CMD_UNLOCK == command)
    {
        uint32_t begin  = (data[ADDR_OFFSET] & OFFSET_ALIGN_MASK);

        uint32_t end    = begin + (data[SIZE_OFFSET] & SIZE_ALIGN_MASK);

        if ((end > begin) && (end <= (FLASH_START + FLASH_LENGTH)) && (size == (OFFSET_SIZE + SIZE_SIZE)))
        {
            unlock_begin = begin;
            unlock_end = end;
            txBuffer->data[0] = BL_RESP_OK;
           (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }
        else
        {
            unlock_begin = 0;
            unlock_end = 0;
            txBuffer->data[0] = BL_RESP_ERROR;
            (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }
        data_seq = 0;
        flash_ptr = 0;
        flash_addr = unlock_begin;
        flash_size = unlock_end;
    }
    else if (BL_CMD_DATA == command)
    {
        if (rx_message[HEADER_SEQ_OFFSET] != data_seq)
        {
            txBuffer->data[0] = BL_RESP_SEQ_ERROR;
            (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }
        else
        {
            for (uint8_t i = 0; i < size; i++)
            {
                if (0U == flash_size)
                {
                    txBuffer->data[0] = BL_RESP_ERROR;
                    (void) CAN1_MessageTransmitFifo(1U, txBuffer);
                    return;
                }

                flash_data[flash_ptr] = rx_message[HEADER_SIZE + i];
                flash_ptr++;

                if (flash_ptr == PAGE_SIZE)
                {
                    flash_write();

                    flash_ptr = 0;
                    flash_addr += PAGE_SIZE;
                    flash_size -= PAGE_SIZE;
                }
            }
            data_seq++;
            txBuffer->data[0] = BL_RESP_OK;
            (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }
    }
    else if (BL_CMD_READ_VERSION == command)
    {
        uint16_t btlVersion = bootloader_GetVersion();

        txBuffer->data[0] = BL_RESP_OK;

        txBuffer->data[1] = (uint8_t)((btlVersion >> 8) & 0xFFU);
        txBuffer->data[2] = (uint8_t)(btlVersion & 0xFFU);

        (void) CAN1_MessageTransmitFifo(3U, txBuffer);
    }
    else if (BL_CMD_VERIFY == command)
    {
        uint32_t crc        = data[CRC_OFFSET];
        uint32_t crc_gen    = 0;

        if (size != CRC_SIZE)
        {
            txBuffer->data[0] = BL_RESP_ERROR;
           (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }

        crc_gen = bootloader_CRCGenerate(unlock_begin, (unlock_end - unlock_begin));

        if (crc == crc_gen)
        {
            txBuffer->data[0] = BL_RESP_CRC_OK;
            (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }
        else
        {
            txBuffer->data[0] = BL_RESP_CRC_FAIL;
            (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        }
    }
    else if (BL_CMD_RESET == command)
    {
        if (CAN1_InterruptGet(CAN_INTERRUPT_TFE_MASK))
        {
            CAN1_InterruptClear(CAN_INTERRUPT_TFE_MASK);
        }

        txBuffer->data[0] = BL_RESP_OK;
        (void) CAN1_MessageTransmitFifo(1U, txBuffer);
        while (CAN1_InterruptGet(CAN_INTERRUPT_TFE_MASK) == false)
        {
           /* Do Nothing */
        }

        NVIC_SystemReset();
    }
    else
    {
        txBuffer->data[0] = BL_RESP_INVALID;
        (void) CAN1_MessageTransmitFifo(1U, txBuffer);
    }
}

/* Function to receive application firmware via CAN1 */
static void CAN1_task(void)
{
    uint32_t                   status = 0U;
    uint8_t                    numberOfMessage = 0U;
    uint8_t                    count = 0U;
    CAN_RX_BUFFER   *rxBuf = NULL;

    if (CAN1_InterruptGet(CAN_INTERRUPT_RF0N_MASK))
    {
        CAN1_InterruptClear(CAN_INTERRUPT_RF0N_MASK);

        /* Check CAN1 Status */
        status = CAN1_ErrorGet();
        if (((status & CAN_PSR_LEC_Msk) == CAN_ERROR_NONE) || ((status & CAN_PSR_LEC_Msk) == CAN_ERROR_LEC_NC))
        {
            numberOfMessage = CAN1_RxFifoFillLevelGet(CAN_RX_FIFO_0);
            if (numberOfMessage != 0U)
            {
                canBLActive = true;

                if (numberOfMessage > (CAN1_RX_FIFO0_SIZE / CAN1_RX_FIFO0_ELEMENT_SIZE))
                {
                    numberOfMessage = CAN1_RX_FIFO0_SIZE / CAN1_RX_FIFO0_ELEMENT_SIZE;
                }

                (void) memset(rxFiFo0, 0, ((uint32_t)numberOfMessage * CAN1_RX_FIFO0_ELEMENT_SIZE));

                if (CAN1_MessageReceiveFifo(CAN_RX_FIFO_0, numberOfMessage, (void *)rxFiFo0) == true)
                {
                    for (count = 0U; count < numberOfMessage; count++)
                    {
                        rxBuf = (CAN_RX_BUFFER *)(uintptr_t)(&rxFiFo0[count * CAN1_RX_FIFO0_ELEMENT_SIZE]);
                        process_command(rxBuf->data, CANDlcToLengthGet(rxBuf->dlc));
                    }
                }
            }
        }
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Bootloader Global Functions
// *****************************************************************************
// *****************************************************************************

void bootloader_CAN_Tasks(void)
{
    if (canBLInitDone == false)
    {
        /* Set CAN1 Message RAM Configuration */
        CAN1_MessageRAMConfigSet(can1MessageRAM);

        canBLInitDone = true;
    }

    do
    {
        CAN1_task();

    }  while (canBLActive);
}
