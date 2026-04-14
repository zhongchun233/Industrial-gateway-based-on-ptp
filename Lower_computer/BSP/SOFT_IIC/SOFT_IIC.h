#ifndef __SOFT_IIC_H__  //Avoid repeated including same files later
#define __SOFT_IIC_H__

#include "main.h"

/* ================= 用户配置区 ================= */
// 在这里修改引脚，适配你的开发板
#define I2C_SCL_PORT    OLED_SCL_GPIO_Port
#define I2C_SCL_PIN     OLED_SCL_Pin
#define I2C_SDA_PORT    OLED_SDA_GPIO_Port
#define I2C_SDA_PIN     OLED_SDA_Pin
/* ============================================= */

/* 宏定义：IO操作 */
//用宏封装底层，方便移植
#define I2C_SCL_0()     HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET)
#define I2C_SCL_1()     HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET)
#define I2C_SDA_0()     HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET)
#define I2C_SDA_1()     HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET)

// 读取SDA引脚电平
#define I2C_READ_SDA()  HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN)

/* 函数声明 */
void Soft_I2C_Init(void);
void Soft_I2C_Start(void);
void Soft_I2C_Stop(void);
uint8_t Soft_I2C_SendByte(uint8_t byte);
uint8_t Soft_I2C_WaitAck(void);
void Soft_I2C_SendAck(uint8_t ack);
#endif /* __EC_BSP_AHT21_DRIVER_H__ */
