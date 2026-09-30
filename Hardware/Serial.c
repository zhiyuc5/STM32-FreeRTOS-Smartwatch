#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t TaskBluetooth_Handler;
extern char BT_RX_Buffer[30];
extern uint8_t BT_RX_Index;

void Serial_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // PA9(TX) 設為復用推挽輸出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // PA10(RX) 設為浮空輸入
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// USART1 初始化(HC-05預設鮑率為9600)
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStructure);
	
	// 開啟接收中斷與 NVIC 設定
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	// 啟動 USART1
	USART_Cmd(USART1, ENABLE);
}

// USART1 中斷服務常式：每次收到一個字元就會跳進來這裡一次
void USART1_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		char received_char = USART_ReceiveData(USART1); // 讀取收到的字元
		
		if (received_char == '#' || received_char == '@') {
			BT_RX_Index = 0; // 收到開頭暗號 '#'，索引歸零重新記錄
		}
		
		// 把字元存進緩衝區，防止溢位
		if (BT_RX_Index < 29) {
			BT_RX_Buffer[BT_RX_Index++] = received_char;
		}
		
		if (received_char == '*') {
			BT_RX_Buffer[BT_RX_Index] = '\0'; // 加上字串結尾符號
			
			// 收完完整的時間字串 瞬間發送訊號喚醒藍牙任務去更新時間
			vTaskNotifyGiveFromISR(TaskBluetooth_Handler, &xHigherPriorityTaskWoken);
		}
		
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
	
	// 如果喚醒的任務優先權很高，要求FreeRTOS離開中斷後立刻切換任務
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
