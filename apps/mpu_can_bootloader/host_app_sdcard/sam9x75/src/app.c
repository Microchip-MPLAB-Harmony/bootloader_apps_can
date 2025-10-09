/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries.
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

#include "app.h"
#include "definitions.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

APP_DATA appData;

static uint8_t CACHE_ALIGN __attribute__((__section__(".region_sram"))) mcan1MessageRAM[MCAN1_MESSAGE_RAM_CONFIG_SIZE];

static uint32_t status = 0;
static uint8_t loop_count = 0;

static uint8_t txFiFo[MCAN1_TX_FIFO_BUFFER_SIZE];
static uint8_t rxFiFo0[MCAN1_RX_FIFO0_SIZE];

static uint8_t CACHE_ALIGN sdCardBuffer[APP_DATA_LEN];

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

static void APP_SysFSEventHandler(SYS_FS_EVENT event,void* eventData,uintptr_t context)
{
    switch(event)
    {
        /* If the event is mount then check if SDCARD media has been mounted */
        case SYS_FS_EVENT_MOUNT:
            if(strcmp((const char *)eventData, SDCARD_MOUNT_NAME) == 0)
            {
                appData.sdCardMountFlag = true;
            }
            break;

        /* If the event is unmount then check if SDCARD media has been unmount */
        case SYS_FS_EVENT_UNMOUNT:
            if(strcmp((const char *)eventData, SDCARD_MOUNT_NAME) == 0)
            {
                appData.sdCardMountFlag = false;

                appData.state = APP_STATE_MOUNT_WAIT;
            }
            break;

        case SYS_FS_EVENT_ERROR:
        default:
            break;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

/* Message Length to Data length code */
static uint8_t MCANLengthToDlcGet(uint8_t length)
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
static uint8_t MCANDlcToLengthGet(uint8_t dlc)
{
    uint8_t msgLength[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
    return msgLength[dlc];
}

static void rx_message(uint8_t numberOfMessage, MCAN_RX_BUFFER *rxBuf, uint8_t rxBufLen)
{
    uint8_t msgLength = 0;
    uint32_t id = 0;

    for (uint8_t count = 0; count < numberOfMessage; count++)
    {
        id = rxBuf->xtd ? rxBuf->id : READ_ID(rxBuf->id);
        msgLength = MCANDlcToLengthGet(rxBuf->dlc);
        if ((id == MCAN_FILTER_ID) && (msgLength == 1))
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

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

static uint32_t APP_CRCGenerate(uint8_t *start_addr, uint32_t size, uint32_t crc)
{
    uint32_t   i, j, value;
    uint32_t   crc_tab[256];
    uint8_t    data;

    for (i = 0; i < 256U; i++)
    {
        value = i;

        for (j = 0; j < 8U; j++)
        {
            if ((value & 1U) != 0U)
            {
                value = (value >> 1) ^ 0xEDB88320U;
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
        data = start_addr[i];
        crc = crc_tab[(crc ^ data) & 0xffU] ^ (crc >> 8);
    }

    return crc;
}

void APP_Initialize ( void )
{
    appData.appMemStartAddr = APP_IMAGE_START_ADDR;
    appData.nBytesWritten = 0;
    appData.sdCardMountFlag = false;
    appData.fileSize        = 0;
    appData.imageSize       = 0;
    appData.crcVal          = 0xffffffff;
    appData.state = APP_STATE_WAIT_SW_PRESS;
    LED_OFF();

    /* Set MCAN1 Message RAM Configuration */
    MCAN1_MessageRAMConfigSet(mcan1MessageRAM);

    /* Register the File System Event handler */
    SYS_FS_EventHandlerSet((void const*)APP_SysFSEventHandler,(uintptr_t)NULL);
}

static int32_t APP_SDCARD_ReadData(uint8_t* pBuffer, uint32_t nBytes)
{
    int32_t nBytesRead;

    nBytesRead = SYS_FS_FileRead(appData.fileHandle, (void *)pBuffer, nBytes);

    if (nBytesRead == -1)
    {
        /* There was an error while reading the file */
        SYS_FS_FileClose(appData.fileHandle);
        appData.fileHandle = SYS_FS_HANDLE_INVALID;
    }
    else
    {
        if(SYS_FS_FileEOF(appData.fileHandle) == true)
        {
            SYS_FS_FileClose(appData.fileHandle);
            appData.fileHandle = SYS_FS_HANDLE_INVALID;
        }
    }
    return nBytesRead;
}

static uint32_t APP_ImageDataWrite(uint32_t nBytes)
{
    uint32_t nTxBytes = 0;
    int32_t  nDataBytesRead = 0;
    static uint8_t sequence = 0;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_DATA;
    appData.wrBuffer[nTxBytes++] = sequence++;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = nBytes & 0xFF;

    nDataBytesRead = APP_SDCARD_ReadData(sdCardBuffer, nBytes);
    if (nDataBytesRead < 0)
    {
        nTxBytes = 0;
        /* There is an error reading the data */
        appData.state = APP_STATE_ERROR;
    }
    else
    {
        /* Copy the data read from SD card to wrBuffer */
        memcpy(&appData.wrBuffer[nTxBytes], sdCardBuffer, nDataBytesRead);

        /* Calculate CRC on data being sent to bootloader */
        appData.crcVal = APP_CRCGenerate(&appData.wrBuffer[nTxBytes], nBytes, appData.crcVal);
        nTxBytes += nBytes;
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

    if (MCAN1_InterruptGet(MCAN_INTERRUPT_RF0N_MASK))
    {
        MCAN1_InterruptClear(MCAN_INTERRUPT_RF0N_MASK);

        /* Check MCAN1 Status */
        status = MCAN1_ErrorGet();
        if (((status & MCAN_PSR_LEC_Msk) == MCAN_ERROR_NONE) || ((status & MCAN_PSR_LEC_Msk) == MCAN_ERROR_LEC_NO_CHANGE))
        {
            numberOfMessage = MCAN1_RxFifoFillLevelGet(MCAN_RX_FIFO_0);
            if (numberOfMessage != 0)
            {
                memset(rxFiFo0, 0x00, (numberOfMessage * MCAN1_RX_FIFO0_ELEMENT_SIZE));
                if (MCAN1_MessageReceiveFifo(MCAN_RX_FIFO_0, numberOfMessage, (MCAN_RX_BUFFER *)rxFiFo0) == true)
                {
                    rx_message(numberOfMessage, (MCAN_RX_BUFFER *)rxFiFo0, MCAN1_RX_FIFO0_ELEMENT_SIZE);
                }
            }
        }
    }
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    uint32_t nTxBytes;
    MCAN_TX_BUFFER *txBuffer = NULL;

    /* Check the application's current state. */
    switch (appData.state)
    {
        case APP_STATE_WAIT_SW_PRESS:
            if (SWITCH_GET() == SWITCH_PRESSED)
            {
                appData.state = APP_STATE_MOUNT_WAIT;
            }
            break;

        case APP_STATE_MOUNT_WAIT:
            /* Wait for SDCARD to be Auto Mounted */
            if(appData.sdCardMountFlag == true)
            {
                appData.state = APP_STATE_SET_CURRENT_DRIVE;
            }
            break;

        case APP_STATE_SET_CURRENT_DRIVE:
            if(SYS_FS_CurrentDriveSet(SDCARD_MOUNT_NAME) == SYS_FS_RES_FAILURE)
            {
                /* Error while setting current drive */
                appData.state = APP_STATE_ERROR;
            }
            else
            {
                /* Open a file for reading. */
                appData.state = APP_STATE_OPEN_FILE;
            }

            break;

        case APP_STATE_OPEN_FILE:
            appData.fileHandle = SYS_FS_FileOpen(APP_IMAGE_NAME,
                    (SYS_FS_FILE_OPEN_READ));
            if(appData.fileHandle == SYS_FS_HANDLE_INVALID)
            {
                /* Could not open the file. Error out*/
                appData.state = APP_STATE_ERROR;
            }
            else
            {
                appData.fileSize = SYS_FS_FileSize(appData.fileHandle);
                appData.imageSize = appData.fileSize;
                appData.state = APP_STATE_SEND_UNLOCK_COMMAND;
            }
            break;

        case APP_STATE_SEND_UNLOCK_COMMAND:
            nTxBytes = APP_UnlockCommandSend(APP_IMAGE_START_ADDR, appData.imageSize);
            appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

            memset(txFiFo, 0x00, MCAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
            txBuffer = (MCAN_TX_BUFFER *)txFiFo;
            txBuffer->id = WRITE_ID(MCAN_FILTER_ID);
            txBuffer->dlc = MCANLengthToDlcGet(nTxBytes);
            txBuffer->fdf = 1;
            txBuffer->brs = 1;
            for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
            }
            if (MCAN1_MessageTransmitFifo(1, txBuffer) == true)
            {
                appData.state = APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE;
            }
            else
            {
                appData.state = APP_STATE_ERROR;
            }
            break;
        case APP_STATE_SEND_DATA_COMMAND:
            appData.nPendingBytes = appData.imageSize - appData.nBytesWritten;
            if (appData.nPendingBytes >= MCAN_DATA_FRAME_SIZE)
            {
                appData.nBytesSent = MCAN_DATA_FRAME_SIZE - APP_PROTOCOL_HEADER_MAX_SIZE;
            }
            else if (appData.nPendingBytes >= APP_PROTOCOL_HEADER_MAX_SIZE)
            {
                appData.nBytesSent = APP_PROTOCOL_HEADER_MAX_SIZE;
            }
            else
            {
                appData.nBytesSent = appData.nPendingBytes;
            }
            nTxBytes = APP_ImageDataWrite(appData.nBytesSent);
            appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

            memset(txFiFo, 0x00, MCAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
            txBuffer = (MCAN_TX_BUFFER *)txFiFo;
            txBuffer->id = WRITE_ID(MCAN_FILTER_ID);
            txBuffer->dlc = MCANLengthToDlcGet(nTxBytes);
            txBuffer->fdf = 1;
            txBuffer->brs = 1;
            for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
            }
            if (MCAN1_MessageTransmitFifo(1, txBuffer) == true)
            {
                appData.state = APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE;
            }
            else
            {
                appData.state = APP_STATE_ERROR;
            }
            break;

        case APP_STATE_SEND_VERIFY_COMMAND:
            nTxBytes = APP_VerifyCommandSend(appData.crcVal);
            appData.trasnferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;

            memset(txFiFo, 0x00, MCAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
            txBuffer = (MCAN_TX_BUFFER *)txFiFo;
            txBuffer->id = WRITE_ID(MCAN_FILTER_ID);
            txBuffer->dlc = MCANLengthToDlcGet(nTxBytes);
            txBuffer->fdf = 1;
            txBuffer->brs = 1;
            for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
            }
            if (MCAN1_MessageTransmitFifo(1, txBuffer) == true)
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

            memset(txFiFo, 0x00, MCAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
            txBuffer = (MCAN_TX_BUFFER *)txFiFo;
            txBuffer->id = WRITE_ID(MCAN_FILTER_ID);
            txBuffer->dlc = MCANLengthToDlcGet(nTxBytes);
            txBuffer->fdf = 1;
            txBuffer->brs = 1;
            for (loop_count = 0; loop_count < nTxBytes; loop_count++){
                txBuffer->data[loop_count] = appData.wrBuffer[loop_count];
            }
            if (MCAN1_MessageTransmitFifo(1, txBuffer) == true)
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
                    if (appData.nBytesWritten < appData.imageSize)
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
            if (appData.fileHandle != SYS_FS_HANDLE_INVALID)
            {
                SYS_FS_FileClose(appData.fileHandle);
            }
            appData.state = APP_STATE_IDLE;
            break;

        case APP_STATE_IDLE:
            break;

        default:
            break;
    }
}

