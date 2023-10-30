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
#include "bootloader_common.h"
#include <device.h>

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

#define ADDR_OFFSET              1
#define SIZE_OFFSET              2
#define CRC_OFFSET               1

#define HEADER_CMD_OFFSET        0
#define HEADER_SEQ_OFFSET        1
#define HEADER_MAGIC_OFFSET      2
#define HEADER_SIZE_OFFSET       3

#define CRC_SIZE                 4
#define HEADER_SIZE              4
#define OFFSET_SIZE              4
#define SIZE_SIZE                4
#define MAX_DATA_SIZE            60


#define HEADER_MAGIC             0xE2
#define CAN_FILTER_ID            0x45A

/* Standard identifier id[28:18]*/
#define WRITE_ID(id)             (id << 18U)
#define READ_ID(id)              (id >> 18U)

#define WORDS(x)                 ((int)((x) / sizeof(uint32_t)))

#define OFFSET_ALIGN_MASK        (~ERASE_BLOCK_SIZE + 1)
#define SIZE_ALIGN_MASK          (~PAGE_SIZE + 1)

/* Compare Value to achieve a 100Ms Delay */
#define TIMER_COMPARE_VALUE     (CORE_TIMER_FREQUENCY / 10)

/* CAN Tx FIFO size */
#define CAN_TX_FIFO_BUFFER_SIZE     16U

enum
{
    BL_CMD_UNLOCK       = 0xA0,
    BL_CMD_DATA         = 0xA1,
    BL_CMD_VERIFY       = 0xA2,
    BL_CMD_RESET        = 0xA3,
    BL_CMD_READ_VERSION = 0xA6,
};

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

static uint8_t  CACHE_ALIGN flash_data[PAGE_SIZE];
static uint32_t flash_addr, flash_size, flash_ptr;

static uint32_t unlock_begin, unlock_end;

static uint8_t rx_message[HEADER_SIZE + MAX_DATA_SIZE];

static uint8_t data_seq = 0;

static bool canBLActive        = false;

// *****************************************************************************
// *****************************************************************************
// Section: Bootloader Local Functions
// *****************************************************************************
// *****************************************************************************

/* Function to program received application firmware data into internal flash */
static void flash_write(void)
{
    if (0 == (flash_addr % ERASE_BLOCK_SIZE))
    {
        /* Erase the Current sector */
        NVM_PageErase(flash_addr);

        while(NVM_IsBusy() == true)
        {
        }
    }

    /* Write Page */
    NVM_RowWrite((uint32_t *)&flash_data[0], flash_addr);

    while(NVM_IsBusy() == true)
{
    }
}

/* Function to process command from the received message */
static void process_command(uint8_t *rx_message, uint8_t rx_messageLength)
{
    uint32_t command = rx_message[HEADER_CMD_OFFSET];
    uint32_t size = rx_message[HEADER_SIZE_OFFSET];
    uint32_t *data = (uint32_t *)rx_message;
    uint8_t  tx_message[3];

    if ((rx_messageLength < HEADER_SIZE) || (size > MAX_DATA_SIZE) ||
        (rx_messageLength < (HEADER_SIZE + size)) || (HEADER_MAGIC != rx_message[HEADER_MAGIC_OFFSET]))
    {
        tx_message[0] = BL_RESP_ERROR;
        CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
    }
    else if (BL_CMD_READ_VERSION == command)
    {
        uint16_t btlVersion = bootloader_GetVersion ();

        tx_message[0] = BL_RESP_OK;

        tx_message[1] = (uint8_t)((btlVersion >> 8) & 0xFF);
        tx_message[2] = (uint8_t)(btlVersion & 0xFF);

        CAN2_MessageTransmit(CAN_FILTER_ID, 3U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
    }
    else if (BL_CMD_UNLOCK == command)
    {
        uint32_t begin  = (data[ADDR_OFFSET] & OFFSET_ALIGN_MASK);

        uint32_t end    = begin + (data[SIZE_OFFSET] & SIZE_ALIGN_MASK);

        if (end > begin && end <= (FLASH_START + FLASH_LENGTH) && size == (OFFSET_SIZE + SIZE_SIZE))
        {
            unlock_begin = begin;
            unlock_end = end;
            tx_message[0] = BL_RESP_OK;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        }
        else
        {
            unlock_begin = 0;
            unlock_end = 0;
            tx_message[0] = BL_RESP_ERROR;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
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
            tx_message[0] = BL_RESP_ERROR;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        }
        else
        {
            for (uint8_t i = 0; i < size; i++)
            {
                if (0 == flash_size)
                {
                    tx_message[0] = BL_RESP_ERROR;
                    CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);

                    return;
                }

                flash_data[flash_ptr++] = rx_message[HEADER_SIZE + i];

                if (flash_ptr == PAGE_SIZE)
                {
                    flash_write();

                    flash_ptr = 0;
                    flash_addr += PAGE_SIZE;
                    flash_size -= PAGE_SIZE;
                }
            }
            data_seq++;

            tx_message[0] = BL_RESP_OK;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        }
    }
    else if (BL_CMD_VERIFY == command)
    {
        uint32_t crc        = data[CRC_OFFSET];
        uint32_t crc_gen    = 0;

        if (size != CRC_SIZE)
        {
            tx_message[0] = BL_RESP_ERROR;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        }

        crc_gen = bootloader_CRCGenerate(unlock_begin, (unlock_end - unlock_begin));

        if (crc == crc_gen)
        {
            tx_message[0] = BL_RESP_CRC_OK;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        }
        else
        {
            tx_message[0] = BL_RESP_CRC_FAIL;
            CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        }
    }
    else if (BL_CMD_RESET == command)
    {
        tx_message[0] = BL_RESP_OK;
        CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
        while (CAN2_InterruptGet(1U, CANFD_FIFO_INTERRUPT_TFERFFIF_MASK) == false)
        {
        }

        bootloader_TriggerReset();
    }
    else
    {
        tx_message[0] = BL_RESP_INVALID;
        CAN2_MessageTransmit(CAN_FILTER_ID, 1U, tx_message, 1U, CANFD_MODE_NORMAL, CANFD_MSG_TX_DATA_FRAME);
    }
}

/* Function to receive application firmware via CAN2 */
static void CAN2_task(void)
{
    uint32_t status = 0;
    uint32_t rx_messageID = 0;
    uint8_t  rx_messageLength = 0;
    CANFD_MSG_RX_ATTRIBUTE msgFrameAttr = CANFD_MSG_RX_DATA_FRAME;

    if (CAN2_InterruptGet(2U, CANFD_FIFO_INTERRUPT_TFNRFNIF_MASK))
    {
        /* Check CAN2 Status */
        status = CAN2_ErrorGet();
        if (status == CANFD_ERROR_NONE)
        {
            canBLActive = true;

            memset (rx_message, 0x00, sizeof(rx_message));

            /* Receive FIFO 1 New Message */
            if (CAN2_MessageReceive(&rx_messageID, &rx_messageLength, rx_message, 0, 2, &msgFrameAttr) == true)
            {
                /* Check CAN2 Status */
                status = CAN2_ErrorGet();
                if (status == CANFD_ERROR_NONE)
                {
                    process_command(rx_message, rx_messageLength);
                    (void)rx_messageID;
                }
                else
                    canBLActive = false;
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
    do
    {
        CAN2_task();

    }  while (canBLActive);
}
