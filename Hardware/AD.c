#include "stm32f10x.h"                  // Device header

/**
  * 函 數：AD初始化
  * 參 數：無
  * 返 回 值：無
  */
void AD_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
	
	// 設定ADC時鐘
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);						// 選擇時脈6分頻，ADCCLK = 72MHz / 6 = 12MHz
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					// 將PA0引腳初始化為類比輸入
	
	// 規則組通道配置
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);// 55cycle->239cycle		//规则组序列1的位置，配置为通道0
	
	ADC_InitTypeDef ADC_InitStructure;						// 定義結構體變數
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;		// 模式，選擇獨立模式，即單獨使用ADC1
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	// 數據對齊，選擇右對齊
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	// 外部觸發，使用軟體觸發，不需要外部觸發
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		// 連續轉換，失能，每轉換一次規則組序列後停止
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;			// 掃描模式，失能，只轉換規則組的序列1這一位置
	ADC_InitStructure.ADC_NbrOfChannel = 1;					// 通道數，為1，僅在掃描模式下，才需要指定大於1的數，在非掃描模式下，只能是1
	ADC_Init(ADC1, &ADC_InitStructure);						// 將結構體變數交給ADC_Init，配置ADC1
	
	ADC_Cmd(ADC1, ENABLE);									// 啟用ADC1，ADC開始運行
	
	// ADC校準
	ADC_ResetCalibration(ADC1);								// 固定流程，內部有電路會自動執行校準
	while (ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1) == SET);
}

/**
  * 函 數：取得AD轉換的值
  * 參 數：無
  * 返 回 值：AD轉換的值，範圍：0~4095
  */
uint16_t AD_GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);					// 軟體觸發AD轉換一次
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	// 等待EOC標誌位，即等待AD轉換結束
	return ADC_GetConversionValue(ADC1);					// 讀資料暫存器，得到AD轉換的結果
}
