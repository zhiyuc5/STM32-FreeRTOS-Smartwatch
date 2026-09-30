# STM32 FreeRTOS 多工智慧手錶原型系統

基於 ARM Cortex-M3 (STM32F103C8T6) 與 FreeRTOS 即時作業系統開發，調度 10 項獨立任務與多重周邊協同運作。

---

## 系統硬體規格與記憶體分配 (System Resources)

* **主控架構**: STM32F103C8T6 (ARM Cortex-M3 @ 72MHz, 20KB SRAM, 64KB Flash)
* **作業系統**: FreeRTOS (Heap_4 動態記憶體管理)
* **除錯工具**: ST-Link V2、USB Logic Analyzer
* **SRAM 配置**: FreeRTOS Heap 配置為 14KB（佔 SRAM 70%）；保留 6KB 供中斷服務堆疊 (MSP)、顯存緩衝與系統全域變數

* **硬體周邊與晶片資源**:
  * 0.96 吋 OLED 螢幕 (軟體模擬 I2C)
  * MPU6050 六軸感測器 (軟體模擬 I2C)
  * HC-05 藍牙模組 (USART1)
  * 實體控制按鍵 (支援介面操作與息屏中斷喚醒)
  * 蜂鳴器模組 (鬧鐘與倒數計時警報，採 Mutex 互斥存取)
  * NTC 熱敏電阻 (ADC1，Beta 方程式解算)
  * 手電筒 LED (TIM1 高級定時器硬體 PWM)
  * 內部 On-Chip Flash (末頁模擬 EEPROM，斷電儲存鬧鐘配置)
  * IWDG (3 秒超時自癒防護)

---

## FreeRTOS 任務架構與 IPC 機制

| 任務名稱 | 優先權 | 通訊 / 同步機制 | 核心職責 |
| :--- | :---: | :--- | :--- |
| **Task_Monitor** | 4 | 狀態旗標輪詢 | 看門狗巡邏，整合息屏放行機制執行硬體 IWDG 餵狗 |
| **Task_Bluetooth** | 3 | Task Notification | USART 接收封包中斷觸發，非阻塞解析遠端對時指令與鬧鐘設定 |
| **Task_MPU6050** | 2 | xQueueOverwrite (計步 Mailbox) | 20ms 週期採樣六軸數據，並將計步濾波結果覆蓋傳遞至 UI |
| **Task_Alarm** | 2 | Buzzer_Mutex | 1000ms 週期比對系統時間，取得蜂鳴器鎖並同步回寫 Flash |
| **Task_Temp** | 2 | 全域整數共享 | 5000ms 採樣 ADC，以 Beta 方程式解算環境溫度 |
| **Task_LED** | 2 | Suspend / Resume | 20ms 時基調整 PWM 佔空比，支援LED開關與呼吸燈 |
| **Task_StopWatch** | 2 | Suspend / Resume | 1000ms 背景碼錶計時，由 UI 動態掛起與喚醒 |
| **Task_CountDown** | 2 | Buzzer_Mutex | 1000ms 倒數計時，時間抵達死等蜂鳴器並喚醒螢幕 |
| **Task_Power** | 1 | 全域變數檢查 | 閒置達 15 秒觸發息屏，並掛起高耗能 UI 任務 |
| **Task_UI** | 1 | 按鍵輪詢 / 接收 Queue | 處理滑動選單、UI介面、水平儀與遊戲 |

---

## 系統實機展示 (Demo Video)

* 📺 **[點此前往觀看實機展示影片 (YouTube - 2分35秒)](https://youtu.be/mBV8BFTWNzg)**
