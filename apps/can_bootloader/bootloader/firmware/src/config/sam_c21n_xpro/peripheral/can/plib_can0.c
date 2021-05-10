/*******************************************************************************
  Controller Area Network (CAN) Peripheral Library Source File

  Company:
    Microchip Technology Inc.

  File Name:
    plib_can0.c

  Summary:
    CAN peripheral library interface.

  Description:
    This file defines the interface to the CAN peripheral library. This
    library provides access to and control of the associated peripheral
    instance.

  Remarks:
    None.
*******************************************************************************/

//DOM-IGNORE-BEGIN
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
//DOM-IGNORE-END
// *****************************************************************************
// *****************************************************************************
// Header Includes
// *****************************************************************************
// *****************************************************************************

#include "device.h"
#include "plib_can0.h"

// *****************************************************************************
// *****************************************************************************
// Global Data
// *****************************************************************************
// *****************************************************************************
#define CAN_STD_ID_Msk        0x7FFU
static CAN_OBJ can0Obj;

static const can_sidfe_registers_t can0StdFilter[] =
{
    {
        .CAN_SIDFE_0 = CAN_SIDFE_0_SFT(0UL) |
                  CAN_SIDFE_0_SFID1(0x45aUL) |
                  CAN_SIDFE_0_SFID2(0x45aUL) |
                  CAN_SIDFE_0_SFEC(1UL)
    },
};

/******************************************************************************
Local Functions
******************************************************************************/

static void CANLengthToDlcGet(uint8_t length, uint8_t *dlc)
{
    if (length <= 8U)
    {
        *dlc = length;
    }
    else if (length <= 12U)
    {
        *dlc = 0x9U;
    }
    else if (length <= 16U)
    {
        *dlc = 0xAU;
    }
    else if (length <= 20U)
    {
        *dlc = 0xBU;
    }
    else if (length <= 24U)
    {
        *dlc = 0xCU;
    }
    else if (length <= 32U)
    {
        *dlc = 0xDU;
    }
    else if (length <= 48U)
    {
        *dlc = 0xEU;
    }
    else
    {
        *dlc = 0xFU;
    }
}
static uint8_t CANDlcToLengthGet(uint8_t dlc)
{
    uint8_t msgLength[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
    return msgLength[dlc];
}

// *****************************************************************************
// *****************************************************************************
// CAN0 PLib Interface Routines
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
/* Function:
    void CAN0_Initialize(void)

   Summary:
    Initializes given instance of the CAN peripheral.

   Precondition:
    None.

   Parameters:
    None.

   Returns:
    None
*/
void CAN0_Initialize(void)
{
    /* Start CAN initialization */
    CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }

    /* Set CCE to unlock the configuration registers */
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;

    /* Set Data Bit Timing and Prescaler Register */
    CAN0_REGS->CAN_DBTP = CAN_DBTP_DTSEG2(3UL) | CAN_DBTP_DTSEG1(10UL) | CAN_DBTP_DBRP(0UL) | CAN_DBTP_DSJW(3UL);

    /* Set Nominal Bit timing and Prescaler Register */
    CAN0_REGS->CAN_NBTP  = CAN_NBTP_NTSEG2(3UL) | CAN_NBTP_NTSEG1(10UL) | CAN_NBTP_NBRP(5UL) | CAN_NBTP_NSJW(3UL);

    /* Receive Buffer / FIFO Element Size Configuration Register */
    CAN0_REGS->CAN_RXESC = 0UL  | CAN_RXESC_F0DS(7UL);
    /* Transmit Buffer/FIFO Element Size Configuration Register */
    CAN0_REGS->CAN_TXESC = CAN_TXESC_TBDS(0UL);

    /* Global Filter Configuration Register */
    CAN0_REGS->CAN_GFC = CAN_GFC_ANFS_REJECT | CAN_GFC_ANFE_REJECT;

    /* Set the operation mode */
    CAN0_REGS->CAN_CCCR = (CAN0_REGS->CAN_CCCR & ~CAN_CCCR_INIT_Msk) | CAN_CCCR_FDOE_Msk | CAN_CCCR_BRSE_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == CAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }
    memset(&can0Obj.msgRAMConfig, 0x00, sizeof(CAN_MSG_RAM_CONFIG));
}

// *****************************************************************************
/* Function:
    bool CAN0_MessageTransmit(uint32_t id, uint8_t length, uint8_t* data, CAN_MODE mode, CAN_MSG_TX_ATTRIBUTE msgAttr)

   Summary:
    Transmits a message into CAN bus.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    id      - 11-bit / 29-bit identifier (ID).
    length  - length of data buffer in number of bytes.
    data    - pointer to source data buffer
    mode    - CAN mode Classic CAN or CAN FD without BRS or CAN FD with BRS
    msgAttr - Data Frame or Remote frame using Tx FIFO or Tx Buffer

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool CAN0_MessageTransmit(uint32_t id, uint8_t length, uint8_t* data, CAN_MODE mode, CAN_MSG_TX_ATTRIBUTE msgAttr)
{
    uint8_t dlc = 0U;
    uint8_t tfqpi = 0U;
    can_txbe_registers_t *fifo = NULL;
    static uint8_t messageMarker = 0U;
    bool op_success = false;

    switch (msgAttr)
    {
        case CAN_MSG_ATTR_TX_FIFO_DATA_FRAME:
        case CAN_MSG_ATTR_TX_FIFO_RTR_FRAME:
            /* The FIFO is not full */
            if (0U == (CAN0_REGS->CAN_TXFQS & CAN_TXFQS_TFQF_Msk))
            {
                tfqpi = (uint8_t)((CAN0_REGS->CAN_TXFQS & CAN_TXFQS_TFQPI_Msk) >> CAN_TXFQS_TFQPI_Pos);
                fifo = (can_txbe_registers_t *) ((uint8_t*)can0Obj.msgRAMConfig.txBuffersAddress + ((uint32_t)tfqpi * CAN0_TX_FIFO_BUFFER_ELEMENT_SIZE));
                op_success = true;
            }
            break;
        default:
            /* Invalid Message Attribute */
            break;
    }
    if (op_success)
    {
        /* If the id is longer than 11 bits, it is considered as extended identifier */
        if (id > CAN_STD_ID_Msk)
        {
            /* An extended identifier is stored into ID */
            fifo->CAN_TXBE_0 = (id & CAN_TXBE_0_ID_Msk) | CAN_TXBE_0_XTD_Msk;
        }
        else
        {
            /* A standard identifier is stored into ID[28:18] */
            fifo->CAN_TXBE_0 = id << 18U;
        }
        if (length > 64U)
        {
            length = 64U;
        }

        CANLengthToDlcGet(length, &dlc);

        fifo->CAN_TXBE_1 = CAN_TXBE_1_DLC((uint32_t)dlc);

        if(mode == CAN_MODE_FD_WITH_BRS)
        {
            fifo->CAN_TXBE_1 |= CAN_TXBE_1_FDF_Msk | CAN_TXBE_1_BRS_Msk;
        }
        else if (mode == CAN_MODE_FD_WITHOUT_BRS)
        {
            fifo->CAN_TXBE_1 |= CAN_TXBE_1_FDF_Msk;
        }
        else
        {
            /* Do nothing */
        }
        if ((msgAttr == CAN_MSG_ATTR_TX_BUFFER_DATA_FRAME) || (msgAttr == CAN_MSG_ATTR_TX_FIFO_DATA_FRAME))
        {
            /* copy the data into the payload */
            memcpy((uint8_t *)&fifo->CAN_TXBE_DATA, data, length);
        }
        else if (msgAttr == CAN_MSG_ATTR_TX_BUFFER_RTR_FRAME || msgAttr == CAN_MSG_ATTR_TX_FIFO_RTR_FRAME)
        {
            fifo->CAN_TXBE_0 |= CAN_TXBE_0_RTR_Msk;
        }
        else
        {
            /* Do nothing */
        }

        messageMarker++;
        fifo->CAN_TXBE_1 |= (((uint32_t)(messageMarker) << CAN_TXBE_1_MM_Pos) & CAN_TXBE_1_MM_Msk) | CAN_TXBE_1_EFC_Msk;

        /* request the transmit */
        CAN0_REGS->CAN_TXBAR = 1UL << tfqpi;

    }
    return op_success;
}

// *****************************************************************************
/* Function:
    bool CAN0_MessageReceive(uint32_t *id, uint8_t *length, uint8_t *data, uint16_t *timestamp,
                                             CAN_MSG_RX_ATTRIBUTE msgAttr, CAN_MSG_RX_FRAME_ATTRIBUTE *msgFrameAttr)

   Summary:
    Receives a message from CAN bus.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    id           - Pointer to 11-bit / 29-bit identifier (ID) to be received.
    length       - Pointer to data length in number of bytes to be received.
    data         - pointer to destination data buffer
    timestamp    - Pointer to Rx message timestamp, timestamp value is 0 if timestamp is disabled
    msgAttr      - Message to be read from Rx FIFO0 or Rx FIFO1 or Rx Buffer
    msgFrameAttr - Data frame or Remote frame to be received

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool CAN0_MessageReceive(uint32_t *id, uint8_t *length, uint8_t *data, uint16_t *timestamp,
                                         CAN_MSG_RX_ATTRIBUTE msgAttr, CAN_MSG_RX_FRAME_ATTRIBUTE *msgFrameAttr)
{
    uint8_t msgLength = 0U;
    uint8_t rxgi = 0U;
    bool testCondition = false;
    can_rxf0e_registers_t *rxf0eFifo = NULL;
    bool status = false;

    switch (msgAttr)
    {
        case CAN_MSG_ATTR_RX_FIFO0:
            /* Check and return false if nothing to be read */
            if ((CAN0_REGS->CAN_RXF0S & CAN_RXF0S_F0FL_Msk) == 0U)
            {
                return status;
            }
            /* Read data from the Rx FIFO0 */
            rxgi = (uint8_t)((CAN0_REGS->CAN_RXF0S & CAN_RXF0S_F0GI_Msk) >> CAN_RXF0S_F0GI_Pos);
            rxf0eFifo = (can_rxf0e_registers_t *) ((uint8_t *)can0Obj.msgRAMConfig.rxFIFO0Address + ((uint32_t)rxgi * CAN0_RX_FIFO0_ELEMENT_SIZE));

            /* Get received identifier */
            if ((rxf0eFifo->CAN_RXF0E_0 & CAN_RXF0E_0_XTD_Msk) != 0U)
            {
                *id = rxf0eFifo->CAN_RXF0E_0 & CAN_RXF0E_0_ID_Msk;
            }
            else
            {
                *id = (rxf0eFifo->CAN_RXF0E_0 >> 18) & CAN_STD_ID_Msk;
            }

            /* Check RTR and FDF bits for Remote/Data Frame */
            testCondition = ((rxf0eFifo->CAN_RXF0E_0 & CAN_RXF0E_0_RTR_Msk) != 0U);
            testCondition = ((rxf0eFifo->CAN_RXF0E_1 & CAN_RXF0E_1_FDF_Msk) == 0U) && testCondition;
            if (testCondition)
            {
                *msgFrameAttr = CAN_MSG_RX_REMOTE_FRAME;
            }
            else
            {
                *msgFrameAttr = CAN_MSG_RX_DATA_FRAME;
            }

            /* Get received data length */
            msgLength = CANDlcToLengthGet((uint8_t)((rxf0eFifo->CAN_RXF0E_1 & CAN_RXF0E_1_DLC_Msk) >> CAN_RXF0E_1_DLC_Pos));

            /* Copy data to user buffer */
            memcpy(data, (uint8_t *)&rxf0eFifo->CAN_RXF0E_DATA, msgLength);
            *length = msgLength;

            /* Ack the fifo position */
            CAN0_REGS->CAN_RXF0A = CAN_RXF0A_F0AI((uint32_t)rxgi);
            status = true;
            break;
        default:
            /* Do nothing */
            break;
    }
    return status;
}

// *****************************************************************************
/* Function:
    bool CAN0_TransmitEventFIFOElementGet(uint32_t *id, uint8_t *messageMarker, uint16_t *timestamp)

   Summary:
    Get the Transmit Event FIFO Element for the transmitted message.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    id            - Pointer to 11-bit / 29-bit identifier (ID) to be received.
    messageMarker - Pointer to Tx message message marker number to be received
    timestamp     - Pointer to Tx message timestamp to be received, timestamp value is 0 if Timestamp is disabled

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool CAN0_TransmitEventFIFOElementGet(uint32_t *id, uint8_t *messageMarker, uint16_t *timestamp)
{
    can_txefe_registers_t *txEventFIFOElement = NULL;
    uint8_t txefgi = 0U;
    bool status = false;

    /* Check if Tx Event FIFO Element available */
    if ((CAN0_REGS->CAN_TXEFS & CAN_TXEFS_EFFL_Msk) != 0U)
    {
        /* Get a pointer to Tx Event FIFO Element */
        txefgi = (uint8_t)((CAN0_REGS->CAN_TXEFS & CAN_TXEFS_EFGI_Msk) >> CAN_TXEFS_EFGI_Pos);
        txEventFIFOElement = (can_txefe_registers_t *) ((uint8_t *)can0Obj.msgRAMConfig.txEventFIFOAddress + ((uint32_t)txefgi * sizeof(can_txefe_registers_t)));

        /* Check if it's a extended message type */
        if (0U != (txEventFIFOElement->CAN_TXEFE_0 & CAN_TXEFE_0_XTD_Msk))
        {
            *id = txEventFIFOElement->CAN_TXEFE_0 & CAN_TXEFE_0_ID_Msk;
        }
        else
        {
            *id = (txEventFIFOElement->CAN_TXEFE_0 >> 18U) & CAN_STD_ID_Msk;
        }

        *messageMarker = (uint8_t)((txEventFIFOElement->CAN_TXEFE_1 & CAN_TXEFE_1_MM_Msk) >> CAN_TXEFE_1_MM_Pos);

        /* Get timestamp from transmitted message */
        if (timestamp != NULL)
        {
            *timestamp = (uint16_t)(txEventFIFOElement->CAN_TXEFE_1 & CAN_TXEFE_1_TXTS_Msk);
        }

        /* Ack the Tx Event FIFO position */
        CAN0_REGS->CAN_TXEFA = CAN_TXEFA_EFAI((uint32_t)txefgi);

        /* Tx Event FIFO Element read successfully, so return true */
        status = true;
    }
    return status;
}

// *****************************************************************************
/* Function:
    CAN_ERROR CAN0_ErrorGet(void)

   Summary:
    Returns the error during transfer.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    None.

   Returns:
    Error during transfer.
*/
CAN_ERROR CAN0_ErrorGet(void)
{
    CAN_ERROR error;
    uint32_t   errorStatus = CAN0_REGS->CAN_PSR;

    error = (CAN_ERROR) ((errorStatus & CAN_PSR_LEC_Msk) | (errorStatus & CAN_PSR_EP_Msk) | (errorStatus & CAN_PSR_EW_Msk)
            | (errorStatus & CAN_PSR_BO_Msk) | (errorStatus & CAN_PSR_DLEC_Msk) | (errorStatus & CAN_PSR_PXE_Msk));

    if ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == CAN_CCCR_INIT_Msk)
    {
        CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;
        CAN0_REGS->CAN_CCCR = (CAN0_REGS->CAN_CCCR & ~CAN_CCCR_INIT_Msk) | CAN_CCCR_FDOE_Msk | CAN_CCCR_BRSE_Msk;
        while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == CAN_CCCR_INIT_Msk)
        {
            /* Wait for initialization complete */
        }
    }

    return error;
}

// *****************************************************************************
/* Function:
    void CAN0_ErrorCountGet(uint8_t *txErrorCount, uint8_t *rxErrorCount)

   Summary:
    Returns the transmit and receive error count during transfer.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    txErrorCount - Transmit Error Count to be received
    rxErrorCount - Receive Error Count to be received

   Returns:
    None.
*/
void CAN0_ErrorCountGet(uint8_t *txErrorCount, uint8_t *rxErrorCount)
{
    *txErrorCount = (uint8_t)(CAN0_REGS->CAN_ECR & CAN_ECR_TEC_Msk);
    *rxErrorCount = (uint8_t)((CAN0_REGS->CAN_ECR & CAN_ECR_REC_Msk) >> CAN_ECR_REC_Pos);
}

// *****************************************************************************
/* Function:
    bool CAN0_InterruptGet(CAN_INTERRUPT_MASK interruptMask)

   Summary:
    Returns the Interrupt status.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    interruptMask - Interrupt source number

   Returns:
    true - Requested interrupt is occurred.
    false - Requested interrupt is not occurred.
*/
bool CAN0_InterruptGet(CAN_INTERRUPT_MASK interruptMask)
{
    return ((CAN0_REGS->CAN_IR & (uint32_t)interruptMask) != 0x0U);
}

// *****************************************************************************
/* Function:
    void CAN0_InterruptClear(CAN_INTERRUPT_MASK interruptMask)

   Summary:
    Clears Interrupt status.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    interruptMask - Interrupt to be cleared

   Returns:
    None
*/
void CAN0_InterruptClear(CAN_INTERRUPT_MASK interruptMask)
{
    CAN0_REGS->CAN_IR = (uint32_t)interruptMask;
}

// *****************************************************************************
/* Function:
    bool CAN0_TxFIFOIsFull(void)

   Summary:
    Returns true if Tx FIFO is full otherwise false.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    None

   Returns:
    true  - Tx FIFO is full.
    false - Tx FIFO is not full.
*/
bool CAN0_TxFIFOIsFull(void)
{
    return ((CAN0_REGS->CAN_TXFQS & CAN_TXFQS_TFQF_Msk) == CAN_TXFQS_TFQF_Msk);
}

// *****************************************************************************
/* Function:
    void CAN0_MessageRAMConfigSet(uint8_t *msgRAMConfigBaseAddress)

   Summary:
    Set the Message RAM Configuration.

   Precondition:
    CAN0_Initialize must have been called for the associated CAN instance.

   Parameters:
    msgRAMConfigBaseAddress - Pointer to application allocated buffer base address.
                              Application must allocate buffer from non-cached
                              contiguous memory and buffer size must be
                              CAN0_MESSAGE_RAM_CONFIG_SIZE

   Returns:
    None
*/
void CAN0_MessageRAMConfigSet(uint8_t *msgRAMConfigBaseAddress)
{
    uint32_t offset = 0U;

    memset(msgRAMConfigBaseAddress, 0x00, CAN0_MESSAGE_RAM_CONFIG_SIZE);

    /* Set CAN CCCR Init for Message RAM Configuration */
    CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) != CAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }

    /* Set CCE to unlock the configuration registers */
    CAN0_REGS->CAN_CCCR |= CAN_CCCR_CCE_Msk;

    can0Obj.msgRAMConfig.rxFIFO0Address = (can_rxf0e_registers_t *)msgRAMConfigBaseAddress;
    offset = CAN0_RX_FIFO0_SIZE;
    /* Receive FIFO 0 Configuration Register */
    CAN0_REGS->CAN_RXF0C = CAN_RXF0C_F0S(5UL) | CAN_RXF0C_F0WM(0UL) | CAN_RXF0C_F0OM_Msk |
            CAN_RXF0C_F0SA((uint32_t)can0Obj.msgRAMConfig.rxFIFO0Address);

    can0Obj.msgRAMConfig.txBuffersAddress = (can_txbe_registers_t *)(msgRAMConfigBaseAddress + offset);
    offset += CAN0_TX_FIFO_BUFFER_SIZE;
    /* Transmit Buffer/FIFO Configuration Register */
    CAN0_REGS->CAN_TXBC = CAN_TXBC_TFQS(1UL) |
            CAN_TXBC_TBSA((uint32_t)can0Obj.msgRAMConfig.txBuffersAddress);

    can0Obj.msgRAMConfig.txEventFIFOAddress =  (can_txefe_registers_t *)(msgRAMConfigBaseAddress + offset);
    offset += CAN0_TX_EVENT_FIFO_SIZE;
    /* Transmit Event FIFO Configuration Register */
    CAN0_REGS->CAN_TXEFC = CAN_TXEFC_EFWM(0UL) | CAN_TXEFC_EFS(1UL) |
            CAN_TXEFC_EFSA((uint32_t)can0Obj.msgRAMConfig.txEventFIFOAddress);

    can0Obj.msgRAMConfig.stdMsgIDFilterAddress = (can_sidfe_registers_t *)(msgRAMConfigBaseAddress + offset);
    memcpy(can0Obj.msgRAMConfig.stdMsgIDFilterAddress,
           (const void *)can0StdFilter,
           CAN0_STD_MSG_ID_FILTER_SIZE);
    offset += CAN0_STD_MSG_ID_FILTER_SIZE;
    /* Standard ID Filter Configuration Register */
    CAN0_REGS->CAN_SIDFC = CAN_SIDFC_LSS(1UL) |
            CAN_SIDFC_FLSSA((uint32_t)can0Obj.msgRAMConfig.stdMsgIDFilterAddress);


    /* Reference offset variable once to remove warning about the variable not being used after increment */
    (void)offset;

    /* Complete Message RAM Configuration by clearing CAN CCCR Init */
    CAN0_REGS->CAN_CCCR = (CAN0_REGS->CAN_CCCR & ~CAN_CCCR_INIT_Msk) | CAN_CCCR_FDOE_Msk | CAN_CCCR_BRSE_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == CAN_CCCR_INIT_Msk)
    {
        /* Wait for configuration complete */
    }
}

// *****************************************************************************
/* Function:
    bool CAN0_StandardFilterElementSet(uint8_t filterNumber, can_sidfe_registers_t *stdMsgIDFilterElement)

   Summary:
    Set a standard filter element configuration.

   Precondition:
    CAN0_Initialize and CAN0_MessageRAMConfigSet must have been called
    for the associated CAN instance.

   Parameters:
    filterNumber          - Standard Filter number to be configured.
    stdMsgIDFilterElement - Pointer to Standard Filter Element configuration to be set on specific filterNumber.

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool CAN0_StandardFilterElementSet(uint8_t filterNumber, can_sidfe_registers_t *stdMsgIDFilterElement)
{
    if ((filterNumber > 1U) || (stdMsgIDFilterElement == NULL))
    {
        return false;
    }
    can0Obj.msgRAMConfig.stdMsgIDFilterAddress[filterNumber - 1U].CAN_SIDFE_0 = stdMsgIDFilterElement->CAN_SIDFE_0;

    return true;
}

// *****************************************************************************
/* Function:
    bool CAN0_StandardFilterElementGet(uint8_t filterNumber, can_sidfe_registers_t *stdMsgIDFilterElement)

   Summary:
    Get a standard filter element configuration.

   Precondition:
    CAN0_Initialize and CAN0_MessageRAMConfigSet must have been called
    for the associated CAN instance.

   Parameters:
    filterNumber          - Standard Filter number to get filter configuration.
    stdMsgIDFilterElement - Pointer to Standard Filter Element configuration for storing filter configuration.

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool CAN0_StandardFilterElementGet(uint8_t filterNumber, can_sidfe_registers_t *stdMsgIDFilterElement)
{
    if ((filterNumber > 1U) || (stdMsgIDFilterElement == NULL))
    {
        return false;
    }
    stdMsgIDFilterElement->CAN_SIDFE_0 = can0Obj.msgRAMConfig.stdMsgIDFilterAddress[filterNumber - 1U].CAN_SIDFE_0;

    return true;
}


void CAN0_SleepModeEnter(void)
{
    CAN0_REGS->CAN_CCCR |=  CAN_CCCR_CSR_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_CSA_Msk) != CAN_CCCR_CSA_Msk)
    {
        /* Wait for clock stop request to complete */
    }
}

void CAN0_SleepModeExit(void)
{
    CAN0_REGS->CAN_CCCR &=  ~CAN_CCCR_CSR_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_CSA_Msk) == CAN_CCCR_CSA_Msk)
    {
        /* Wait for no clock stop */
    }
    CAN0_REGS->CAN_CCCR &= ~CAN_CCCR_INIT_Msk;
    while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == CAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }
}



/*******************************************************************************
 End of File
*/
