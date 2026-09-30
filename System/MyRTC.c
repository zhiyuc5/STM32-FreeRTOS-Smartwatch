#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <time.h>

extern SemaphoreHandle_t RTC_Mutex;

int MyRTC_Time[] = {2026, 9, 28, 8, 40, 50};	//設定時間

void MyRTC_SetTime(void);				

void MyRTC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);		// 開啟PWR的時鐘
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);		// 開啟BKP的時鐘
	
	// 備份暫存器存取啟用
	PWR_BackupAccessCmd(ENABLE);							// 使用PWR開啟對備份暫存器的訪問
	
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)			// 透過寫入備份暫存器的標誌位，判斷RTC是否為第一次配置
															// if成立則執行第一次的RTC配置
	{
		RCC_LSEConfig(RCC_LSE_ON);							// 開啟LSE時鐘
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET);	// 等待LSE準備就緒
		
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);				// 選擇RTCCLK來源為LSE
		RCC_RTCCLKCmd(ENABLE);								// RTCCLK使能
		
		RTC_WaitForSynchro();								// 等待同步
		RTC_WaitForLastTask();								// 等待上一次操作完成
		
		RTC_SetPrescaler(32768 - 1);						// 設定RTC預分頻器，預分頻後的計數頻率為1Hz
		RTC_WaitForLastTask();								// 等待上一次操作完成
		 
		MyRTC_SetTime();									// 設定時間，呼叫此函數，全域數組裡時間值刷新到RTC硬體電路
		
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);			// 在備份暫存器寫入自己規定的標誌位，用來判斷RTC是不是第一次執行配置
	}
	else													// RTC不是第一次配置
	{
		RTC_WaitForSynchro();								// 等待同步
		RTC_WaitForLastTask();								// 等待上一次操作完成
	}
}

void MyRTC_SetTime(void)
{
	time_t time_cnt;
	struct tm time_date;
	
	time_date.tm_year = MyRTC_Time[0] - 1900;
	time_date.tm_mon = MyRTC_Time[1] - 1;
	time_date.tm_mday = MyRTC_Time[2];
	time_date.tm_hour = MyRTC_Time[3];
	time_date.tm_min = MyRTC_Time[4];
	time_date.tm_sec = MyRTC_Time[5];
	
	// 申請RTC鎖(死等直到拿到為止)
	xSemaphoreTake(RTC_Mutex, portMAX_DELAY);
	
	time_cnt = mktime(&time_date) - 8 * 60 * 60;
	RTC_SetCounter(time_cnt);
	RTC_WaitForLastTask();
	
	// 歸還RTC鎖
	xSemaphoreGive(RTC_Mutex);
}

void MyRTC_ReadTime(void)
{
	time_t time_cnt;
	struct tm time_date;
	
	// 申請RTC鎖
	xSemaphoreTake(RTC_Mutex, portMAX_DELAY);
	
	time_cnt = RTC_GetCounter() + 8 * 60 * 60;
	time_date = *localtime(&time_cnt);
	
	MyRTC_Time[0] = time_date.tm_year + 1900;
	MyRTC_Time[1] = time_date.tm_mon + 1;
	MyRTC_Time[2] = time_date.tm_mday;
	MyRTC_Time[3] = time_date.tm_hour;
	MyRTC_Time[4] = time_date.tm_min;
	MyRTC_Time[5] = time_date.tm_sec;

	// 歸還RTC鎖
	xSemaphoreGive(RTC_Mutex);
}
