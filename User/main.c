#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "menu.h"
#include "Timer.h"
#include "Key.h"
#include "dino.h"
#include "MPU6050.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "MyRTC.h"
#include "Serial.h"
#include "Power.h"
#include "PWM.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ALARM_MAX 5  // 手錶最多支援5組鬧鐘
#define ALARM_FLASH_ADDR 0x0800FC00 // Flash storage define 64KB最後一頁起點
#define FLASH_MAGIC_NUM  0xA5A5     // 辨識碼，確認Flash裡是否有存過的資料

TaskHandle_t TaskUI_Handler;
TaskHandle_t TaskStopWatch_Handler; 
TaskHandle_t TaskBluetooth_Handler; 
TaskHandle_t TaskCountDown_Handler;
TaskHandle_t TaskLED_Handler;

QueueHandle_t StepQueue_Handle;

SemaphoreHandle_t RTC_Mutex; 
SemaphoreHandle_t Buzzer_Mutex;

int8_t cd_hour = 0, cd_min = 0, cd_sec = 0;
uint8_t cd_running = 0; 

char BT_RX_Buffer[30];      // 裝手機傳來的字串
uint8_t BT_RX_Index = 0;    // 接收位置指標

uint8_t is_gaming = 0;

int16_t AX, AY, AZ, GX, GY, GZ;	// MPU6050數據變數

// 溫度變數，預設25.0度(10+偏差15)
uint8_t Current_Temp_x10 = 100;

extern TickType_t LastKeyTime;
extern uint8_t is_ScreenOff;  

// watchdog標記(1=live,0=die)
volatile uint8_t Alive_UI = 0;
volatile uint8_t Alive_MPU = 0;
volatile uint8_t Alive_Alarm = 0;

// 鬧鐘的資料結構
typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t enable;
} Alarm_t;

Alarm_t MyAlarms[ALARM_MAX];

// 裝計步資料的結構
typedef struct {
    uint32_t steps;        // 放步數
    TickType_t timestamp;  // 放這步是何時發生
} SensorMsg_t;


void Flash_SaveAlarms(void)
{
    // 解鎖並擦除該頁(Flash寫入前必須先擦除，變成全1)
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    FLASH_ErasePage(ALARM_FLASH_ADDR); 

    // 寫入16-bit辨識碼
    FLASH_ProgramHalfWord(ALARM_FLASH_ADDR, FLASH_MAGIC_NUM);

    // 把MyAlarms陣列當成一長串記憶體，以16-bit為單位逐一寫入
    uint16_t *pData = (uint16_t *)MyAlarms;
    uint32_t data_length = sizeof(MyAlarms) / 2 + (sizeof(MyAlarms) % 2); 

    for (uint32_t i = 0; i < data_length; i++) {
        // 位址偏移：原本位址+2 bytes(避開辨識碼)+資料索引偏移
        FLASH_ProgramHalfWord(ALARM_FLASH_ADDR + 2 + (i * 2), pData[i]);
    }

    // 上鎖保護
    FLASH_Lock();
}

void Flash_LoadAlarms(void)
{
    // 讀取指定位址的辨識碼
    uint16_t magic = *(volatile uint16_t*)ALARM_FLASH_ADDR;

    // if辨識碼合，代表裡面有存過鬧鐘
    if (magic == FLASH_MAGIC_NUM) {
        uint16_t *pData = (uint16_t *)MyAlarms;
        uint32_t data_length = sizeof(MyAlarms) / 2 + (sizeof(MyAlarms) % 2);

        for (uint32_t i = 0; i < data_length; i++) {
            pData[i] = *(volatile uint16_t*)(ALARM_FLASH_ADDR + 2 + (i * 2));
        }
    }
}

// 通用螢幕喚醒(藍牙、鬧鐘、倒數計時共用)
void Screen_WakeUp(void)
{
    LastKeyTime = xTaskGetTickCount(); // 重置15秒計時器
    
    if (is_ScreenOff == 1)
    {
        is_ScreenOff = 0;              // 標記螢幕開啟
        vTaskResume(TaskUI_Handler);   // 喚醒UI任務重刷屏
    }
}

// 手錶的主畫面與選單切換
void Task_UI(void *pvParameters)
{		
	int clkflag1;
	uint8_t begin_flag = 1;
	
	// 系統啟動後稍等，確保OLED供電與I2C匯流排穩定(防黑屏)
	vTaskDelay(pdMS_TO_TICKS(200));

	while (1)
	{	
		Alive_UI = 1; // watchdog

		clkflag1 = First_Page_Clock();
		
		if(begin_flag == 1)
		{
			clkflag1 = 0;
			begin_flag = 0;
		}
		
		if(clkflag1 == 1){ Menu(); }           	 // 進menu
		else if(clkflag1 == 2){ SettingPage(); } // 進setting

		// 釋放CPU資源，防死迴圈卡死RTOS
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void Task_MPU6050(void *pvParameters)
{	
    float acc_mag;
    float baseline = 2048.0f; 
    TickType_t last_step_time = 0;
    
    // local步數計數器，和要傳送的包裹
    uint32_t local_step_count = 0;
    SensorMsg_t msg_to_send;

    while(1)
    {	
		Alive_MPU = 1; // watchdog

        MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);
            
        acc_mag = sqrt((float)AX*AX + (float)AY*AY + (float)AZ*AZ);
        baseline = baseline * 0.9f + acc_mag * 0.1f;
        float vibration = acc_mag - baseline;
            
        if(vibration > 2000.0f && (xTaskGetTickCount() - last_step_time) > pdMS_TO_TICKS(800))
        { 
            local_step_count++;
            last_step_time = xTaskGetTickCount();
                
            // 新步數打包進包裹
            msg_to_send.steps = local_step_count;
            msg_to_send.timestamp = last_step_time;
                
			// always覆蓋上一筆舊資料
            xQueueOverwrite(StepQueue_Handle, &msg_to_send);
        }       
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void Task_StopWatch(void *pvParameters)
{
	while(1)
	{
		// 1秒zz
		vTaskDelay(pdMS_TO_TICKS(1000)); 
		
		// 睡醒+1
		StopWatch_Tick();
	}
}

// 藍牙接任務
void Task_Bluetooth(void *pvParameters)
{
	while(1)
	{
		// 等手機傳資料過來(UART中斷喚醒)
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		
		Screen_WakeUp(); // 點亮螢幕
		
		// 判斷指令格式(格式：#2026,09,25,23,05,00*)
		if (BT_RX_Buffer[0] == '#' && BT_RX_Buffer[20] == '*')
		{
			extern int MyRTC_Time[];
			MyRTC_Time[0] = (BT_RX_Buffer[1]-'0')*1000 + (BT_RX_Buffer[2]-'0')*100 + (BT_RX_Buffer[3]-'0')*10 + (BT_RX_Buffer[4]-'0');
			MyRTC_Time[1] = (BT_RX_Buffer[6]-'0')*10 + (BT_RX_Buffer[7]-'0');
			MyRTC_Time[2] = (BT_RX_Buffer[9]-'0')*10 + (BT_RX_Buffer[10]-'0');
			MyRTC_Time[3] = (BT_RX_Buffer[12]-'0')*10 + (BT_RX_Buffer[13]-'0');
			MyRTC_Time[4] = (BT_RX_Buffer[15]-'0')*10 + (BT_RX_Buffer[16]-'0');
			MyRTC_Time[5] = (BT_RX_Buffer[18]-'0')*10 + (BT_RX_Buffer[19]-'0');
			
			MyRTC_SetTime(); 
		}
		// 判斷是否是鬧鐘指令
		else if (BT_RX_Buffer[0] == '@' && BT_RX_Buffer[8] == '*') 
		{
			// 抓鬧鐘編號
			uint8_t alarm_id = BT_RX_Buffer[1] - '0';
			
			// 確保ID沒有超出定義的最大值
			if (alarm_id < ALARM_MAX) 
			{
				// 格式：@0,07,30*
				MyAlarms[alarm_id].hour = (BT_RX_Buffer[3]-'0')*10 + (BT_RX_Buffer[4]-'0');
				MyAlarms[alarm_id].min  = (BT_RX_Buffer[6]-'0')*10 + (BT_RX_Buffer[7]-'0');
				MyAlarms[alarm_id].enable = 1; 

				// 收指令給回饋
				if(xSemaphoreTake(Buzzer_Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
					extern void Buzzer_Beep3(void);
					Buzzer_Beep3();   
					xSemaphoreGive(Buzzer_Mutex); 
				}
				// 設定完鬧鐘，寫入Flash保存
				Flash_SaveAlarms();
			}
		}
		
		// clear buffer，接下一次指令
		BT_RX_Index = 0; 
		memset(BT_RX_Buffer, 0, sizeof(BT_RX_Buffer));
	}
}

// 鬧鐘背景檢查任務
void Task_Alarm(void *pvParameters)
{
    extern void Buzzer_Beep3(void);
    extern int MyRTC_Time[];

    while(1)
    {
        // 每1秒檢查一次
        vTaskDelay(pdMS_TO_TICKS(1000)); 
		
		Alive_Alarm = 1; // watchdog
        
        MyRTC_ReadTime(); 
        
        // 掃描所有的鬧鐘
        for (int i = 0; i < ALARM_MAX; i++)
        {
            if (MyAlarms[i].enable == 1)
            {
                // 檢查鬧鐘時間是否抵達
                if (MyRTC_Time[3] == MyAlarms[i].hour && 
                    MyRTC_Time[4] == MyAlarms[i].min)
                {	
					Screen_WakeUp(); // 點亮螢幕
                    // 蜂鳴器的硬體防護鎖，防跟倒數計時race condition
                    if(xSemaphoreTake(Buzzer_Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                        Buzzer_Beep3();   
                        xSemaphoreGive(Buzzer_Mutex);
						// 響過了才把鬧鐘關掉
                        MyAlarms[i].enable = 0;		
						// 響完關閉後，同步更新到Flash
						Flash_SaveAlarms();
                    }
                }
            }
        }
    }
}

void Task_CountDown(void *pvParameters)
{
	extern void Buzzer_Beep3(void);
	
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000)); // 等1秒
		
		// 檢查時間到了沒
		if (cd_hour == 0 && cd_min == 0 && cd_sec == 0) {
			cd_running = 0;
			
			Screen_WakeUp(); // 點亮螢幕
			// 確保現在只有我在用蜂鳴器 (死等直到拿到鎖)
			if(xSemaphoreTake(Buzzer_Mutex, portMAX_DELAY) == pdTRUE) {
				Buzzer_Beep3();     
				xSemaphoreGive(Buzzer_Mutex); // 歸還
			}
			
			vTaskSuspend(NULL); 
			continue;
		}
		
		// 時間遞減邏輯
		if (cd_sec > 0) {
			cd_sec--;
		} else {
			if (cd_min > 0) {
				cd_min--;
				cd_sec = 59;
			} else {
				if (cd_hour > 0) {
					cd_hour--;
					cd_min = 59;
					cd_sec = 59;
				}
			}
		}
	}
}

// watchdog任務
// 拿電源狀態變數 (1=暗, 0=亮)
extern uint8_t is_ScreenOff; 

void Task_Monitor(void *pvParameters)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); 
    IWDG_SetPrescaler(IWDG_Prescaler_64);         
    IWDG_SetReload(1875);                         
    IWDG_ReloadCounter();                         
    IWDG_Enable();                                

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1500)); 
        
        // 判斷UI是否為合法休假
        uint8_t ui_healthy = 0;
        if (is_ScreenOff == 1) {
            ui_healthy = 1; // 如果螢幕關著，視為UI OK
        } else {
            ui_healthy = Alive_UI; // 如果螢幕亮著，就檢查有沒有簽到
        }

        // 點名
        if (ui_healthy == 1 && Alive_MPU == 1 && Alive_Alarm == 1)
        {
            IWDG_ReloadCounter(); // All good ? ok
            
            // 歸0下輪重簽
            Alive_UI = 0;
            Alive_MPU = 0;
            Alive_Alarm = 0;
        }
    }
}

void Task_Temp(void *pvParameters)
{
    extern uint16_t AD_GetValue(void);
    
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000)); 
        
        uint16_t ad_val = AD_GetValue();
        if (ad_val == 0) ad_val = 1; 
        if (ad_val >= 4095) ad_val = 4094;

        float Rt = 10000.0f * ((float)ad_val / (4095.0f - (float)ad_val));
        float tempK = 1.0f / (log(Rt / 10000.0f) / 3950.0f + 1.0f / (273.15f + 25.0f));
        float tempC = tempK - 273.15f;
        
        // 溫度乘10，加0.5做四捨五入後，轉成整數
        Current_Temp_x10 = (uint16_t)(tempC * 10.0f + 0.5f);
    }
}

// 呼吸燈任務
void Task_LED(void *pvParameters)
{
    extern void PWM_SetCompare1(uint16_t Compare);
    int16_t duty = 0;
    int8_t dir = 2; 
    
    while(1)
    {
        duty += dir;
        if(duty >= 100) {
            duty = 100;
            dir = -2;
        }
        else if(duty <= 0) {
            duty = 0;
            dir = 2;
        }
        
        PWM_SetCompare1((uint16_t)duty);
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

int main(void)
{
	//用中斷分組 4 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	RTC_Mutex = xSemaphoreCreateMutex();
	Buzzer_Mutex = xSemaphoreCreateMutex();
	// StepQueue的輸送帶，長度1
    StepQueue_Handle = xQueueCreate(1, sizeof(SensorMsg_t)); //10改1

	OLED_Init();
	OLED_Clear();
	Peripheral_Init(); 
	Timer_Init();      
	Serial_Init();
	Power_Init(); 
	PWM_Init();
	// 通電後把斷電前的鬧鐘設定弄回來
	Flash_LoadAlarms();
	
	xTaskCreate(
		Task_UI,             // 任務函式指標
		"UI_Task",           // 任務名稱
		1024,                // 堆疊大小
		NULL,                // 傳入參數
		1,                   // 優先權
		&TaskUI_Handler      // 任務控制塊指標(用於後續刪除或掛起任務)
	);
	
	xTaskCreate(
		Task_MPU6050, 
		"MPU_Task", 
		256, 
		NULL, 
		2, 
		NULL
	);
	
	xTaskCreate(
		Task_StopWatch, 
		"StopWatch", 
		128, 
		NULL, 
		2, 
		&TaskStopWatch_Handler
	);
	vTaskSuspend(TaskStopWatch_Handler);
	
	xTaskCreate(
		Task_Bluetooth, 
		"BT_Task", 
		256, 
		NULL, 
		3, 
		&TaskBluetooth_Handler
	);
	
	xTaskCreate(
		Task_CountDown, 
		"CountDown", 
		128, 
		NULL, 
		2, 
		&TaskCountDown_Handler
	);
	vTaskSuspend(TaskCountDown_Handler); 
	
	xTaskCreate(
		Task_Power, 
		"Power", 
		128, 
		NULL, 
		1, 
		NULL
	);
	
	xTaskCreate(
		Task_Alarm, 
		"Alarm_Task", 
		256, 
		NULL, 
		2,  
		NULL
	);
	
	xTaskCreate(
		Task_Monitor, 
		"Monitor", 
		128, 
		NULL, 
		4,    // 最高優先權
		NULL
	);
	
	xTaskCreate(
		Task_Temp, 
		"Temp_Task", 
		128,   
		NULL, 
		2,     
		NULL
	);
	
	// 呼吸燈背景任務
	xTaskCreate(
		Task_LED, 
		"LED_Task", 
		128,                 
		NULL, 
		2,                   // 與計步、碼錶同級
		&TaskLED_Handler
	);
	vTaskSuspend(TaskLED_Handler); // ★ 預設先掛起，不耗任何 CPU 資源
	//啟動任務調度
	vTaskStartScheduler();
	
	// 跑到這，代表Heap記憶體不足
	while(1);
}

// 定時器中斷函數，可複製到其他用他的地方
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Key3_Tick();
		Key_Tick();
		
		// 遊戲中才允許執行恐龍的運算
		if (is_gaming == 1)
		{
			Dino_Tick();
		}
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
