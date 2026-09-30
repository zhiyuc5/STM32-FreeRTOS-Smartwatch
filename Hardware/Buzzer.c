#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

void Buzzer_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;       // PB14
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOB, GPIO_Pin_14); // 預設拉高電位 (低電平觸發，高電位=不叫)
}

// 蜂鳴器叫 3 聲的函式，利用 FreeRTOS 延遲，不佔用 CPU
void Buzzer_Beep3(void)
{
	for(int i = 0; i < 3; i++)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_14); // 嗶
		vTaskDelay(pdMS_TO_TICKS(150));
		GPIO_SetBits(GPIOB, GPIO_Pin_14);   // 停
		vTaskDelay(pdMS_TO_TICKS(150));
	}
}
