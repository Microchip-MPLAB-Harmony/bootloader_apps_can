# Configuring an application to be bootloaded for CORTEX-M based MCUs

**Parent topic:**[Configuring an Application to be bootloaded](GUID-1FAF8A87-195E-48BE-ADC4-614F430A19AE.md)

## Application settings in MCC system configuration

1.  Launch MCC for the application project to be configured

2.  Select **system** component from the project graph and configure the below highlighted settings

    ![application_config_mhc_setting.png](GUID-8D9BA07D-9C00-4BD6-AD12-6418A36E051E-low.png)

3.  **Disable Fuse Settings:**

    -   Fuse settings needs to be disabled for the application which will be boot-loaded as the fuse settings are supposed to be programmed through programming tool from bootloader code. Also the fuse settings are not programmable through firmware

    -   Enabling the fuse settings also increases the size of the binary when generated through the hex file

    -   When updating the bootloader itself, make sure that the fuse settings for the bootloader application are also disabled

4.  **Specify the Application Start Address:**

    -   Specify the Start address from where the application will run under the **Application Start Address \(Hex\)** option in System block in MCC.

    -   This value should be equal to or greater than the bootloader size and must be aligned to the erase unit size on that device.

    -   As this value will be used by bootloader to Jump to application at device reset it should match the value provided to bootloader code

    -   The **Application Start Address \(Hex\)** will be used to generate XC32 compiler settings to place the code at intended address

    -   After the project is regenerated, the ROM\_ORIGIN and ROM\_LENGTH are the XC32 linker variables which will be overridden with value provided for **Application Start Address \(Hex\)** and can be verified under Options for xc32-ld in Project Properties in MPLABX IDE as shown below.

    ![application_config_xc32_ld_rom.png](GUID-CD7ADA4F-B021-4554-A471-847841261FFD-low.png)


## MPLAB X Settings

-   Specifying post build option to automatically generate the binary file from hex file once the build is complete

    -   The generated binary file will serve as an input to the **btl\_bin\_to\_c\_array.py** to generate a C style array containing HEX data when the **host\_app\_nvm** host application is used

    ```
    	${MP_CC_DIR}/xc32-objcopy -I ihex -O binary ${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.hex ${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.bin
    ```

    ![application_config_post_build_script.png](GUID-97524091-B124-4574-AFF7-88C77B05FBBA-low.png)


## Additional settings \(Optional\)

-   **RAM\_ORIGIN** and **RAM\_LENGTH** values should be provided for reserving configured bytes from start of RAM to **trigger bootloader from firmware**

-   Under Project Properties, expand options for xc32-ld and define the values for **RAM\_ORIGIN** and **RAM\_LENGTH** under Additional options

-   This is optional and can be ignored if not required

    ![application_config_xc32_ld_ram.png](GUID-E7C52B4A-EB1C-4DE7-BB10-4BA5EDAB708D-low.png)


