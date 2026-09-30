#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "MyRTC.h"                                                                                                                                                                                                     
#include "Key.h"
#include "LED.h"
#include "SetTime.h"
#include "menu.h"
#include "MPU6050.h"
#include "Delay.h"
#include "dino.h"
#include "AD.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Buzzer.h"
#include "queue.h"   
#include <math.h>

uint8_t KeyNum;	//存按鍵值

extern uint32_t Step_Count;

extern int8_t cd_hour, cd_min, cd_sec;
extern uint8_t cd_running;

extern TaskHandle_t TaskStopWatch_Handler; 
extern TaskHandle_t TaskCountDown_Handler;
extern TaskHandle_t TaskLED_Handler;

// PWM設定
extern void PWM_SetCompare1(uint16_t Compare);
// 宣告定義的queue struct
typedef struct {
    uint32_t steps;
    TickType_t timestamp;
} SensorMsg_t;

extern QueueHandle_t StepQueue_Handle;

extern volatile uint8_t Alive_UI;
extern uint8_t Current_Temp_x10;

int Timer_Selection(void);
int CountDown(void);

void Peripheral_Init(void)
{
	MyRTC_Init();
	Key_Init();
	LED_Init();
	MPU6050_Init();
	AD_Init();
	Buzzer_Init();
	
	// CPU負載實驗 PA1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 預設拉低
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);
}



//時鐘首頁
//時鐘首頁UI
void Show_Clock_UI(void)
{
	MyRTC_ReadTime();
	OLED_Printf(0,0,OLED_6X8,"%d-%d-%d",MyRTC_Time[0],MyRTC_Time[1],MyRTC_Time[2]);
	OLED_Printf(16,16,OLED_12X24,"%02d:%02d:%02d",MyRTC_Time[3],MyRTC_Time[4],MyRTC_Time[5]);
	OLED_ShowString(0,48,"Menu",OLED_8X16);
	OLED_ShowString(100,48,"Set",OLED_8X16);
	
	// +15偏差
	OLED_Printf(80, 0, OLED_6X8, "%d.%d C", (Current_Temp_x10 / 10) + 15, Current_Temp_x10 % 10);
}


int clkflag=1;	// 首頁時鐘按鍵標誌位，賦初值為1，光標一進來停在第一項

//	控制光標在時鐘首頁的移動
int First_Page_Clock(void)
{
	while(1)
	{	
		Alive_UI = 1;	// watchdog
		KeyNum=Key_GetNum();

		if(KeyNum == 1)	// 上一項
		{
			clkflag--;
			if(clkflag <= 0) clkflag = 2;
		}
		else if(KeyNum == 2)	// 下一項
		{
			clkflag++;
			if(clkflag >= 3) clkflag = 1;
		}
		else if(KeyNum == 3)	// 確定鍵
		{
			OLED_Clear();
			OLED_Update();
			return clkflag;
		}
		
		else if(KeyNum==4)
		{
			// 原開關機(長按)
			GPIO_ResetBits(GPIOB, GPIO_Pin_13);
			GPIO_SetBits(GPIOB, GPIO_Pin_12);
		};
		switch(clkflag)
		{
			case 1:
				OLED_Clear();
				Show_Clock_UI();
				OLED_ReverseArea(0,48,32,16);
				OLED_Update();
				break;
			
			case 2:
				OLED_Clear();
				Show_Clock_UI();
				OLED_ReverseArea(96,48,32,16);
				OLED_Update();
				break;
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

// 設定介面

// 設定介面UI
void Show_SettingPage_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"Time Setting",OLED_8X16);
}

int setflag=1;
// 控制光標在設定的移動
int SettingPage(void)
{
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum = Key_GetNum();
		uint8_t setflag_temp = 0;
		if(KeyNum == 1)	// 上一項
		{
			setflag--;
			if(setflag<=0) setflag=2;
		}
		else if(KeyNum == 2)	// 下一項
		{
			setflag++;
			if(setflag >= 3) setflag = 1;
		}
		else if(KeyNum == 3)	// 確定
		{
			OLED_Clear();
			OLED_Update();
			setflag_temp=setflag;
		}
		
		if(setflag_temp == 1){ return 0;}
		else if(setflag_temp == 2){ SetTime();}	// 跳到日期時間設定
		
		switch(setflag)
		{
			case 1:
				Show_SettingPage_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;
			
			case 2:
				Show_SettingPage_UI();
				OLED_ReverseArea(0,16,96,16);
				OLED_Update();
				break;
		}
	}
}

// menu
uint8_t pre_selection;	// 上個選項
uint8_t target_selection;	// 目標選項
uint8_t x_pre = 48;	// 上次選項的x座標
uint8_t Speed = 4;	// 速度
uint8_t move_flag;	// 開始移動的標誌位，1表示開始移動，0表示停止移動


// menu動畫
void Menu_Animation(void)
{
	OLED_Clear();
	OLED_ShowImage(42,10,44,44,Frame);
	
	if(pre_selection<target_selection)
	{
		x_pre-=Speed;
		if(x_pre==0)
		{
			pre_selection++;
			move_flag=0;
			x_pre=48;
		}
	}
	
	if(pre_selection>target_selection)
	{
		x_pre+=Speed;
		if(x_pre==96)
		{
			pre_selection--;
			move_flag=0;
			x_pre=48;
		}
	}
	
	if(pre_selection>=1)
	{
		OLED_ShowImage(x_pre-48,16,32,32,Menu_Graph[pre_selection-1]);
	}
	
	if(pre_selection>=2)
	{
		OLED_ShowImage(x_pre-96,16,32,32,Menu_Graph[pre_selection-2]);
	}
	
	OLED_ShowImage(x_pre,16,32,32,Menu_Graph[pre_selection]);
	OLED_ShowImage(x_pre+48,16,32,32,Menu_Graph[pre_selection+1]);
	OLED_ShowImage(x_pre+96,16,32,32,Menu_Graph[pre_selection+2]);
	
	OLED_Update();
}


// 選擇移動方向
void Set_Selection(uint8_t move_flag,uint8_t Pre_Selection,uint8_t Target_Selection)
{
	if(move_flag==1)
	{
		pre_selection=Pre_Selection;
		target_selection=Target_Selection;
		
	}
	Menu_Animation();
}

// 轉場
void MenuToFunction(void)
{
	for(uint8_t i=0;i<=6;i++) // 下移8 只有6次
	{
		OLED_Clear();
			if(pre_selection>=1)
		{
			OLED_ShowImage(x_pre-48,16+8*i,32,32,Menu_Graph[pre_selection-1]);
		}
		
		
		OLED_ShowImage(x_pre,16+8*i,32,32,Menu_Graph[pre_selection]);
		OLED_ShowImage(x_pre+48,16+8*i,32,32,Menu_Graph[pre_selection+1]);
		
		OLED_Update();
	}
	
}


uint8_t menu_flag=1;
// menu光標移動
int Menu(void)
{
	move_flag=1;
	uint8_t DirectFlag=2;	// 1上一項 2下一項
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum=Key_GetNum();
		uint8_t menu_flag_temp=0;
		if(KeyNum==1)	// 上一項
		{
			DirectFlag=1;
			move_flag=1;
			menu_flag--;
			if(menu_flag<=0)menu_flag=7;
		}
		else if(KeyNum==2)	// 下一項
		{
			DirectFlag=2;
			move_flag=1;
			menu_flag++;
			if(menu_flag>=8)menu_flag=1;
		}
		else if(KeyNum==3)	// 確定
		{
			OLED_Clear();
			OLED_Update();
			menu_flag_temp=menu_flag;
		}
		
		if(menu_flag_temp==1){return 0;}
		else if(menu_flag_temp==2){MenuToFunction();Timer_Selection();}
		else if(menu_flag_temp==3){MenuToFunction();LED();}		// 跳到手電筒
		else if(menu_flag_temp==4){MenuToFunction();MPU6050();}	// 跳到mpu6050
		else if(menu_flag_temp==5){MenuToFunction();Game();}	// 跳到GAME
		else if(menu_flag_temp==6){MenuToFunction();Emoji();}	// 跳到動畫
		else if(menu_flag_temp==7){MenuToFunction();Gradienter();}	// 跳到水平儀
			

			if(menu_flag==1)
			{
				if(DirectFlag==1)Set_Selection(move_flag,1,0);
				else if(DirectFlag==2)Set_Selection(move_flag,0,0);
			}
			
			else
			{
				if(DirectFlag==1)Set_Selection(move_flag,menu_flag,menu_flag-1);
				else if(DirectFlag==2)Set_Selection(move_flag,menu_flag-2,menu_flag-1);
			}
	}
}


// 碼表
uint8_t hour,min,sec;
void Show_TimerSelect_UI(void)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ShowString(16, 20, "1. StopWatch", OLED_8X16);
	OLED_ShowString(16, 40, "2. CountDown", OLED_8X16);
}

int Timer_Selection(void)
{
	uint8_t ts_cursor = 1;
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum = Key_GetNum();
		if(KeyNum == 1) { ts_cursor--; if(ts_cursor <= 0) ts_cursor = 3; }
		else if(KeyNum == 2) { ts_cursor++; if(ts_cursor >= 4) ts_cursor = 1; }
		else if(KeyNum == 3) {
			if(ts_cursor == 1) { OLED_Clear(); OLED_Update(); return 0; }
			else if(ts_cursor == 2) { StopWatch(); } // 進入碼錶
			else if(ts_cursor == 3) { CountDown(); } // 進入倒數計時
		}
		
		OLED_Clear();
		Show_TimerSelect_UI();
		switch(ts_cursor) {
			case 1: OLED_ReverseArea(0,0,16,16); break;
			case 2: OLED_ReverseArea(16,20,96,16); break;
			case 3: OLED_ReverseArea(16,40,96,16); break;
		}
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

// 碼表介面
void Show_StopWatch_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(32,20,OLED_8X16,"%02d:%02d:%02d",hour,min,sec);
	OLED_ShowString(0,44,"Start",OLED_8X16); // 1 8
	OLED_ShowString(48,44,"Stop ",OLED_8X16);
	OLED_ShowString(88,44,"Clear",OLED_8X16);
}

void StopWatch_Tick(void)
{
	sec++;
	if(sec >= 60)
	{
		sec = 0;
		min++;
		if(min >= 60)
		{
			min = 0;
			hour++;
			if(hour > 99) hour = 0;
		}
	}
}

uint8_t stopwatch_flag = 1; // 記錄光標目前停在哪個按鈕

int StopWatch(void)
{
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum = Key_GetNum();
		
		if(KeyNum == 1) // 上一項
		{
			stopwatch_flag--;
			if(stopwatch_flag <= 0) stopwatch_flag = 4;
		}
		else if(KeyNum == 2) // 下一項
		{
			stopwatch_flag++;
			if(stopwatch_flag >= 5) stopwatch_flag = 1;
		}
		
		// 確認鍵按下時的邏輯
		else if(KeyNum == 3) 
		{
			if(stopwatch_flag == 1)      // 按下返回鍵
			{
				OLED_Clear();
				OLED_Update();
				return 0; // 退出頁面，背景執行
			}
			else if(stopwatch_flag == 2) // 按下開始鍵
			{
				vTaskResume(TaskStopWatch_Handler); // 喚醒任務，開始計時
			}
			else if(stopwatch_flag == 3) // 按下暫停鍵
			{
				vTaskSuspend(TaskStopWatch_Handler); // 掛起任務，凍結時間
			}
			else if(stopwatch_flag == 4) // 按下歸零鍵
			{
				vTaskSuspend(TaskStopWatch_Handler); // 歸零前先暫停任務，防止變數錯亂
				hour = min = sec = 0;                // 清空變數
			}
		}
		
		OLED_Clear();       // 先清空畫布
		Show_StopWatch_UI(); // 畫上文字與變數
		
		switch(stopwatch_flag)
		{
			case 1: OLED_ReverseArea(0,0,16,16); break;
			case 2: OLED_ReverseArea(8,44,40,16); break; //3 32
			case 3: OLED_ReverseArea(48,44,40,16); break; //3 32
			case 4: OLED_ReverseArea(88,44,40,16); break; //3 32
		}
		
		OLED_Update(); // 一次性更新螢幕
		
		// 放CPU
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void Show_CountDown_UI(void)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	
	// 顯示設定的時間 (Y由 18 移到 8)
	OLED_Printf(32, 8, OLED_8X16, "%02d:%02d:%02d", cd_hour, cd_min, cd_sec);
	
	// 調整按鈕 (Y由 36 移到 28)
	OLED_ShowString(32, 28, "+H", OLED_8X16);
	OLED_ShowString(64, 28, "+M", OLED_8X16);
	OLED_ShowString(96, 28, "+S", OLED_8X16);
	
	// 開始/暫停與清除 (Y由 54 移到 48)
	if(cd_running) OLED_ShowString(16, 48, "Pause", OLED_8X16);
	else           OLED_ShowString(16, 48, "Start", OLED_8X16);
	
	OLED_ShowString(80, 48, "Clear", OLED_8X16);
}

int CountDown(void)
{
	uint8_t cd_cursor = 1;
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum = Key_GetNum();
		
		if(KeyNum == 1) { cd_cursor--; if(cd_cursor <= 0) cd_cursor = 6; }
		else if(KeyNum == 2) { cd_cursor++; if(cd_cursor >= 7) cd_cursor = 1; }
		else if(KeyNum == 3) {
			if(cd_cursor == 1) { OLED_Clear(); OLED_Update(); return 0; } // 返回
			else if(cd_cursor == 2) { cd_hour++; if(cd_hour > 99) cd_hour = 0; }
			else if(cd_cursor == 3) { cd_min++;  if(cd_min > 59) cd_min = 0; }
			else if(cd_cursor == 4) { cd_sec++;  if(cd_sec > 59) cd_sec = 0; }
			else if(cd_cursor == 5) { // 開始/暫停
				if(cd_hour == 0 && cd_min == 0 && cd_sec == 0) continue; 
				if(cd_running == 0) {
					cd_running = 1;
					vTaskResume(TaskCountDown_Handler); // 喚醒倒數任務
				} else {
					cd_running = 0;
					vTaskSuspend(TaskCountDown_Handler); // 掛起任務
				}
			}
			else if(cd_cursor == 6) { // 清除
				vTaskSuspend(TaskCountDown_Handler);
				cd_running = 0;
				cd_hour = 0; cd_min = 0; cd_sec = 0;
			}
		}
		
		OLED_Clear();
		Show_CountDown_UI();
		switch(cd_cursor) {
			case 1: OLED_ReverseArea(0,0,16,16); break;
			case 2: OLED_ReverseArea(32,28,16,16); break;
			case 3: OLED_ReverseArea(64,28,16,16); break;
			case 4: OLED_ReverseArea(96,28,16,16); break;
			case 5: OLED_ReverseArea(16,48,40,16); break;
			case 6: OLED_ReverseArea(80,48,40,16); break;
		}
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

// 手電筒UI
void Show_LED_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(8,20,"OFF",OLED_12X24); // 1 20
	OLED_ShowString(52,20,"ON",OLED_12X24); // 1 72
	OLED_ShowString(84,20,"BRE",OLED_12X24);
}

uint8_t led_flag=1;
// 手電筒選單與呼吸運算邏輯
int LED(void)
{	
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum = Key_GetNum();
		
		// 光標移動 (1:返回, 2:OFF, 3:ON, 4:BRE)
		if(KeyNum == 1) // 上一項
		{
			led_flag--;
			if(led_flag <= 0) led_flag = 4;
		}
		else if(KeyNum == 2) // 下一項
		{
			led_flag++;
			if(led_flag >= 5) led_flag = 1;
		}
		else if(KeyNum == 3) // 按確認鍵返回上一層
		{
			if(led_flag == 1)
			{
				OLED_Clear();
				OLED_Update();
				return 0; // 退出介面(LED維持當前狀態)
			}
		}
		
		// 依當前模式改變硬體輸出
		switch(led_flag)
		{
			case 1:
				// 返回鍵維持目前狀態
				break;
			
			case 2: // OFF
				vTaskSuspend(TaskLED_Handler); // 凍結呼吸任務
				PWM_SetCompare1(0);   // 常滅
				break;
			
			case 3: // ON
				vTaskSuspend(TaskLED_Handler); // 凍結呼吸任務
				PWM_SetCompare1(100); // 全亮
				break;
			
			case 4: // BRE
				vTaskResume(TaskLED_Handler);  // 喚醒呼吸任務在背景運算
                break;
		}
		
		// 畫面繪製與對應光標反白
		OLED_Clear();
		Show_LED_UI();
		switch(led_flag)
		{
			case 1: OLED_ReverseArea(0, 0, 16, 16); break;   // 反白返回鍵
			case 2: OLED_ReverseArea(8, 20, 36, 24); break;  // 反白 OFF (寬 36)
			case 3: OLED_ReverseArea(52, 20, 24, 24); break; // 反白 ON  (寬 24)
			case 4: OLED_ReverseArea(84, 20, 36, 24); break; // 反白 BRE (寬 36)
		}
		OLED_Update();
		
		vTaskDelay(pdMS_TO_TICKS(15));
	}
}

// MPU6050
int16_t ax,ay,az,gx,gy,gz;	// MPU6050測得的三軸加速度和角速度
float roll_g,pitch_g,yaw_g;	// 陀螺儀解算的歐拉角
float roll_a,pitch_a;		// 加速度計解算的歐拉角
float Roll,Pitch,Yaw;		// 互補濾波後的歐拉角
float a=0.9;				// 互補濾波器係數
float Delta_t=0.005;		// 採樣週期
double pi=3.1415927;

// 姿態數據運算
void MPU6050_Calculation(void)
{
	vTaskDelay(pdMS_TO_TICKS(5)); 
	MPU6050_GetData(&ax,&ay,&az,&gx,&gy,&gz);
	
	// 透過陀螺儀解算歐拉角
	roll_g=Roll+(float)gx*Delta_t;
	pitch_g=Pitch+(float)gy*Delta_t;
	yaw_g=Yaw+(float)gz*Delta_t;
	
	// 透過加速度計解算歐拉角
	pitch_a=atan2((-1)*ax,az)*180/pi;
	roll_a=atan2(ay,az)*180/pi;
	
	// 透過互補濾波器進行資料融合
	Roll=a*roll_g+(1-a)*roll_a;
	Pitch=a*pitch_g+(1-a)*pitch_a;
	Yaw=a*yaw_g;
	
}

int MPU6050(void)
{
    static uint32_t current_steps = 0; 
    SensorMsg_t received_msg;

    // 進來頁面時先不等待，把信箱裡最新累積的步數收下來
    if(xQueueReceive(StepQueue_Handle, &received_msg, 0) == pdTRUE) {
        current_steps = received_msg.steps;
    }

    // 剛進來時先畫一次畫面(保證是最新的步數)
    OLED_Clear();
    OLED_ShowImage(0, 0, 16, 16, Return); 
    OLED_Printf(24, 16, OLED_8X16, "Pedometer");
    OLED_Printf(24, 40, OLED_8X16, "Steps:%d", current_steps);
    OLED_ReverseArea(0,0,16,16);
    OLED_Update();

    while(1)
    {	
		Alive_UI = 1; 
        // 是否有按鍵按下要退出
        KeyNum = Key_GetNum();
        if(KeyNum == 3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        
        // 去queue檢查有沒有新包裹
        if(xQueueReceive(StepQueue_Handle, &received_msg, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            // 有 更新步數
            current_steps = received_msg.steps;
            
            // 有更新 重繪畫面
            OLED_Clear();
            OLED_ShowImage(0, 0, 16, 16, Return); 
            OLED_Printf(24, 16, OLED_8X16, "Pedometer");
            OLED_Printf(24, 40, OLED_8X16, "Steps:%d", current_steps);
            OLED_ReverseArea(0,0,16,16);
            OLED_Update();
        }
    }
}

// Game
void Show_Game_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"GoogleDino",OLED_8X16);
}

uint8_t game_flag=1;
// game UI光標移動
int Game(void)
{
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum=Key_GetNum();
		uint8_t game_flag_temp=0;
		if(KeyNum==1)	// 上一項
		{
			game_flag--;
			if(game_flag<=0)game_flag=2;
		}
		else if(KeyNum==2)	// 下一項
		{
			game_flag++;
			if(game_flag>=3)game_flag=1;
		}
		else if(KeyNum==3)	// 確定
		{
			OLED_Clear();
			OLED_Update();
			game_flag_temp=game_flag;
		}
		
		if(game_flag_temp==1){return 0;}
		else if(game_flag_temp==2){DinoGame_Pos_Init();DinoGame_Animation();}
		
		switch(game_flag)
		{
			case 1:
				Show_Game_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;
			
			case 2:
				Show_Game_UI();
				LED_OFF();
				OLED_ReverseArea(0,16,80,16);
				OLED_Update();
				break;
			
			
		
		}
	}
}

//表情包
void Show_Emoji_UI(void)
{	
	// 閉眼
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,10+i,16,16,Eyebrow[0]);//左眉毛
		OLED_ShowImage(82,10+i,16,16,Eyebrow[1]);//右眉毛
		OLED_DrawEllipse(40,32,6,6-i,1);//左眼
		OLED_DrawEllipse(88,32,6,6-i,1);//右眼
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(100)); 
	}
	
	// 睜眼
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,12-i,16,16,Eyebrow[0]);//左眉毛
		OLED_ShowImage(82,12-i,16,16,Eyebrow[1]);//右眉毛
		OLED_DrawEllipse(40,32,6,4+i,1);//左眼
		OLED_DrawEllipse(88,32,6,4+i,1);//右眼
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(100)); 
	}
	
	vTaskDelay(pdMS_TO_TICKS(500)); 
	
}

// 按鍵退出表情包
int Emoji(void)
{
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}
		
		Show_Emoji_UI();
		
	}
}

// 顯示水平儀
void Show_Gradienter_UI(void)
{
	MPU6050_Calculation();
	OLED_DrawCircle(64,32,30,0);
	OLED_DrawCircle(64-Roll,32+Pitch,4,1);
}

// 按鍵退出水平儀
int Gradienter(void)
{
	while(1)
	{	
		Alive_UI = 1; 
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}
		OLED_Clear();
		Show_Gradienter_UI();
		OLED_Update();
	}
}
