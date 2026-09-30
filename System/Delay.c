//#include "stm32f10x.h"

///**
//  * @brief  微秒级延时
//  * @param  xus 延时时长，范围：0~233015
//  * @retval 无
//  */
//void Delay_us(uint32_t xus)
//{
//	SysTick->LOAD = 72 * xus;				//设置定时器重装值
//	SysTick->VAL = 0x00;					//清空当前计数值
//	SysTick->CTRL = 0x00000005;				//设置时钟源为HCLK，启动定时器
//	while(!(SysTick->CTRL & 0x00010000));	//等待计数到0
//	SysTick->CTRL = 0x00000004;				//关闭定时器
//}

///**
//  * @brief  毫秒级延时
//  * @param  xms 延时时长，范围：0~4294967295
//  * @retval 无
//  */
//void Delay_ms(uint32_t xms)
//{
//	while(xms--)
//	{
//		Delay_us(1000);
//	}
//}
// 
///**
//  * @brief  秒级延时
//  * @param  xs 延时时长，范围：0~4294967295
//  * @retval 无
//  */
//void Delay_s(uint32_t xs)
//{
//	while(xs--)
//	{
//		Delay_ms(1000);
//	}
//} 
#include "stm32f10x.h"

/**
  * @brief  微秒级延时 (FreeRTOS 專用軟體延遲版)
  * @param  xus 延时时长
  * @retval 无
  * @note   已將 SysTick 釋放給 FreeRTOS，改用 CPU 空迴圈實現延遲
  */
void Delay_us(uint32_t xus)
{
	// 針對 72MHz 的 STM32F103，編譯器最佳化下，迴圈約消耗 6 個 CPU 時鐘週期
	uint32_t delay = 72 * xus / 6; 
	while (delay--);
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}
 
/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
}
