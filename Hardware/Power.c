#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"

extern TaskHandle_t TaskUI_Handler; 
extern uint8_t Key_Num;             // 拿取底層按鍵變數
extern TickType_t WakeUpTime;

TickType_t LastKeyTime = 0;
uint8_t is_ScreenOff = 0;       // 記錄目前是不是黑屏狀態


void Power_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	
	// 設定 PA4 為上拉輸入
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4);
	
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

// 外部中斷：只要按下 PA4 就會觸發
void EXTI4_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line4) != RESET)
	{
		LastKeyTime = xTaskGetTickCountFromISR(); 
		
		if(is_ScreenOff == 1)
		{
			is_ScreenOff = 0; 
			
			// 記錄螢幕剛亮起的時間點
			WakeUpTime = xTaskGetTickCountFromISR(); 
			
			// 叫醒UI任務
			BaseType_t xYieldRequired = xTaskResumeFromISR(TaskUI_Handler);
			if(xYieldRequired == pdTRUE) {
				portYIELD_FROM_ISR(xYieldRequired);
			}
		}
		
		EXTI_ClearITPendingBit(EXTI_Line4);
	}
}

// 電源與關螢幕管理任務
void Task_Power(void *pvParameters)
{
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(100)); // 每0.1秒檢查一次
		
		// 如果超過 15 秒沒按按鍵，且螢幕還亮著
		if((xTaskGetTickCount() - LastKeyTime) > pdMS_TO_TICKS(15000) && is_ScreenOff == 0)
		{
			is_ScreenOff = 1; // 標記為黑屏狀態
			
			// 凍結 UI 任務
			vTaskSuspend(TaskUI_Handler);
			
			// 清空螢幕，達到關屏效果
			OLED_Clear();
			OLED_Update();
			
		}
	}
}
