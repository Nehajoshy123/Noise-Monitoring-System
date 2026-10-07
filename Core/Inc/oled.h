#ifndef OLED_H
#define OLED_H

#include "main.h"

/* I2C Address for SSD1306 (0x3C shifted left by 1 for STM32 HAL) */
#define OLED_I2C_ADDR        (0x3C << 1)

/* Display dimensions */
#define OLED_WIDTH           128
#define OLED_HEIGHT          32

/* Core functions */
void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_SetCursor(uint8_t x, uint8_t page);
void OLED_WriteChar(char c);
void OLED_WriteString(const char *str);

#endif /* OLED_H */
