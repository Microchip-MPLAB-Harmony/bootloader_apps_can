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
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include "definitions.h"                // SYS function prototypes
#include "user.h"
#include "sys/kmem.h"

#include APP_HEX_HEADER_FILE

/* Definitions */
#define APP_IMAGE_SIZE                              sizeof(image_pattern)
#define APP_IMAGE_END_ADDR                          (APP_IMAGE_START_ADDR + APP_IMAGE_SIZE)
#define APP_PROTOCOL_HEADER_MAX_SIZE                4
#define LED_ON()                                    LED_Set()
#define LED_OFF()                                   LED_Clear()
#define LED2_ON()                                   LED2_Set()
#define LED2_OFF()                                  LED2_Clear()
#define LED3_ON()                                   LED3_Set()
#define LED3_OFF()                                  LED3_Clear()
#define SWITCH_GET()                                SWITCH_Get()
#define SWITCH_PRESSED                              0

#define CAN_DATA_FRAME_SIZE                        (8UL)
#define CAN_FILTER_ID                              0x45A

/* Standard identifier id[28:18]*/
#define WRITE_ID(id) (id << 18)
#define READ_ID(id)  (id >> 18)

enum
{    
    BL_RESP_OK              = 0x50,
    BL_RESP_ERROR           = 0x51,
    BL_RESP_INVALID         = 0x52,
    BL_RESP_CRC_OK          = 0x53,
    BL_RESP_CRC_FAIL        = 0x54,
    BL_RESP_SEQ_ERROR       = 0x55
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
    volatile APP_TRANSFER_STATUS    transferStatus;
    uint32_t                        appMemStartAddr;
    uint32_t                        nBytesWritten;
    uint32_t                        nBytesSent;
    uint32_t                        nPendingBytes;
    uint8_t                         wrBuffer[CAN_DATA_FRAME_SIZE];
} APP_DATA;

/* Global Data */
APP_DATA                            appData;

static uint8_t status = 0;
static uint8_t loop_count = 0;

// Message Receive Variables
static uint32_t rx_messageID = 0;
static uint8_t rx_message[8];
static uint8_t rx_messageLength = 0;
CAN_MSG_RX_ATTRIBUTE msgAttr1 = CAN_MSG_RX_DATA_FRAME;

/* CAN3 Message RAM Configuration Size */
#define CAN3_RX_FIFO_SIZE                   8U
#define CAN3_TX_FIFO_BUFFER_SIZE            8U
#define CAN3_TX_EVENT_FIFO_SIZE             8U

static uint8_t txFiFo[CAN3_TX_FIFO_BUFFER_SIZE];
//static uint8_t rxFifo[CAN3_RX_FIFO_SIZE];

// *****************************************************************************
// *****************************************************************************
// Section: Local functions
// *****************************************************************************
// *****************************************************************************

static void APP_Initialize (void)
{
    appData.appMemStartAddr = APP_IMAGE_START_ADDR;
    appData.nBytesWritten = 0;
    appData.state = APP_STATE_INIT;
    LED_OFF();
}

static uint32_t crc_generate (void)
{
    uint32_t   i, j, value;
    uint32_t   crc_tab[256];
    uint32_t   size    = APP_IMAGE_SIZE;
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
        data = *(uint8_t *)(&image_pattern[0] + i);
        crc = crc_tab[(crc ^ data) & 0xff] ^ (crc >> 8);
    }
    printf ("CRC Value: 0x%08x\n\r", crc);
    return crc;
}

static uint32_t APP_CRCGenerate(void)
{
    uint32_t crc  = 0;

    crc = crc_generate();

    return crc;
}

static uint32_t APP_ImageDataWrite (uint32_t memAddr, uint32_t nBytes)
{
    uint32_t k;
    uint32_t nTxBytes = 0;
    uint32_t wrIndex = (memAddr - APP_IMAGE_START_ADDR);
    static uint8_t sequence = 2;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_DATA;
    appData.wrBuffer[nTxBytes++] = sequence++;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = nBytes & 0xFF;

    for (k = 0; k < nBytes; k++, nTxBytes++)
    {        
        appData.wrBuffer[nTxBytes] = image_pattern[wrIndex + k];
        printf ("Data to send: 0x%02x\n\r", appData.wrBuffer[nTxBytes]);
    }
    
    return nTxBytes;
}

static uint32_t APP_UnlockCommandSend (uint32_t appStartAddr, uint32_t appSize, uint32_t nBytes)
{
    uint32_t k;
    uint32_t nTxBytes = 0;    
    static uint8_t appConfiguration [8];    
    static uint8_t sequence = 0;
    
    appConfiguration[0] = appStartAddr;
    appConfiguration[1] = appStartAddr >> 8;
    appConfiguration[2] = appStartAddr >> 16;
    appConfiguration[3] = appStartAddr >> 24;
    
    appConfiguration[4] = appSize;
    appConfiguration[5] = appSize >> 8;
    appConfiguration[6] = appSize >> 16;
    appConfiguration[7] = appSize >> 24;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_UNLOCK;
    appData.wrBuffer[nTxBytes++] = sequence;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = nBytes & 0xFF;
    
    for (k = 0; k < nBytes; k++, nTxBytes++)
    {
        appData.wrBuffer[nTxBytes] = appConfiguration[k + 4 * sequence];
        printf ("Data to send: 0x%02x\n\r", appData.wrBuffer[nTxBytes]);
    }
    
    sequence++;

    return nTxBytes;
}

static uint32_t APP_VerifyCommandSend (uint32_t crc)
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

static uint32_t APP_ResetCommandSend (void)
{
    uint32_t nTxBytes = 0;

    appData.wrBuffer[nTxBytes++] = APP_BL_COMMAND_RESET;
    appData.wrBuffer[nTxBytes++] = 0;
    appData.wrBuffer[nTxBytes++] = 0xE2;
    appData.wrBuffer[nTxBytes++] = 0;

    return nTxBytes;
}

void APP_CheckBTLResponse (void)
{
    if (CAN3_InterruptGet(1, CAN_FIFO_INTERRUPT_RXNEMPTYIF_MASK)) {
        /* Check CAN3 Status */
        status = CAN3_ErrorGet();
        
        if (status == CAN_ERROR_NONE)
        {
            memset (rx_message, 0x00, sizeof(rx_message));
            
            if (CAN3_MessageReceive (&rx_messageID, &rx_messageLength, rx_message, 0, 1, &msgAttr1) == true)
            {
                /* Check CAN3 Status */
                status = CAN3_ErrorGet();
                
                if (status == CAN_ERROR_NONE)
                {
                    
                    if (rx_message[0] == BL_RESP_OK || rx_message[0] == BL_RESP_CRC_OK)
                    {
                        printf ("/*** Received successful ***/\n\r");
                        appData.transferStatus = APP_TRANSFER_STATUS_SUCCESS;
                    }
                    
                    else
                    {
                        printf ("Received failed\n\r");
                        appData.transferStatus = APP_TRANSFER_STATUS_ERROR;
                    }
                }
                
                else
                {
                    printf ("Error in application reply\n\r");
                    appData.transferStatus = APP_TRANSFER_STATUS_ERROR;
                }
            }
            else
            {
                printf ("Message reception failed\n\r");
                appData.transferStatus = APP_TRANSFER_STATUS_ERROR;
            }
        }
        else
        {
            printf ("Last CAN communication generated an error\n\r");
            appData.transferStatus = APP_TRANSFER_STATUS_ERROR;
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
    uint32_t crcValue;
    CAN_TX_RX_MSG_BUFFER *txBuffer;
    
    SYS_Initialize (NULL);
    
    APP_Initialize();
    
    while (true)
    {
        switch (appData.state)
        {
            case APP_STATE_INIT:
                appData.state = APP_STATE_WAIT_SW_PRESS;                
                printf ("/*** Please press the SW1 on-board button ***/\n\n\r");
                break;
                
            case APP_STATE_WAIT_SW_PRESS:
                if (SWITCH_GET() == SWITCH_PRESSED)
                {
                    printf ("/*** Button pressed ***/\n\n\r");
                    appData.state = APP_STATE_SEND_UNLOCK_COMMAND;
                }
                break;
                
            case APP_STATE_SEND_UNLOCK_COMMAND:
                printf ("/*** Unlock Command Start ***/\n\n\r");
                appData.nPendingBytes = CAN_DATA_FRAME_SIZE - appData.nBytesWritten;
                
                if (appData.nPendingBytes >= CAN_DATA_FRAME_SIZE)
                    appData.nBytesSent = CAN_DATA_FRAME_SIZE - APP_PROTOCOL_HEADER_MAX_SIZE;
                else if (appData.nPendingBytes >= APP_PROTOCOL_HEADER_MAX_SIZE)
                    appData.nBytesSent = APP_PROTOCOL_HEADER_MAX_SIZE;
                else
                    appData.nBytesSent = appData.nPendingBytes;              
                
                nTxBytes = APP_UnlockCommandSend (APP_IMAGE_START_ADDR, APP_IMAGE_SIZE, appData.nBytesSent);
                appData.transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                
                memset (txFiFo, 0x00, CAN3_TX_FIFO_BUFFER_SIZE);
                txBuffer = (CAN_TX_RX_MSG_BUFFER *)txFiFo;
                txBuffer->msgSID = WRITE_ID (CAN_FILTER_ID);
                
                printf ("Unlock command transfer in-progress\n\r");
                for (loop_count = 0; loop_count < nTxBytes; loop_count++)
                {
                    txBuffer->msgData[loop_count] = appData.wrBuffer[loop_count];
                    printf ("Data sent: 0x%02x\n\r", txBuffer->msgData[loop_count]);
                }
                
                printf ("Wait for Unlock command transfer to complete\n\r");
                if (CAN3_MessageTransmit (txBuffer->msgSID, 8, txBuffer->msgData, 0, CAN_MSG_TX_DATA_FRAME) == true)
                {
                    appData.state = APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE;
                    printf ("Wait for Unlock command transfer...\n\r");
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                    printf ("/*** Unlock command transfer failed ***/\n\n\r");
                }
                break;
                
            case APP_STATE_SEND_DATA_COMMAND:
                printf ("/*** Prepare for sending data ***/\n\n\r");
                
                appData.nPendingBytes = APP_IMAGE_SIZE - appData.nBytesWritten;
                
                if (appData.nPendingBytes >= CAN_DATA_FRAME_SIZE)
                    appData.nBytesSent = CAN_DATA_FRAME_SIZE - APP_PROTOCOL_HEADER_MAX_SIZE;
                else if (appData.nPendingBytes >= APP_PROTOCOL_HEADER_MAX_SIZE)
                    appData.nBytesSent = APP_PROTOCOL_HEADER_MAX_SIZE;
                else
                    appData.nBytesSent = appData.nPendingBytes;
                
                nTxBytes = APP_ImageDataWrite ((appData.appMemStartAddr + appData.nBytesWritten), appData.nBytesSent);
                appData.transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                
                memset (txFiFo, 0x00, CAN3_TX_FIFO_BUFFER_SIZE);
                txBuffer = (CAN_TX_RX_MSG_BUFFER *)txFiFo;
                txBuffer->msgSID = WRITE_ID(CAN_FILTER_ID);
                
                printf ("Data transfer in-progress\n\r");
                    
                for (loop_count = 0; loop_count < nTxBytes; loop_count++)
                {
                    txBuffer->msgData[loop_count] = appData.wrBuffer[loop_count];
                    printf ("Data sent: 0x%02x\n\r", txBuffer->msgData[loop_count]);
                }
                
                if (CAN3_MessageTransmit (txBuffer->msgSID, 8, txBuffer->msgData, 0, CAN_MSG_TX_DATA_FRAME) == true)
                {
                    appData.state = APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE;
                    printf ("Wait for Data transfer...\n\r");
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                    printf ("/*** Data transfer failed ***/\n\n\r");
                }
                break;
                
            case APP_STATE_SEND_VERIFY_COMMAND:
                crcValue =  APP_CRCGenerate ();
                nTxBytes = APP_VerifyCommandSend (crcValue);
                appData.transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                
                memset (txFiFo, 0x00, CAN3_TX_FIFO_BUFFER_SIZE);
                txBuffer = (CAN_TX_RX_MSG_BUFFER *)txFiFo;
                txBuffer->msgSID = WRITE_ID (CAN_FILTER_ID);
                
                for (loop_count = 0; loop_count < nTxBytes; loop_count++)
                {
                    txBuffer->msgData[loop_count] = appData.wrBuffer[loop_count];
                    printf ("Data sent: 0x%02x\n\r", txBuffer->msgData[loop_count]);
                }
                
                if (CAN3_MessageTransmit (txBuffer->msgSID, 8, txBuffer->msgData, 0, CAN_MSG_TX_DATA_FRAME) == true)
                {
                    appData.state = APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE;
                    printf ("Wait for Verify command transfer...\n\r");
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                    printf ("/*** Verify command transfer failed ***/\n\n\r");
                }
                break;
                
            case APP_STATE_SEND_RESET_COMMAND:
                nTxBytes = APP_ResetCommandSend ();
                appData.transferStatus = APP_TRANSFER_STATUS_IN_PROGRESS;
                
                memset (txFiFo, 0x00, CAN3_TX_FIFO_BUFFER_SIZE);
                txBuffer = (CAN_TX_RX_MSG_BUFFER *)txFiFo;
                txBuffer->msgSID = WRITE_ID(CAN_FILTER_ID);
                
                for (loop_count = 0; loop_count < nTxBytes; loop_count++)
                {
                    txBuffer->msgData[loop_count] = appData.wrBuffer[loop_count];
                    printf ("Data sent: 0x%02x\n\r", txBuffer->msgData[loop_count]);
                }
                
                if (CAN3_MessageTransmit (txBuffer->msgSID, 8, txBuffer->msgData, 0, CAN_MSG_TX_DATA_FRAME) == true)
                {
                    appData.state = APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE;
                    printf ("Wait for Reset command transfer...\n\r");
                }
                else
                {
                    appData.state = APP_STATE_ERROR;
                    printf ("/*** Reset command transfer failed ***/\n\n\r");
                }
                break;
                
            case APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE:
            case APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE:
            case APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE:
            case APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE:
                APP_CheckBTLResponse ();
                if (appData.transferStatus == APP_TRANSFER_STATUS_SUCCESS)
                {
                    
                    if (appData.state == APP_STATE_WAIT_UNLOCK_COMMAND_TRANSFER_COMPLETE)
                    {
                        appData.nBytesWritten += appData.nBytesSent;
                        printf ("/*** Unlock command transfer completed ***/\n\n\r");
                        
                        if (appData.nBytesWritten < CAN_DATA_FRAME_SIZE)
                            appData.state = APP_STATE_SEND_UNLOCK_COMMAND;
                        else
                        {
                            appData.state = APP_STATE_SEND_DATA_COMMAND;
                            appData.nBytesWritten = 0;
                        }
                    }
                    
                    else if (appData.state == APP_STATE_WAIT_DATA_COMMAND_TRANSFER_COMPLETE)
                    {
                        appData.nBytesWritten += appData.nBytesSent;
                        
                        printf ("/*** Data command transfer completed ***/\n\n\r");
                        if (appData.nBytesWritten < APP_IMAGE_SIZE)
                            appData.state = APP_STATE_SEND_DATA_COMMAND;
                        else
                            appData.state = APP_STATE_SEND_VERIFY_COMMAND;                                    
                    }
                    
                    else if (appData.state == APP_STATE_WAIT_VERIFY_COMMAND_TRANSFER_COMPLETE)
                    {                        
                        printf ("/*** Verify command transfer completed ***/\n\n\r");
                        appData.state = APP_STATE_SEND_RESET_COMMAND;
                    }
                    else if (appData.state == APP_STATE_WAIT_RESET_COMMAND_TRANSFER_COMPLETE)
                    {                        
                        printf ("/*** Reset command transfer completed ***/\n\n\r");
                        appData.state = APP_STATE_SUCCESSFUL;
                    }
                }
                
                else if (appData.transferStatus == APP_TRANSFER_STATUS_ERROR)
                    appData.state = APP_STATE_ERROR;
                break;
               
            case APP_STATE_SUCCESSFUL:
                LED3_ON ();
                appData.state = APP_STATE_IDLE;
                printf ("/*** Successful ***/\n\n\r");
                break;
                
            case APP_STATE_ERROR:
                LED_OFF();
                LED2_ON ();
                appData.state = APP_STATE_IDLE;
                printf ("/*** Failure ***/\n\n\r");
                break;

            case APP_STATE_IDLE:
                break;

            default:
                break;    
        }
    }
}

