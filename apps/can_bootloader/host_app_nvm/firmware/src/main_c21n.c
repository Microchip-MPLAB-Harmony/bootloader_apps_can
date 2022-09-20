/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the appData.state
    machines of all modules in the system
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
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>
#include "definitions.h"                // SYS function prototypes
#include "user.h"

#include APP_HEX_HEADER_FILE

/* Definitions */
#define APP_IMAGE_SIZE                              sizeof(image_pattern)
#define APP_IMAGE_END_ADDR                          (APP_IMAGE_START_ADDR + APP_IMAGE_SIZE)
#define APP_PROTOCOL_HEADER_MAX_SIZE                4
#define LED_ON()                                    LED_Clear()
#define LED_OFF()                                   LED_Set()
#define SWITCH_GET()                                SWITCH_Get()
#define SWITCH_PRESSED                              0

#define CAN_DATA_FRAME_SIZE                         (64UL)
#define CAN_FILTER_ID                               0x45A

/* Standard identifier id[28:18]*/
#define WRITE_ID(id) (id << 18)
#define READ_ID(id)  (id >> 18)

enum
{
    BL_RESP_OK          = 0x50,
    BL_RESP_ERROR       = 0x51,
    BL_RESP_INVALID     = 0x52,
    BL_RESP_CRC_OK      = 0x53,
    BL_RESP_CRC_FAIL    = 0x54,
    BL_RESP_SEQ_ERROR   = 0x55
};

typedef enum
{
    APP_BL_COMMAND_UNLOCK = 0xA0,
    APP_BL_COMMAND_DATA = 0xA1,
    APP_BL_COMMAND_VERIFY = 0xA2,
    APP_BL_COMMAND_RESET = 0xA3
}APP_BL_COMMAND;

typedef enum
{
    APP_STATE_INIT,
    APP_STATE_WAIT_SW_PRESS,
    APP_STATE_SEND_UNLOCK_COMMAND,
    APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE,
    APP_STATE_SEND_DATA_COMMAND,
    APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE,
    APP_STATE_SEND_VERIFY_COMMAND,
    APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE,
    APP_STATE_SEND_RESET_COMMAND,
    APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE,
    APP_STATE_SUCCESSFUL,
    APP_STATE_ERROR,
    APP_STATE_IDLE

} APP_STATES;

typedef enum
{
    APP_TRANSFER_STATUS_IN_PROGRESS,
    APP_TRANSFER_STATUS_SUCCESS,
    APP_TRANSFER_STATUS_ERROR,
    APP_TRANSFER_STATUS_IDLE,

} APP_TRANSFER_STATUS;

typedef struct
{
    APP_STATES                      state;
    volatile APP_TRANSFER_STATUS    trasnferStatus;
    uint32_t                        appMemStartAddr;
    uint32_t                        nBytesWritten;
    uint32_t                        nBytesSent;
    uint32_t                        nPendingBytes;
    uint8_t                         wrBuffer[CAN_DATA_FRAME_SIZE];
} APP_DATA;

/* Global Data */
APP_DATA                            appData;
static uint8_t CACHE_ALIGN          can0MessageRAM[CAN0_MESSAGE_RAM_CONFIG_SIZE];

static uint32_t status = 0;
static uint8_t loop_count = 0;

static uint8_t txFiFo[CAN0_TX_FIFO_BUFFER_SIZE];
static uint8_t rxFiFo0[CAN0_RX_FIFO0_SIZE];

// *****************************************************************************
// *****************************************************************************
// Section: Local functions
// *****************************************************************************
// *****************************************************************************

/* Message Length to Data length code */
static uint8_t CANLengthToDlcGet(uint8_t length)
{
    uint8_t dlc = 0;

    if (length <= 8U)
    {
        dlc = length;
    }
    else if (length <= 12U)
    {
        dlc = 0x9U;
    }
    else if (length <= 16U)
    {
        dlc = 0xAU;
    }
    else if (length <= 20U)
    {
        dlc = 0xBU;
    }
    else if (length <= 24U)
    {
        dlc = 0xCU;
    }
    else if (length <= 32U)
    {
        dlc = 0xDU;
    }
    else if (length <= 48U)
    {
        dlc = 0xEU;
    }
    else
    {
        dlc = 0xFU;
    }
    return dlc;
}

/* Data length code to Message Length */
static uint8_t CANDlcToLengthGet(uint8_t dlc)
{
    uint8_t msgLength[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
    return msgLength[dlc];
}

static void rx_message(uint8_t numberOfMessage, CAN_RX_BUFFER *rxBuf, uint8_t rxBufLen)
{
    uint8_t msgLength = 0;
    uint32_t id = 0;

    for (uint8_t count = 0; count < numberOfMessage; count++)
    {
        id = rxBuf->xtd ? rxBuf->id : READ_ID(rxBuf->id);
        msgLength = CANDlcToLengthGet(rxBuf->dlc);
        if ((id == CAN_FILTER_ID) && (msgLength == 1))
        {
            if ((rxBuf->data[0] == BL_RESP_OK) || (rxBuf->data[0] == BL_RESP_CRC_OK))
            {
                appData.trasnferStatus = APP_TRANSFER_STATUS_SUCCESS;
            }
            else
            {
                appData.trasnferStatus = APP_TRANSFER_STATUS_ERROR;
            }
            break;
        }
        rxBuf += rxBufLen;
    }
}

static void APP_Initialize(void)
{
    appData.appMemStartAddr = APP_IMAGE_START_ADDR;
    appData.nBytesWritten = 0;
    appData.state = APP_STATE_INIT;
    LED_OFF();
}

static uint32_t APP_CRCGenerate(void)
{
    uint32_t crc  = 0;

    PAC_PeripheralProtectSetup (PAC_PERIPHERAL_DSU, PAC_PROTECTION_CLEAR);

    DSU_CRCCalculate (
        (uint32_t)&image_pattern[0],
        APP_IMAGE_SIZE,
        0xffffffff,
        &crc
    );

    return crc;
}

static uint32_t APP_ImageDataWrite(uint32_t memAddr, uint32_t nBytes)
{
    uint32_t k;
    uint32_t nTxBytes = 0;
    uint32_t wrIndex = (memAddr - APP_IMAGE_START_ADDR);
    static uint8_t sequence = 0;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_DATA;
    appData.wrBuffer[nTxBytes++] = sequence++;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = nBytes & 0xFF;

    for (k = 0; k < nBytes; k++, nTxBytes++)
    {
        appData.wrBuffer[nTxBytes] = image_pattern[wrIndex + k];
    }

    return nTxBytes;
}

static uint32_t APP_UnlockCommandSend(uint32_t appStartAddr, uint32_t appSize)
{
    uint32_t nTxBytes = 0;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_UNLOCK;
    appData.wrBuffer[nTxBytes++] = 0;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = 8;
    appData.wrBuffer[nTxBytes++] = appStartAddr;
    appData.wrBuffer[nTxBytes++] = (appStartAddr >> 8);
    appData.wrBuffer[nTxBytes++] = (appStartAddr >> 16);
    appData.wrBuffer[nTxBytes++] = (appStartAddr >> 24);
    appData.wrBuffer[nTxBytes++] = appSize;
    appData.wrBuffer[nTxBytes++] = (appSize >> 8);
    appData.wrBuffer[nTxBytes++] = (appSize >> 16);
    appData.wrBuffer[nTxBytes++] = (appSize >> 24);

    return nTxBytes;
}

static uint32_t APP_VerifyCommandSend(uint32_t crc)
{
    uint32_t nTxBytes = 0;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_VERIFY;
    appData.wrBuffer[nTxBytes++] = 0;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = 4;
    appData.wrBuffer[nTxBytes++] = crc;
    appData.wrBuffer[nTxBytes++] = (crc >> 8);
    appData.wrBuffer[nTxBytes++] = (crc >> 16);
    appData.wrBuffer[nTxBytes++] = (crc >> 24);

    return nTxBytes;
}

static uint32_t APP_ResetCommandSend(void)
{
    uint32_t nTxBytes = 0;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_RESET;
    appData.wrBuffer[nTxBytes++] = 0;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = 0;

    return nTxBytes;
}

void APP_CheckBTLResponse(void)
{
    uint8_t numberOfMessage = 0;

    if (CAN0_InterruptGet(CAN_INTERRUPT_RF0N_MASK))
    {
        CAN0_InterruptClear(CAN_INTERRUPT_RF0N_MASK);

        /* Check CAN0 Status */
        status = CAN0_ErrorGet();
        if (((status & CAN_PSR_LEC_Msk) == CAN_ERROR_NONE) || ((status & CAN_PSR_LEC_Msk) == CAN_ERROR_LEC_NC))
        {
            numberOfMessage = CAN0_RxFifoFillLevelGet(CAN_RX_FIFO_0);
            if (numberOfMessage != 0)
            {
                memset(rxFiFo0, 0x00, (numberOfMessage * CAN0_RX_FIFO0_ELEMENT_SIZE));
                if (CAN0_MessageReceiveFifo(CAN_RX_FIFO_0, numberOfMessage, (CAN_RX_BUFFER *)rxFiFo0) == true)
                {
                    rx_message(numberOfMessage, (CAN_RX_BUFFER *)rxFiFo0, CAN0_RX_FIFO0_ELEMENT_SIZE);
                }
            }
        }
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
int main ( void )
{
    uint32_t nTxBytes;
    uint32_t crc;
    CAN_TX_BUFFER *txBuffer = NULL;

    /* Initialize all modules */
    SYS_Initialize ( NULL );

    APP_Initialize();

    /* Set CAN0 Message RAM Configuration */
    CAN0_MessageRAMConfigSet(can0MessageRAM);

    while(1)
    {
        switch (appData.state)
        {
            case APP_STATE_INIT:
                appData.state = APP_STATE_WAIT_SW_PRESS;
                break;

            case APP_STATE_WAIT_SW_PRESS:
                if (SWITCH_GET() == SWITCH_PRESSED)
                {
                    appData.state = APP_STATE_SEND_UNLOCK_COMMAND;
                }
                break;

            case APP_STATE_SEND_UNLOCK_COMMAND:
                nTxBytes = APP_UnlockCommandSend(APP_IMAGE_START_ADDR, APP_IMAGE_SIZE);
                appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

                memset(txFiFo, 0x00, CAN0_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *)txFiFo;
                txBuffer->id = WRITE_ID(CAN_FILTER_ID);
                txBuffer->dlc = CANLengthToDlcGet(nTxBytes);
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                    txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
                }                
                if (CAN0_MessageTransmitFifo(1, txBuffer) == true)
                {
                    appData.state = APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE;
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                }
                break;
            case APP_STATE_SEND_DATA_COMMAND:
                appData.nPendingBytes = APP_IMAGE_SIZE - appData.nBytesWritten;
                if (appData.nPendingBytes >= CAN_DATA_FRAME_SIZE)
                {
                    appData.nBytesSent = CAN_DATA_FRAME_SIZE - APP_PROTOCOL_HEADER_MAX_SIZE;
                }
                else if (appData.nPendingBytes >= APP_PROTOCOL_HEADER_MAX_SIZE)
                {
                    appData.nBytesSent = APP_PROTOCOL_HEADER_MAX_SIZE;
                }
                else
                {
                    appData.nBytesSent = appData.nPendingBytes;
                }
                nTxBytes = APP_ImageDataWrite((appData.appMemStartAddr + appData.nBytesWritten), appData.nBytesSent);
                appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

                memset(txFiFo, 0x00, CAN0_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *)txFiFo;
                txBuffer->id = WRITE_ID(CAN_FILTER_ID);
                txBuffer->dlc = CANLengthToDlcGet(nTxBytes);
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                    txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
                }                
                if (CAN0_MessageTransmitFifo(1, txBuffer) == true)
                {
                    appData.state = APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE;
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                }
                break;

            case APP_STATE_SEND_VERIFY_COMMAND:
                crc = APP_CRCGenerate();
                nTxBytes = APP_VerifyCommandSend(crc);
                appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

                memset(txFiFo, 0x00, CAN0_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *)txFiFo;
                txBuffer->id = WRITE_ID(CAN_FILTER_ID);
                txBuffer->dlc = CANLengthToDlcGet(nTxBytes);
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                    txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
                }                
                if (CAN0_MessageTransmitFifo(1, txBuffer) == true)
                {
                    appData.state = APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE;
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                }
                break;

            case APP_STATE_SEND_RESET_COMMAND:
                nTxBytes = APP_ResetCommandSend();
                appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

                memset(txFiFo, 0x00, CAN0_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *)txFiFo;
                txBuffer->id = WRITE_ID(CAN_FILTER_ID);
                txBuffer->dlc = CANLengthToDlcGet(nTxBytes);
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                    txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
                }                
                if (CAN0_MessageTransmitFifo(1, txBuffer) == true)
                {
                    appData.state = APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE;
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                }
                break;

            case APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE:
            case APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE:
            case APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE:
            case APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE:
                APP_CheckBTLResponse();
                if (appData.trasnferStatus == APP_TRANSFER_STATUS_SUCCESS)
                {
                    if (appData.state == APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE)
                    {
                        appData.state = APP_STATE_SEND_DATA_COMMAND;
                    }
                    else if (appData.state == APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE)
                    {
                        appData.nBytesWritten += appData.nBytesSent;
                        if (appData.nBytesWritten < APP_IMAGE_SIZE)
                        {
                            appData.state = APP_STATE_SEND_DATA_COMMAND;
                        }
                        else
                        {
                            appData.state = APP_STATE_SEND_VERIFY_COMMAND;
                        }
                    }
                    else if(appData.state == APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE)
                    {
                        appData.state = APP_STATE_SEND_RESET_COMMAND;
                    }
                    else if(appData.state == APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE)
                    {
                        appData.state = APP_STATE_SUCCESSFUL;
                    }
                }
                else if (appData.trasnferStatus == APP_TRANSFER_STATUS_ERROR)
                {
                    appData.state = APP_STATE_ERROR;
                }
                break;

            case APP_STATE_SUCCESSFUL:

                LED_ON();
                appData.state = APP_STATE_IDLE;
                break;

            case APP_STATE_ERROR:

                LED_OFF();
                appData.state = APP_STATE_IDLE;
                break;

            case APP_STATE_IDLE:
                break;

            default:
                break;
        }
    }
}

