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
/*******************************************************************************
  User Configuration Header

  File Name:
    user.h

  Summary:
    Build-time configuration header for the user defined by this project.

  Description:
    An MPLAB Project may have multiple configurations.  This file defines the
    build-time options for a single configuration.

  Remarks:
    It only provides macro definitions for build-time configuration options

*******************************************************************************/

#ifndef USER_H
#define USER_H

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: User Configuration macros
// *****************************************************************************
// *****************************************************************************

/* Include the Hex header file of the application to be programmed by the 
 * CAN host bootloader application. 
 * Specify the start address of the application being programmed.
 * Specify the user application start address. The application start address must
 * be aligned to erase page unit. If the bootloader itself is being upgraded then 
 * the APP_IMAGE_START_ADDR must be set to 0x00 (start of bootloader). Ensure the
 * bootloader and application is also configured with the same value of application 
 * start address. 
 */
    
#define SAM_C21N_XPRO         1
#define SAM_E54_XPRO          2
#define SAM_E70_XULT          3
#define SAM_V71_XULT          4
#define PIC32CZ_CA80_CULT     5

/* Select the device being upgraded by the CAN bootloader host. 
 * This macro takes one of the following values: 
 * SAM_C21N_XPRO, SAM_E54_XPRO, SAM_E70_XULT, SAM_V71_XULT
 */       
#define APP_CAN_BOOTLOADER_TARGET_DEVICE        SAM_E70_XULT    
    
#if APP_CAN_BOOTLOADER_TARGET_DEVICE == SAM_C21N_XPRO
#define APP_HEX_HEADER_FILE         "test_app_images/image_pattern_hex_sam_c21n_xpro.h"
#define APP_IMAGE_START_ADDR        0x1000UL

#elif APP_CAN_BOOTLOADER_TARGET_DEVICE == SAM_E54_XPRO   
#define APP_HEX_HEADER_FILE         "test_app_images/image_pattern_hex_sam_e54_xpro.h"
#define APP_IMAGE_START_ADDR        0x2000UL    

#elif APP_CAN_BOOTLOADER_TARGET_DEVICE == SAM_E70_XULT   
#define APP_HEX_HEADER_FILE         "test_app_images/image_pattern_hex_sam_e70_xult.h"
#define APP_IMAGE_START_ADDR        0x402000UL   

#elif APP_CAN_BOOTLOADER_TARGET_DEVICE == SAM_V71_XULT   
#define APP_HEX_HEADER_FILE         "test_app_images/image_pattern_hex_sam_v71_xult.h"
#define APP_IMAGE_START_ADDR        0x402000UL

#elif APP_CAN_BOOTLOADER_TARGET_DEVICE == PIC32CZ_CA80_CULT   
#define APP_HEX_HEADER_FILE         "test_app_images/image_pattern_hex_pic32cz_ca80_cult.h"
#define APP_IMAGE_START_ADDR        0xc000000UL

#endif    
    
//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif // USER_H
/*******************************************************************************
 End of File
*/
