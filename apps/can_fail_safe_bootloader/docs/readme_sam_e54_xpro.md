---
grand_parent: CAN Bootloader Applications
parent: CAN Fail Safe Bootloader
title: Building and Running on SAM E54 Xplained Pro Evaluation Kit
has_toc: false
---

[![MCHP](https://www.microchip.com/ResourcePackages/Microchip/assets/dist/images/logo.png)](https://www.microchip.com)

# Building and Running the CAN Fail Safe Bootloader applications

## Downloading and building the application

To clone or download this application from Github,go to the [main page of this repository](https://github.com/Microchip-MPLAB-Harmony/bootloader_apps_can) and then click Clone button to clone this repo or download as zip file. This content can also be download using content manager by following [these instructions](https://github.com/Microchip-MPLAB-Harmony/contentmanager/wiki)

Path of the application within the repository is **apps/can_fail_safe_bootloader/**

To build the application, refer to the following table and open the project using its IDE.

### Bootloader Application

| Project Name      | Description                                    |
| ----------------- | ---------------------------------------------- |
| bootloader/firmware/sam_e54_xpro.X    | MPLABX Project for [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/atsame54-xpro)|


### Test Application

| Project Name      | Description                                    |
| ----------------- | ---------------------------------------------- |
| test_app/firmware/sam_e54_xpro.X    | MPLABX Project for [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/atsame54-xpro)|


## Setting up [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/atsame54-xpro)

- [SAM E54 Xplained Pro Evaluation Kit](https://www.microchip.com/developmenttools/ProductDetails/atsame54-xpro) is used for both **Host Development kit** and **Target Development kit**

    ![can_bootloader_host_target_connection](../../docs/images/can_bootloader_host_target_connection.png)

- Connect the CANL on CAN Header of the **Host development kit** to the CANL on CAN Header of the **Target development kit**
- Connect the CANH on CAN Header of the **Host development kit** to the CANH on CAN Header of the **Target development kit**
- Connect a ground wire between the Host development kit and Target development kit
- Connect the Debug USB port on the Host development kit to the computer using a micro USB cable
- Connect the Debug USB port on the Target development kit to the computer using a micro USB cable

## Running the Application

1. Open the bootloader project *bootloader/firmware/sam_e54_xpro.X* in the IDE
2. Build and program the application using the IDE on to the **Target development kit**
    - **LED0** will be turned-on to indicate that bootloader code is running on the target
    - **LED0** will also turn on when the bootloader does not find a valid application; i.e. the first word of the application (stack pointer), contains 0xFFFFFFFF

3. Open the NVM host application project *host_app_nvm/firmware/sam_e54_xpro.X* in the IDE

4. Build and program the NVM host application using the IDE on to the Host development kit
    - The prebuilt combined bootloader and application HEX header file **host_app_nvm/firmware/src/test_app_images/image_pattern_hex_sam_e54_xpro_bootloader_app_merged.h** will be programmed to the **Target Development kit**

    - This must be programmed once to program both bootloader and application into the inactive flash bank

5. Open the Terminal application (Ex.:Tera Term) on the computer
6. Configure the serial port settings for **Target Development kit** as follows:
    - Baud : 115200
    - Data : 8 Bits
    - Parity : None
    - Stop : 1 Bit
    - Flow Control : None

7. Press the Switch **SW0** on the Host development kit to trigger programming of the binary
8. Once the programming is complete,
    - **LED0** on the **Host development kit** will be turned on indicating success

    - **LED0** on the **Target development kit** should start blinking and you should see below output on the console
        - The NVM Flash Bank Can be **BANK A** or **BANK B** based on from where the test application is running

    ![output](./images/btl_can_test_app_console_bank_b.png)

    ![output](./images/btl_can_test_app_console_bank_a.png)

9. Press and hold the Switch **SW0** on the **Target development kit** to trigger Bootloader from test application
    - This is to program the application binary in other bank and you should see below output

    ![output](./images/btl_can_test_app_console_bank_b_trigger_bootloader.png)

10. To program the prebuilt application image **host_app_nvm/firmware/src/test_app_images/image_pattern_hex_sam_e54_xpro.h** to the inactive bank of **Target Development kit**, Open the "user.h" file of the NVM host application project *host_app_nvm/firmware/sam_e54_xpro.X* as shown below:

    ![btl_can_fail_safe_host_app_nvm_user_ide](./images/btl_can_fail_safe_host_app_nvm_user_ide.png)

11. In the "user.h" file update the **APP_HEX_HEADER_FILE** and **APP_IMAGE_START_ADDR** values as mentioned below

    ```
    #define APP_HEX_HEADER_FILE         "test_app_images/image_pattern_hex_sam_e54_xpro.h"

    #define APP_IMAGE_START_ADDR        0x82000UL
    ```

    ![btl_can_fail_safe_host_app_nvm_user_config](./images/btl_can_fail_safe_host_app_nvm_user_config.png)

    - **APP_HEX_HEADER_FILE:** Relative path to the generated header file containing the application hex image in an array
    - **APP_IMAGE_START_ADDR:** User application start address
        - It must be set to **0x80000UL** when programming the **combined bootloader and application binary** to the inactive bank
        - It must be set to **0x82000UL** when programming the **application binary** only to the inactive bank

12. Build and program the NVM host application using the IDE on to the **Host development kit**

13. On the **Target Development Kit** (the board being programmed), press and hold the Switch **SW0** and then press Reset button or Power cycle to force trigger bootloader at startup
    - **LED0** will be turned-on to indicate that bootloader code is running on the target

14. Press Reset button on the **Host development kit** to program the application binary
15. Repeat Steps 7-8 once
    - You should see other Bank in console displayed compared to first run


## Additional Steps (Optional)

### Generating Hex Image pattern for an application to be bootloaded

![bin_to_c_array](../../../tools/docs/images/btl_bin_to_c_array.png)

1. To bootload any application other than **host_app_nvm/firmware/src/test_app_images/image_pattern_hex_sam_e54_xpro.h** refer to [Application Configurations](../../docs/readme_configure_application_sam.md)

2. Build the application project to generate the binary **(Do not program the binary)**

3. Convert the generated binary (.bin file) to a header file containing the image data in a C style array:
    - On a Windows machine, open the command prompt and run the [btl_bin_to_c_array](../../../tools/docs/readme_btl_bin_to_c_array.md) utility to generate a header file containing the image data in an array

          python <harmony3_path>\bootloader\tools\btl_bin_to_c_array_gen.py -b <binary_file> -o <harmony3_path>\bootloader_apps_can\apps\can_fail_safe_bootloader\host_app_nvm\firmware\src\test_app_images\image_pattern_hex_sam_e54_xpro.h -d same5x

4. Once done repeat the applicable steps mentioned in [Running The Application](#running-the-application)

### Generating Hex Image pattern for Commbined Bootloader and Application Binary

![can_bootloader_host_nvm_btl_app_merge](../../docs/images/can_bootloader_host_nvm_btl_app_merge.png)

1. Launch MHC for the bootloader project *bootloader/firmware/sam_e54_xpro.X*
2. Select **system** component from the project graph and disable fuse settings

3. **Disable Fuse Settings:**
    - Fuse settings needs to be disabled for the bootloader which will be boot-loaded as the fuse settings are supposed to be programmed through programming tool
    - Also the fuse settings are not programmable through firmware
    - Enabling the fuse settings also increases the size of the binary when generated through the hex file

    ![btl_can_fail_safe_config_fuse_setting](./images/btl_can_fail_safe_config_fuse_setting.png)

4. Regenrate the project

5. Specifing post build option to automatically generate the binary file from hex file once the build is complete

        ${MP_CC_DIR}/xc32-objcopy -I ihex -O binary ${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.hex ${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.bin

    ![btl_can_fail_safe_post_build_script](./images/btl_can_fail_safe_post_build_script.png)

6. Build the bootloader project to generate the bootloader binary **(Do not program the binary)**

7. Build the sample application test_app (or any other application configured to bootload) using IDE to generate the application binary **(Do not program the binary)**

8. On a Windows machine, open the command prompt

9. Run the [btl_app_merge_bin.py](../../../tools/docs/readme_btl_app_merge_bin.md) utiliy from command prompt to merge the generated Bootloader binary and test application binary. Below output should be displayed on command prompt

        python <harmony3_path>\bootloader\tools\btl_app_merge_bin.py -o 0x2000 -b <harmony3_path>\bootloader_apps_can\apps\can_fail_safe_bootloader\bootloader\firmware\sam_e54_xpro.X\dist\sam_e54_xpro\production\sam_e54_xpro.X.production.bin -a <harmony3_path>\bootloader_apps_can\apps\can_fail_safe_bootloader\test_app\firmware\sam_e54_xpro.X\dist\sam_e54_xpro\production\sam_e54_xpro.X.production.bin

    ![btl_can_fail_safe_app_merger_console](./images/btl_can_fail_safe_app_merger_console.png)

10. Run [btl_bin_to_c_array](../../../tools/docs/readme_btl_bin_to_c_array.md) utility to convert the generated merged binary **btl_app_merged.bin** to a header file containing the image data in a C style array
    - The merged binary will be created in the directory from where the script was called

          python <harmony3_path>\bootloader\tools\btl_bin_to_c_array.py -b <Path_to_merged_binary>\btl_app_merged.bin -o <harmony3_path>\bootloader_apps_can\apps\can_fail_safe_bootloader\host_app_nvm\firmware\src\test_app_images\image_pattern_hex_sam_e54_xpro_bootloader_app_merged.h -d same5x

11. Once done repeat the applicable steps mentioned in [Running The Application](#running-the-application)
