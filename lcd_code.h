#include "project.h"

#define I2C_ADDRESS 0x27  // I2C address for PCF8574 I/O expander
#define LCD_BACKLIGHT 0x08  // Turn on light of screen
#define LCD_ENABLE    0x04
#define LCD_READWRITE 0x02
#define LCD_REGISTER  0x01

void I2C0_Init(void);
void I2C_MODULE_Write(uint8_t data);
void LCD_SendNibble(uint8_t nibble, bool isData);
void LCD_SendCommand(uint8_t command);
void LCD_SendData(uint32_t data);
void LCD_Init(void);
void LCD_WriteString(char *str);
void DelayMs(int ms);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);