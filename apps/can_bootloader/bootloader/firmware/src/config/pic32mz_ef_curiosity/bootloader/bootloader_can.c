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
#define SIZE_OFFSET              1
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
#define CAN_FILTER_ID			 0x45A

/* Standard identifier id[28:18]*/
#define WRITE_ID(id)             (id << 18U)
#define READ_ID(id)              (id >> 18U)

#define WORDS(x)                 ((int)((x) / sizeof(uint32_t)))

#define OFFSET_ALIGN_MASK        (~ERASE_BLOCK_SIZE + 1)
#define SIZE_ALIGN_MASK          (~PAGE_SIZE + 1)

/* Compare Value to achieve a 100Ms Delay */
#define TIMER_COMPARE_VALUE     (CORE_TIMER_FREQUENCY / 10)

/* CAN Tx FIFO size */
#define CAN_TX_FIFO_BUFFER_SIZE		16U

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

static uint8_t	CACHE_ALIGN flash_data[PAGE_SIZE];
static uint32_t flash_addr, flash_size, flash_ptr;

static uint32_t begin, end;
static uint32_t unlock_begin, unlock_end;

static uint8_t rx_message[HEADER_SIZE + MAX_DATA_SIZE];
static uint8_t txFiFo [CAN_TX_FIFO_BUFFER_SIZE];

CAN_MSG_RX_ATTRIBUTE msgAttr = CAN_MSG_RX_DATA_FRAME;

static uint8_t data_seq = 0;

static bool canBLInitDone      = false;

static bool canBLActive        = false;

// *****************************************************************************
// *****************************************************************************
// Section: Bootloader Local Functions
// *****************************************************************************
// *****************************************************************************

/* Function to Generate CRC by reading the firmware programmed into internal flash */
static uint32_t crc_generate(void)
{
    uint32_t   i, j, value;
    uint32_t   crc_tab[256];
    uint32_t   size    = unlock_end - unlock_begin;
    uint32_t   crc     = 0xffffffff;
    uint8_t    data;

    for (i = 0; i < 256; i++)
    {
        value = i;

        for (j = 0; j < 8; j++)
        {
            if (value & 1)
            {
                value = (value >> 1) ^ 0xEDB88320;
            }
            else
            {
                value >>= 1;
            }
        }
        crc_tab[i] = value;
    }

    for (i = 0; i < size; i++)
    {
        data = *(uint8_t *)KVA0_TO_KVA1(unlock_begin + i);
        crc = crc_tab[(crc ^ data) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

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
    CAN_TX_RX_MSG_BUFFER *txBuffer = NULL;
    
    memset (txFiFo, 0x00, CAN_TX_FIFO_BUFFER_SIZE);
    txBuffer = (CAN_TX_RX_MSG_BUFFER *)txFiFo;
    txBuffer->msgSID = WRITE_ID (CAN_FILTER_ID);

    if ((rx_messageLength < HEADER_SIZE) || (size > MAX_DATA_SIZE) ||
        (rx_messageLength < (HEADER_SIZE + size)) || (HEADER_MAGIC != rx_message[HEADER_MAGIC_OFFSET]))
	{
        LED2_Clear ();
        
		txBuffer->msgData[0] = BL_RESP_ERROR;
        CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
        while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
    }
    else if (BL_CMD_READ_VERSION == command)
    {
        uint16_t btlVersion = bootloader_GetVersion ();
        
        txBuffer->msgData[0] = BL_RESP_OK;
        
        txBuffer->msgData[1] = (uint8_t)((btlVersion >> 8) & 0xFF);
        txBuffer->msgData[2] = (uint8_t)(btlVersion & 0xFF);        
        
        CAN2_MessageTransmit(txBuffer->msgSID, 3, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
        while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
    }
    else if (BL_CMD_UNLOCK == command)
    {        
		if (rx_message[HEADER_SEQ_OFFSET] != data_seq)
		{
            LED2_Clear ();
            
			txBuffer->msgData[0] = BL_RESP_SEQ_ERROR;
			CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
			while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
		}
		else
		{
			if (data_seq == 0)
			{
                LED_Clear ();
                
				begin = (data[ADDR_OFFSET] & OFFSET_ALIGN_MASK);
				
				txBuffer->msgData[0] = BL_RESP_OK;
				CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
				while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
                
                LED_Set ();
			}
			else if (data_seq == 1)
			{
				end = begin + (data[SIZE_OFFSET] & SIZE_ALIGN_MASK);
				size += size;
				
				if (end > begin && end <= (FLASH_START + FLASH_LENGTH) && size == (OFFSET_SIZE + SIZE_SIZE))
				{					
					unlock_begin = begin;
					unlock_end = end;
				
                    LED_Clear ();
                
					txBuffer->msgData[0] = BL_RESP_OK;
					CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);
					while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
                    
                    LED_Set ();
				}
				else
				{
                    LED2_Clear ();
                    
					unlock_begin = 0;
					unlock_end = 0;
				
					txBuffer->msgData[0] = BL_RESP_ERROR;
					CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
					while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
				}
			}
			else
			{
                LED2_Clear ();
                
				unlock_begin = 0;
				unlock_end = 0;
			
				txBuffer->msgData[0] = BL_RESP_ERROR;
				CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
				while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
			}
			
			data_seq++;
			flash_ptr = 0;
			flash_addr = unlock_begin;
			flash_size = unlock_end;
		}
    }
    else if (BL_CMD_DATA == command)
    {
        if (rx_message[HEADER_SEQ_OFFSET] != data_seq)
        {
            LED2_Clear ();
            
			txBuffer->msgData[0] = BL_RESP_ERROR;
			CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
			while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
        }
        else
        {
            for (uint8_t i = 0; i < size; i++)
            {
                if (0 == flash_size)
                {
                    LED2_Clear ();
                    
					txBuffer->msgData[0] = BL_RESP_ERROR;
					CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
					while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
					
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
			
            LED_Clear ();
            
            txBuffer->msgData[0] = BL_RESP_OK;
            CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);
			while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
            
            LED_Set ();
        }
    }
    else if (BL_CMD_VERIFY == command)
    {
        uint32_t crc        = data[CRC_OFFSET];
        uint32_t crc_gen    = 0;

        if (size != CRC_SIZE)
        {	
            LED2_Clear ();
            
			txBuffer->msgData[0] = BL_RESP_ERROR;
			CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
			while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
        }

        crc_gen = crc_generate();

        if (crc == crc_gen)
        {
            LED_Clear ();			
            
            txBuffer->msgData[0] = BL_RESP_CRC_OK;
            CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
			while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
            
            LED_Set ();
        }
        else
        {
            LED2_Clear ();
            
			txBuffer->msgData[0] = BL_RESP_CRC_FAIL;
			CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);		
			while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
        }
    }
    else if (BL_CMD_RESET == command)
    {
        txBuffer->msgData[0] = BL_RESP_OK;
        
        LED_Clear ();
        
		CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, msgAttr);
        while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXNFULLIF_MASK) == false);

        bootloader_TriggerReset();
    }
    else
    {
        LED2_Clear ();
        
        txBuffer->msgData[0] = BL_RESP_INVALID;
        CAN2_MessageTransmit(txBuffer->msgSID, 1, txBuffer->msgData, 0, CAN_MSG_RX_DATA_FRAME);
		while (CAN2_InterruptGet(0, CAN_FIFO_INTERRUPT_TXEMPTYIF_MASK) == false);
    }
}

/* Function to receive application firmware via CAN2 */
static void CAN2_task(void)
{
    uint32_t status = 0;
    uint32_t rx_messageID = 0;
    uint8_t  rx_messageLength = 0;
    CAN_MSG_RX_ATTRIBUTE msgFrameAttr = CAN_MSG_RX_DATA_FRAME;

    if (CAN2_InterruptGet(1, CAN_FIFO_INTERRUPT_RXNEMPTYIF_MASK))
    {
        /* Check CAN2 Status */
        status = CAN2_ErrorGet();
        if (status == CAN_ERROR_NONE)
        {
			canBLActive = true;
			
            memset (rx_message, 0x00, sizeof(rx_message));

            /* Receive FIFO 1 New Message */
            if (CAN2_MessageReceive(&rx_messageID, &rx_messageLength, rx_message, 0, 1, &msgFrameAttr) == true)
            {
				/* Check CAN2 Status */
				status = CAN2_ErrorGet();
				if (status == CAN_ERROR_NONE)
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
    if (canBLInitDone == false)
    {		
        LED_Clear ();
        LED3_Clear ();
        
        canBLInitDone = true;
    }

    do
    {
        CAN2_task();

    }  while (canBLActive);
}
