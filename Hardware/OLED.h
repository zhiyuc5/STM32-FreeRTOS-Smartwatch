#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include "OLED_Data.h"

/*参数宏定义*********************/

/*FontSize参数取值*/
/*此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度*/
#define OLED_8X16				8
#define OLED_6X8				6
#define OLED_12X24      12
/*IsFilled参数数值*/
#define OLED_UNFILLED			0
#define OLED_FILLED				1

/*********************参数宏定义*/


/*函数声明*********************/

/*初始化函数*/
void OLED_Init(void);

/*更新函数*/
void OLED_Update(void);
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显存控制函数*/
void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显示函数*/
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/*绘图函数*/
void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

/*********************函数声明*/

extern uint8_t OLED_DisplayBuf[8][128];
#endif



//#ifndef __OLED_H
//#define __OLED_H

//#include "stm32f10x.h"
//#include "OLED_Data.h"
//#include <stdint.h>

///*
// * STM32F103 + SSD1306/SH1106 OLED
// * Hardware I2C1 + DMA version
// *
// * Default wiring:
// *   PB8 -> SCL
// *   PB9 -> SDA
// *
// * I2C1 is remapped to PB8/PB9.
// * I2C1 TX uses DMA1 Channel 6.
// */

///* FontSize values: also used as horizontal character spacing */
//#define OLED_8X16                    8
//#define OLED_6X8                     6
//#define OLED_12X24                  12

///* IsFilled values */
//#define OLED_UNFILLED                0
//#define OLED_FILLED                  1

///*
// * Optional configuration before including OLED.h:
// *
// * #define OLED_I2C_SPEED 400000U
// *
// * Default is 100 kHz, which is intentionally conservative for
// * breadboard/Dupont-wire wiring.
// *
// * #define OLED_I2C_TIMEOUT 1000000UL
// *
// * Optional logic-analyzer marker:
// * #define OLED_DMA_DEBUG_PA1
// *
// * When enabled, OLED_Update() toggles PA1 around the whole update.
// */

///* Global framebuffer */
//extern uint8_t OLED_DisplayBuf[8][128];

///* Initialization */
//void OLED_Init(void);
//void OLED_GPIO_Init(void);

///* Compatibility low-level interfaces */
//void OLED_W_SCL(uint8_t BitValue);
//void OLED_W_SDA(uint8_t BitValue);
//void OLED_I2C_Start(void);
//void OLED_I2C_Stop(void);
//void OLED_I2C_SendByte(uint8_t Byte);

///* OLED command/data interfaces */
//void OLED_WriteCommand(uint8_t Command);
//void OLED_WriteData(uint8_t *Data, uint8_t Count);

///* Cursor/update */
//void OLED_SetCursor(uint8_t Page, uint8_t X);
//void OLED_Update(void);
//void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

///* Framebuffer */
//void OLED_Clear(void);
//void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
//void OLED_Reverse(void);
//void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

///* Display */
//void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
//void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
//void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
//void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
//void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
//void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
//void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number,
//                      uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
//void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height,
//                    const uint8_t *Image);
//void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

///* Drawing */
//void OLED_DrawPoint(int16_t X, int16_t Y);
//uint8_t OLED_GetPoint(int16_t X, int16_t Y);
//void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
//void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height,
//                        uint8_t IsFilled);
//void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1,
//                       int16_t X2, int16_t Y2, uint8_t IsFilled);
//void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
//void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
//void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius,
//                  int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

///* DMA IRQ is implemented by OLED.c */
//void DMA1_Channel6_IRQHandler(void);

//#endif
