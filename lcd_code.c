#include "lcd_code.h"
void DelayMs(int ms) 
{
    SysCtlDelay((SysCtlClockGet() / (3 * 1000)) * ms);
}

void I2C0_Init(void)
{
    // Enable I2C0 and GPIOB for I2C communication
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    // Set pin functions for I2C communication on PB2 and PB3
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);

    // Configure pins for I2C function
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);

    // Initialize I2C master with system clock and standard speed
    I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);
}
// Function to write a byte to the PCF8574 I2C expander
void I2C_MODULE_Write(uint8_t data) 
{
    I2CMasterSlaveAddrSet(I2C0_BASE, I2C_ADDRESS, false);
    I2CMasterDataPut(I2C0_BASE, data);
    I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);

    while (I2CMasterBusy(I2C0_BASE)) 
		{
    }
}
////////////////////////////////////////////////////////////////////
/////////////LCD/////////////////////////////////////////////////
// Send a 4-bit nibble to the LCD with control bits via PCF8574
void LCD_SendNibble(uint8_t nibble, bool isData)
{
    uint8_t control = LCD_BACKLIGHT | (isData ? LCD_REGISTER : 0);
    I2C_MODULE_Write((nibble << 4) | control | LCD_ENABLE);
    DelayMs(1);
    I2C_MODULE_Write((nibble << 4) | control);
    DelayMs(1);
}
////////////////////////////////////////////////////////////////////
// Send commands to the LCD
void LCD_SendCommand(uint8_t command) 
{
    LCD_SendNibble(command >> 4, false);
    LCD_SendNibble(command & 0x0F, false);
    DelayMs(2);
}
////////////////////////////////////////////////////////////////////
// Send characters to the LCD
void LCD_SendData(uint32_t data) 
{
    LCD_SendNibble(data >> 4, true);
    LCD_SendNibble(data & 0x0F, true);
    DelayMs(2);
}
////////////////////////////////////////////////////////////////////
void LCD_Init(void) 
{
		DelayMs(50); // Wait for LCD to stabilize after power-up

    // Set LCD to 8-bit mode initially
    LCD_SendNibble(0x03, false); 
    DelayMs(5);
    LCD_SendNibble(0x03, false);
    DelayMs(5);
    LCD_SendNibble(0x03, false);
    DelayMs(5);
    LCD_SendNibble(0x02, false); // Switch to 4-bit mode

    // Configure LCD with desired functions (2 lines)
    LCD_SendCommand(0x28);
    // Configure LCD display control (display on, cursor settings)
    LCD_SendCommand(0x0C);
    // Set LCD entry mode (cursor move direction, display shift)
    LCD_SendCommand(0x06);
    // Clear LCD screen
    LCD_SendCommand(0x01);
    DelayMs(2);
}
////////////////////////////////////////////////////////////////////
// Write a string to the LCD
void LCD_WriteString(char *str)
{
    while (*str) 
		{
        LCD_SendData(*str++);
    }
}
// Set cursor position: (row = 0 or 1), (col = 0 to 15)
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
    {
        address = col;
    }
    else if (row == 1)
    {
        address = 0x40 + col;
    }
    else
    {
        // Default to row 0 if invalid input
        address = col;
    }

    LCD_SendCommand(0x80 | address);
}

void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    DelayMs(2);
}