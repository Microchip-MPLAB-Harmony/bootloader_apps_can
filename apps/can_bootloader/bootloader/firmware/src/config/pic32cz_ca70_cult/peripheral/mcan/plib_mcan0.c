/*******************************************************************************
  Controller Area Network (MCAN) Peripheral Library Source File

  Company:
    Microchip Technology Inc.

  File Name:
    plib_mcan0.c

  Summary:
    MCAN peripheral library interface.

  Description:
    This file defines the interface to the MCAN peripheral library. This
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
#include "plib_mcan0.h"

// *****************************************************************************
// *****************************************************************************
// Global Data
// *****************************************************************************
// *****************************************************************************
#define MCAN_STD_ID_Msk        0x7FFU

static MCAN_OBJ mcan0Obj;

static const mcan_sidfe_registers_t mcan0StdFilter[] =
{
    {
        .MCAN_SIDFE_0 = MCAN_SIDFE_0_SFT(0UL) |
                  MCAN_SIDFE_0_SFID1(0x45aUL) |
                  MCAN_SIDFE_0_SFID2(0x45aUL) |
                  MCAN_SIDFE_0_SFEC(1UL)
    },
};

// *****************************************************************************
// *****************************************************************************
// MCAN0 PLib Interface Routines
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
/* Function:
    void MCAN0_Initialize(void)

   Summary:
    Initializes given instance of the MCAN peripheral.

   Precondition:
    None.

   Parameters:
    None.

   Returns:
    None
*/
void MCAN0_Initialize(void)
{
    /* Start MCAN initialization */
    MCAN0_REGS->MCAN_CCCR = MCAN_CCCR_INIT_Msk;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) != MCAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }

    /* Set CCE to unlock the configuration registers */
    MCAN0_REGS->MCAN_CCCR |= MCAN_CCCR_CCE_Msk;

    /* Set Data Bit Timing and Prescaler Register */
    MCAN0_REGS->MCAN_DBTP = MCAN_DBTP_DTSEG2(6) | MCAN_DBTP_DTSEG1(16) | MCAN_DBTP_DBRP(1) | MCAN_DBTP_DSJW(6);

    /* Set Nominal Bit timing and Prescaler Register */
    MCAN0_REGS->MCAN_NBTP  = MCAN_NBTP_NTSEG2(6) | MCAN_NBTP_NTSEG1(16) | MCAN_NBTP_NBRP(11) | MCAN_NBTP_NSJW(6);

    /* Receive Buffer / FIFO Element Size Configuration Register */
    MCAN0_REGS->MCAN_RXESC = 0UL  | MCAN_RXESC_F0DS(7UL);
    /* Transmit Buffer/FIFO Element Size Configuration Register */
    MCAN0_REGS->MCAN_TXESC = MCAN_TXESC_TBDS(0UL);

    /* Global Filter Configuration Register */
    MCAN0_REGS->MCAN_GFC = MCAN_GFC_ANFS(2) | MCAN_GFC_ANFE(2);

    /* Set the operation mode */
    MCAN0_REGS->MCAN_CCCR |= MCAN_CCCR_FDOE_ENABLED | MCAN_CCCR_BRSE_ENABLED;


    MCAN0_REGS->MCAN_CCCR &= ~MCAN_CCCR_INIT_Msk;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) == MCAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }

    (void) memset(&mcan0Obj.msgRAMConfig, 0x00, sizeof(MCAN_MSG_RAM_CONFIG));
}


// *****************************************************************************
/* Function:
    bool MCAN0_MessageTransmitFifo(uint8_t numberOfMessage, MCAN_TX_BUFFER *txBuffer)

   Summary:
    Transmit multiple messages into MCAN bus from Tx FIFO.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    numberOfMessage - Total number of message.
    txBuffer        - Pointer to Tx buffer

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool MCAN0_MessageTransmitFifo(uint8_t numberOfMessage, MCAN_TX_BUFFER *txBuffer)
{
    uint8_t  *txFifo = NULL;
    uint8_t  *txBuf = (uint8_t *)txBuffer;
    uint32_t bufferNumber = 0U;
    uint8_t  tfqpi = 0U;
    uint8_t  count = 0U;

    if (((numberOfMessage < 1U) || (numberOfMessage > 1U)) || (txBuffer == NULL))
    {
        return false;
    }

    tfqpi = (uint8_t)((MCAN0_REGS->MCAN_TXFQS & MCAN_TXFQS_TFQPI_Msk) >> MCAN_TXFQS_TFQPI_Pos);

    for (count = 0U; count < numberOfMessage; count++)
    {
        txFifo = (uint8_t *)((uint8_t*)mcan0Obj.msgRAMConfig.txBuffersAddress + ((uint32_t)tfqpi * MCAN0_TX_FIFO_BUFFER_ELEMENT_SIZE));

        (void) memcpy(txFifo, txBuf, MCAN0_TX_FIFO_BUFFER_ELEMENT_SIZE);

        txBuf += MCAN0_TX_FIFO_BUFFER_ELEMENT_SIZE;
        bufferNumber |= (1UL << tfqpi);
        tfqpi++;
        if (tfqpi == 1U)
        {
            tfqpi = 0U;
        }
    }

    __DSB();

    /* Set Transmission request */
    MCAN0_REGS->MCAN_TXBAR = bufferNumber;

    return true;
}

// *****************************************************************************
/* Function:
    uint8_t MCAN0_TxFifoFreeLevelGet(void)

   Summary:
    Returns Tx FIFO Free Level.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    None.

   Returns:
    Tx FIFO Free Level.
*/
uint8_t MCAN0_TxFifoFreeLevelGet(void)
{
    return (uint8_t)(MCAN0_REGS->MCAN_TXFQS & MCAN_TXFQS_TFFL_Msk);
}

// *****************************************************************************
/* Function:
    bool MCAN0_TxBufferIsBusy(uint8_t bufferNumber)

   Summary:
    Check if Transmission request is pending for the specific Tx buffer.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    None.

   Returns:
    true  - Transmission request is pending.
    false - Transmission request is not pending.
*/
bool MCAN0_TxBufferIsBusy(uint8_t bufferNumber)
{
    return ((MCAN0_REGS->MCAN_TXBRP & (1UL << bufferNumber)) != 0U);
}

// *****************************************************************************
/* Function:
    bool MCAN0_TxEventFifoRead(uint8_t numberOfTxEvent, MCAN_TX_EVENT_FIFO *txEventFifo)

   Summary:
    Read Tx Event FIFO for the transmitted messages.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    numberOfTxEvent - Total number of Tx Event
    txEventFifo     - Pointer to Tx Event FIFO

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool MCAN0_TxEventFifoRead(uint8_t numberOfTxEvent, MCAN_TX_EVENT_FIFO *txEventFifo)
{
    uint8_t txefgi     = 0U;
    uint8_t count      = 0U;
    uint8_t *txEvent   = NULL;
    uint8_t *txEvtFifo = (uint8_t *)txEventFifo;

    if (txEventFifo == NULL)
    {
        return false;
    }

    /* Read data from the Rx FIFO0 */
    txefgi = (uint8_t)((MCAN0_REGS->MCAN_TXEFS & MCAN_TXEFS_EFGI_Msk) >> MCAN_TXEFS_EFGI_Pos);
    for (count = 0U; count < numberOfTxEvent; count++)
    {
        txEvent = (uint8_t *) ((uint8_t *)mcan0Obj.msgRAMConfig.txEventFIFOAddress + ((uint32_t)txefgi * sizeof(MCAN_TX_EVENT_FIFO)));

        (void) memcpy(txEvtFifo, txEvent, sizeof(MCAN_TX_EVENT_FIFO));

        if ((count + 1U) == numberOfTxEvent)
        {
            break;
        }
        txEvtFifo += sizeof(MCAN_TX_EVENT_FIFO);
        txefgi++;
        if (txefgi == 1U)
        {
            txefgi = 0U;
        }
    }

    /* Ack the Tx Event FIFO position */
    MCAN0_REGS->MCAN_TXEFA = MCAN_TXEFA_EFAI((uint32_t)txefgi);

    return true;
}

// *****************************************************************************
/* Function:
    uint8_t MCAN0_TxEventFifoFillLevelGet(void)

   Summary:
    Returns Tx Event FIFO Fill Level.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    None.

   Returns:
    Tx Event FIFO Fill Level.
*/
uint8_t MCAN0_TxEventFifoFillLevelGet(void)
{
    return (uint8_t)(MCAN0_REGS->MCAN_TXEFS & MCAN_TXEFS_EFFL_Msk);
}


// *****************************************************************************
/* Function:
    bool MCAN0_MessageReceiveFifo(MCAN_RX_FIFO_NUM rxFifoNum, uint8_t numberOfMessage, MCAN_RX_BUFFER *rxBuffer)

   Summary:
    Read messages from Rx FIFO0/FIFO1.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    rxFifoNum       - Rx FIFO number
    numberOfMessage - Total number of message
    rxBuffer        - Pointer to Rx buffer

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool MCAN0_MessageReceiveFifo(MCAN_RX_FIFO_NUM rxFifoNum, uint8_t numberOfMessage, MCAN_RX_BUFFER *rxBuffer)
{
    uint8_t rxgi = 0U;
    uint8_t count = 0U;
    uint8_t *rxFifo = NULL;
    uint8_t *rxBuf = (uint8_t *)rxBuffer;
    bool status = false;

    if (rxBuffer == NULL)
    {
        return status;
    }

    switch (rxFifoNum)
    {
        case MCAN_RX_FIFO_0:
            /* Read data from the Rx FIFO0 */
            rxgi = (uint8_t)((MCAN0_REGS->MCAN_RXF0S & MCAN_RXF0S_F0GI_Msk) >> MCAN_RXF0S_F0GI_Pos);
            for (count = 0U; count < numberOfMessage; count++)
            {
                rxFifo = (uint8_t *) ((uint8_t *)mcan0Obj.msgRAMConfig.rxFIFO0Address + ((uint32_t)rxgi * MCAN0_RX_FIFO0_ELEMENT_SIZE));

                (void) memcpy(rxBuf, rxFifo, MCAN0_RX_FIFO0_ELEMENT_SIZE);

                if ((count + 1U) == numberOfMessage)
                {
                    break;
                }
                rxBuf += MCAN0_RX_FIFO0_ELEMENT_SIZE;
                rxgi++;
                if (rxgi == 5U)
                {
                    rxgi = 0U;
                }
            }

            /* Ack the fifo position */
            MCAN0_REGS->MCAN_RXF0A = MCAN_RXF0A_F0AI((uint32_t)rxgi);

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
    uint8_t MCAN0_RxFifoFillLevelGet(MCAN_RX_FIFO_NUM rxFifoNum)

   Summary:
    Returns Rx FIFO0/FIFO1 Fill Level.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    None.

   Returns:
    Rx FIFO0/FIFO1 Fill Level.
*/
uint8_t MCAN0_RxFifoFillLevelGet(MCAN_RX_FIFO_NUM rxFifoNum)
{
    if (rxFifoNum == MCAN_RX_FIFO_0)
    {
        return (uint8_t)(MCAN0_REGS->MCAN_RXF0S & MCAN_RXF0S_F0FL_Msk);
    }
    else
    {
        return (uint8_t)(MCAN0_REGS->MCAN_RXF1S & MCAN_RXF1S_F1FL_Msk);
    }
}

// *****************************************************************************
/* Function:
    MCAN_ERROR MCAN0_ErrorGet(void)

   Summary:
    Returns the error during transfer.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    None.

   Returns:
    Error during transfer.
*/
MCAN_ERROR MCAN0_ErrorGet(void)
{
    MCAN_ERROR error;
    uint32_t   errorStatus = MCAN0_REGS->MCAN_PSR;

    error = (MCAN_ERROR) ((errorStatus & MCAN_PSR_LEC_Msk) | (errorStatus & MCAN_PSR_EP_Msk) | (errorStatus & MCAN_PSR_EW_Msk)
            | (errorStatus & MCAN_PSR_BO_Msk) | (errorStatus & MCAN_PSR_DLEC_Msk) | (errorStatus & MCAN_PSR_PXE_Msk));

    if ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) == MCAN_CCCR_INIT_Msk)
    {
        MCAN0_REGS->MCAN_CCCR &= ~MCAN_CCCR_INIT_Msk;
        while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) == MCAN_CCCR_INIT_Msk)
        {
            /* Wait for initialization complete */
        }
    }

    return error;
}

// *****************************************************************************
/* Function:
    void MCAN0_ErrorCountGet(uint8_t *txErrorCount, uint8_t *rxErrorCount)

   Summary:
    Returns the transmit and receive error count during transfer.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    txErrorCount - Transmit Error Count to be received
    rxErrorCount - Receive Error Count to be received

   Returns:
    None.
*/
void MCAN0_ErrorCountGet(uint8_t *txErrorCount, uint8_t *rxErrorCount)
{
    *txErrorCount = (uint8_t)(MCAN0_REGS->MCAN_ECR & MCAN_ECR_TEC_Msk);
    *rxErrorCount = (uint8_t)((MCAN0_REGS->MCAN_ECR & MCAN_ECR_REC_Msk) >> MCAN_ECR_REC_Pos);
}

// *****************************************************************************
/* Function:
    bool MCAN0_InterruptGet(MCAN_INTERRUPT_MASK interruptMask)

   Summary:
    Returns the Interrupt status.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    interruptMask - Interrupt source number

   Returns:
    true - Requested interrupt is occurred.
    false - Requested interrupt is not occurred.
*/
bool MCAN0_InterruptGet(MCAN_INTERRUPT_MASK interruptMask)
{
    return ((MCAN0_REGS->MCAN_IR & (uint32_t)interruptMask) != 0x0U);
}

// *****************************************************************************
/* Function:
    void MCAN0_InterruptClear(MCAN_INTERRUPT_MASK interruptMask)

   Summary:
    Clears Interrupt status.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    interruptMask - Interrupt to be cleared

   Returns:
    None
*/
void MCAN0_InterruptClear(MCAN_INTERRUPT_MASK interruptMask)
{
    MCAN0_REGS->MCAN_IR = (uint32_t)interruptMask;
}

// *****************************************************************************
/* Function:
    void MCAN0_MessageRAMConfigSet(uint8_t *msgRAMConfigBaseAddress)

   Summary:
    Set the Message RAM Configuration.

   Precondition:
    MCAN0_Initialize must have been called for the associated MCAN instance.

   Parameters:
    msgRAMConfigBaseAddress - Pointer to application allocated buffer base address.
                              Application must allocate buffer from non-cached
                              contiguous memory and buffer size must be
                              MCAN0_MESSAGE_RAM_CONFIG_SIZE

   Returns:
    None
*/
void MCAN0_MessageRAMConfigSet(uint8_t *msgRAMConfigBaseAddress)
{
    uint32_t offset = 0U;
    uint32_t msgRAMConfigBaseAddr = (uint32_t)msgRAMConfigBaseAddress;

    (void) memset(msgRAMConfigBaseAddress, 0x00, MCAN0_MESSAGE_RAM_CONFIG_SIZE);

    /* Set MCAN CCCR Init for Message RAM Configuration */
    MCAN0_REGS->MCAN_CCCR |= MCAN_CCCR_INIT_ENABLED;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) != MCAN_CCCR_INIT_Msk)
    {
        /* Wait for configuration complete */
    }

    /* Set CCE to unlock the configuration registers */
    MCAN0_REGS->MCAN_CCCR |= MCAN_CCCR_CCE_Msk;

    mcan0Obj.msgRAMConfig.rxFIFO0Address = (mcan_rxf0e_registers_t *)msgRAMConfigBaseAddr;
    offset = MCAN0_RX_FIFO0_SIZE;
    /* Receive FIFO 0 Configuration Register */
    MCAN0_REGS->MCAN_RXF0C = MCAN_RXF0C_F0S(5UL) | MCAN_RXF0C_F0WM(0UL) | MCAN_RXF0C_F0OM_Msk |
            MCAN_RXF0C_F0SA(((uint32_t)mcan0Obj.msgRAMConfig.rxFIFO0Address >> 2));

    mcan0Obj.msgRAMConfig.txBuffersAddress = (mcan_txbe_registers_t *)(msgRAMConfigBaseAddr + offset);
    offset += MCAN0_TX_FIFO_BUFFER_SIZE;
    /* Transmit Buffer/FIFO Configuration Register */
    MCAN0_REGS->MCAN_TXBC = MCAN_TXBC_TFQS(1UL) |
            MCAN_TXBC_TBSA(((uint32_t)mcan0Obj.msgRAMConfig.txBuffersAddress >> 2));

    mcan0Obj.msgRAMConfig.txEventFIFOAddress =  (mcan_txefe_registers_t *)(msgRAMConfigBaseAddr + offset);
    offset += MCAN0_TX_EVENT_FIFO_SIZE;
    /* Transmit Event FIFO Configuration Register */
    MCAN0_REGS->MCAN_TXEFC = MCAN_TXEFC_EFWM(0UL) | MCAN_TXEFC_EFS(1UL) |
            MCAN_TXEFC_EFSA(((uint32_t)mcan0Obj.msgRAMConfig.txEventFIFOAddress >> 2));

    mcan0Obj.msgRAMConfig.stdMsgIDFilterAddress = (mcan_sidfe_registers_t *)(msgRAMConfigBaseAddr + offset);
    (void) memcpy((void*)mcan0Obj.msgRAMConfig.stdMsgIDFilterAddress,
           (const void*)mcan0StdFilter,
           MCAN0_STD_MSG_ID_FILTER_SIZE);
    offset += MCAN0_STD_MSG_ID_FILTER_SIZE;
    /* Standard ID Filter Configuration Register */
    MCAN0_REGS->MCAN_SIDFC = MCAN_SIDFC_LSS(1UL) |
            MCAN_SIDFC_FLSSA(((uint32_t)mcan0Obj.msgRAMConfig.stdMsgIDFilterAddress >> 2));

    /* Set 16-bit MSB of mcan0 base address */
    MATRIX_REGS->CCFG_CAN0 = (MATRIX_REGS->CCFG_CAN0 & ~CCFG_CAN0_Msk)
                            | CCFG_CAN0_CAN0DMABA(((uint32_t)msgRAMConfigBaseAddr >> 16));

    /* Reference offset variable once to remove warning about the variable not being used after increment */
    (void)offset;

    /* Complete Message RAM Configuration by clearing MCAN CCCR Init */
    MCAN0_REGS->MCAN_CCCR &= ~MCAN_CCCR_INIT_Msk;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) == MCAN_CCCR_INIT_Msk)
    {
        /* Wait for configuration complete */
    }
}

// *****************************************************************************
/* Function:
    bool MCAN0_StandardFilterElementSet(uint8_t filterNumber, mcan_sidfe_registers_t *stdMsgIDFilterElement)

   Summary:
    Set a standard filter element configuration.

   Precondition:
    MCAN0_Initialize and MCAN0_MessageRAMConfigSet must have been called
    for the associated MCAN instance.

   Parameters:
    filterNumber          - Standard Filter number to be configured.
    stdMsgIDFilterElement - Pointer to Standard Filter Element configuration to be set on specific filterNumber.

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool MCAN0_StandardFilterElementSet(uint8_t filterNumber, mcan_sidfe_registers_t *stdMsgIDFilterElement)
{
    if ((filterNumber > 1U) || (stdMsgIDFilterElement == NULL))
    {
        return false;
    }
    mcan0Obj.msgRAMConfig.stdMsgIDFilterAddress[filterNumber - 1U].MCAN_SIDFE_0 = stdMsgIDFilterElement->MCAN_SIDFE_0;

    return true;
}

// *****************************************************************************
/* Function:
    bool MCAN0_StandardFilterElementGet(uint8_t filterNumber, mcan_sidfe_registers_t *stdMsgIDFilterElement)

   Summary:
    Get a standard filter element configuration.

   Precondition:
    MCAN0_Initialize and MCAN0_MessageRAMConfigSet must have been called
    for the associated MCAN instance.

   Parameters:
    filterNumber          - Standard Filter number to get filter configuration.
    stdMsgIDFilterElement - Pointer to Standard Filter Element configuration for storing filter configuration.

   Returns:
    Request status.
    true  - Request was successful.
    false - Request has failed.
*/
bool MCAN0_StandardFilterElementGet(uint8_t filterNumber, mcan_sidfe_registers_t *stdMsgIDFilterElement)
{
    if ((filterNumber > 1U) || (stdMsgIDFilterElement == NULL))
    {
        return false;
    }
    stdMsgIDFilterElement->MCAN_SIDFE_0 = mcan0Obj.msgRAMConfig.stdMsgIDFilterAddress[filterNumber - 1U].MCAN_SIDFE_0;

    return true;
}


void MCAN0_SleepModeEnter(void)
{
    MCAN0_REGS->MCAN_CCCR |=  MCAN_CCCR_CSR_Msk;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_CSA_Msk) != MCAN_CCCR_CSA_Msk)
    {
        /* Wait for clock stop request to complete */
    }
}

void MCAN0_SleepModeExit(void)
{
    MCAN0_REGS->MCAN_CCCR &=  ~MCAN_CCCR_CSR_Msk;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_CSA_Msk) == MCAN_CCCR_CSA_Msk)
    {
        /* Wait for no clock stop */
    }
    MCAN0_REGS->MCAN_CCCR &= ~MCAN_CCCR_INIT_Msk;
    while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) == MCAN_CCCR_INIT_Msk)
    {
        /* Wait for initialization complete */
    }
}

bool MCAN0_BitTimingCalculationGet(MCAN_BIT_TIMING_SETUP *setup, MCAN_BIT_TIMING *bitTiming)
{
    bool status = false;
    uint32_t numOfTimeQuanta;
    uint8_t tseg1;
    float temp1;
    float temp2;

    if ((setup != NULL) && (bitTiming != NULL))
    {
        if (setup->nominalBitTimingSet == true)
        {
            numOfTimeQuanta = MCAN0_CLOCK_FREQUENCY / (setup->nominalBitRate * ((uint32_t)setup->nominalPrescaler + 1U));
            if ((numOfTimeQuanta >= 4U) && (numOfTimeQuanta <= 385U))
            {
                if (setup->nominalSamplePoint < 50.0f)
                {
                    setup->nominalSamplePoint = 50.0f;
                }
                temp1 = (float)numOfTimeQuanta;
                temp2 = (temp1 * setup->nominalSamplePoint) / 100.0f;
                tseg1 = (uint8_t)temp2;
                bitTiming->nominalBitTiming.nominalTimeSegment2 = (uint8_t)(numOfTimeQuanta - tseg1 - 1U);
                bitTiming->nominalBitTiming.nominalTimeSegment1 = tseg1 - 2U;
                bitTiming->nominalBitTiming.nominalSJW = bitTiming->nominalBitTiming.nominalTimeSegment2;
                bitTiming->nominalBitTiming.nominalPrescaler = setup->nominalPrescaler;
                bitTiming->nominalBitTimingSet = true;
                status = true;
            }
            else
            {
                bitTiming->nominalBitTimingSet = false;
            }
        }
        if (setup->dataBitTimingSet == true)
        {
            numOfTimeQuanta = MCAN0_CLOCK_FREQUENCY / (setup->dataBitRate * ((uint32_t)setup->dataPrescaler + 1U));
            if ((numOfTimeQuanta >= 4U) && (numOfTimeQuanta <= 49U))
            {
                if (setup->dataSamplePoint < 50.0f)
                {
                    setup->dataSamplePoint = 50.0f;
                }
                temp1 = (float)numOfTimeQuanta;
                temp2 = (temp1 * setup->dataSamplePoint) / 100.0f;
                tseg1 = (uint8_t)temp2;
                bitTiming->dataBitTiming.dataTimeSegment2 = (uint8_t)(numOfTimeQuanta - tseg1 - 1U);
                bitTiming->dataBitTiming.dataTimeSegment1 = tseg1 - 2U;
                bitTiming->dataBitTiming.dataSJW = bitTiming->dataBitTiming.dataTimeSegment2;
                bitTiming->dataBitTiming.dataPrescaler = setup->dataPrescaler;
                bitTiming->dataBitTimingSet = true;
                status = true;
            }
            else
            {
                bitTiming->dataBitTimingSet = false;
                status = false;
            }
        }
    }

    return status;
}

bool MCAN0_BitTimingSet(MCAN_BIT_TIMING *bitTiming)
{
    bool status = false;
    bool nominalBitTimingSet = false;
    bool dataBitTimingSet = false;

    if ((bitTiming->nominalBitTimingSet == true)
    && (bitTiming->nominalBitTiming.nominalTimeSegment1 >= 0x1U)
    && (bitTiming->nominalBitTiming.nominalTimeSegment2 <= 0x7FU)
    && (bitTiming->nominalBitTiming.nominalPrescaler <= 0x1FFU)
    && (bitTiming->nominalBitTiming.nominalSJW <= 0x7FU))
    {
        nominalBitTimingSet = true;
    }

    if  ((bitTiming->dataBitTimingSet == true)
    &&  ((bitTiming->dataBitTiming.dataTimeSegment1 >= 0x1U) && (bitTiming->dataBitTiming.dataTimeSegment1 <= 0x1FU))
    &&  (bitTiming->dataBitTiming.dataTimeSegment2 <= 0xFU)
    &&  (bitTiming->dataBitTiming.dataPrescaler <= 0x1FU)
    &&  (bitTiming->dataBitTiming.dataSJW <= 0xFU))
    {
        dataBitTimingSet = true;
    }

    if ((nominalBitTimingSet == true) || (dataBitTimingSet == true))
    {
        /* Start MCAN initialization */
        MCAN0_REGS->MCAN_CCCR = MCAN_CCCR_INIT_Msk;
        while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) != MCAN_CCCR_INIT_Msk)
        {
            /* Wait for initialization complete */
        }

        /* Set CCE to unlock the configuration registers */
        MCAN0_REGS->MCAN_CCCR |= MCAN_CCCR_CCE_Msk;

        if (dataBitTimingSet == true)
        {
            /* Set Data Bit Timing and Prescaler Register */
            MCAN0_REGS->MCAN_DBTP = MCAN_DBTP_DTSEG2(bitTiming->dataBitTiming.dataTimeSegment2) | MCAN_DBTP_DTSEG1(bitTiming->dataBitTiming.dataTimeSegment1) | MCAN_DBTP_DBRP(bitTiming->dataBitTiming.dataPrescaler) | MCAN_DBTP_DSJW(bitTiming->dataBitTiming.dataSJW);

        }

        if (nominalBitTimingSet == true)
        {
            /* Set Nominal Bit timing and Prescaler Register */
            MCAN0_REGS->MCAN_NBTP  = MCAN_NBTP_NTSEG2(bitTiming->nominalBitTiming.nominalTimeSegment2) | MCAN_NBTP_NTSEG1(bitTiming->nominalBitTiming.nominalTimeSegment1) | MCAN_NBTP_NBRP(bitTiming->nominalBitTiming.nominalPrescaler) | MCAN_NBTP_NSJW(bitTiming->nominalBitTiming.nominalSJW);
        }

        /* Set the operation mode */
        MCAN0_REGS->MCAN_CCCR |= MCAN_CCCR_FDOE_ENABLED | MCAN_CCCR_BRSE_ENABLED;


        MCAN0_REGS->MCAN_CCCR &= ~MCAN_CCCR_INIT_Msk;
        while ((MCAN0_REGS->MCAN_CCCR & MCAN_CCCR_INIT_Msk) == MCAN_CCCR_INIT_Msk)
        {
            /* Wait for initialization complete */
        }
        status = true;
    }
    return status;
}

/*******************************************************************************
 End of File
*/
