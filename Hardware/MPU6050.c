#include "stm32f10x.h"                  // Device header
#include "MyI2C.h"
#include "MPU6050_Reg.h"

#define MPU6050_ADDRESS		0xD0		// MPU6050的I2C從機地址

/**
  * 函 數：MPU6050寫入暫存器
  * 參 數：RegAddress 暫存器位址，範圍：參考MPU6050手冊的暫存器描述
  * 參 數：Data 要寫入暫存器的數據，範圍：0x00~0xFF
  * 返 回 值：無
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	MyI2C_Start(); 						// I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS); 	// 傳送從機位址，讀寫位元為0，表示即將寫入
	MyI2C_ReceiveAck(); 				// 接收應答
	MyI2C_SendByte(RegAddress); 		// 傳送暫存器位址
	MyI2C_ReceiveAck(); 				// 接收應答
	MyI2C_SendByte(Data); 				// 傳送要寫入暫存器的數據
	MyI2C_ReceiveAck(); 				// 接收應答
	MyI2C_Stop(); 						// I2C終止
}

/**
  * 函 數：MPU6050讀取暫存器
  * 參 數：RegAddress 暫存器位址，範圍：參考MPU6050手冊的暫存器描述
  * 返 回 值：讀取暫存器的數據，範圍：0x00~0xFF
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;
	
	MyI2C_Start(); 							// I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS); 		// 傳送從機位址，讀寫位元為0，表示即將寫入
	MyI2C_ReceiveAck(); 					// 接收應答
	MyI2C_SendByte(RegAddress); 			// 傳送暫存器位址
	MyI2C_ReceiveAck(); 					// 接收應答
	
	MyI2C_Start(); 							// I2C重複起始
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01); // 傳送從機位址，讀寫位元為1，表示即將讀取
	MyI2C_ReceiveAck(); 					// 接收應答
	Data = MyI2C_ReceiveByte(); 			// 接收指定暫存器的數據
	MyI2C_SendAck(1); 						// 傳送應答，給從機非應答，終止從機的資料輸出
	MyI2C_Stop(); 							// I2C終止
	
	return Data;
}

/**
  * 函 數：MPU6050初始化
  * 參 數：無
  * 返 回 值：無
  */
void MPU6050_Init(void)
{
	MyI2C_Init();									//先初始化底层的I2C
	
	/*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);		//采样率分频寄存器，配置采样率
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			//配置寄存器，配置DLPF
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}

/**
  * 函    数：MPU6050获取ID号
  * 参    数：无
  * 返 回 值：MPU6050的ID号
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		//返回WHO_AM_I寄存器的值
}

/**
  * 函 數：MPU6050取得數據
  * 參 數：AccX AccY AccZ 加速度計X、Y、Z軸的數據，使用輸出參數的形式返回，範圍：-32768~32767
  * 參 數：GyroX GyroY GyroZ 陀螺儀X、Y、Z軸的數據，使用輸出參數的形式返回，範圍：-32768~32767
  * 返 回 值：無
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t DataH, DataL;								// 定義資料高8位和低8位的變數
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);		// 讀取加速度計X軸的高8位元數據
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);		// 讀取加速度計X軸的低8位元數據
	*AccX = (DataH << 8) | DataL;						// 資料拼接，透過輸出參數返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);		// 讀取加速度計Y軸的高8位元數據
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);		// 讀取加速度計Y軸的低8位元數據
	*AccY = (DataH << 8) | DataL;						// 資料拼接，透過輸出參數返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);		// 讀取加速度計Z軸的高8位元數據
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);		// 讀取加速度計Z軸的低8位元數據
	*AccZ = (DataH << 8) | DataL;						// 資料拼接，透過輸出參數返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);		// 讀取陀螺儀X軸的高8位元數據
	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);		// 讀取陀螺儀X軸的低8位元數據
	*GyroX = (DataH << 8) | DataL;						// 資料拼接，透過輸出參數返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);		// 讀取陀螺儀Y軸的高8位數據
	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);		// 讀取陀螺儀Y軸的低8位元數據
	*GyroY = (DataH << 8) | DataL;						// 資料拼接，透過輸出參數返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);		// 讀取陀螺儀Z軸的高8位元數據
	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);		// 讀取陀螺儀Z軸的低8位元數據
	*GyroZ = (DataH << 8) | DataL;						// 資料拼接，透過輸出參數返回
}
