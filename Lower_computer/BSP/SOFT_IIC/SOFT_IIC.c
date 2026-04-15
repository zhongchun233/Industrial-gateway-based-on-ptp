#include "SOFT_IIC.h"
#include "DWT_Delay.h"
#include "FreeRTOS.h" 
#include "task.h"     
/* 微秒级延时 */
static void I2C_Delay_us(uint32_t us)
{
    DWT_Delay_us(us);
}

/**
  * @brief  初始化I2C引脚
  */
void Soft_I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* 开启GPIO时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 配置SCL和SDA为开漏输出模式 (Open-Drain) */
    /* 大厂提示：I2C总线必须是开漏模式，配合外部上拉电阻 */
    GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; 
    GPIO_InitStruct.Pull = GPIO_PULLUP; // 内部上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    HAL_GPIO_Init(I2C_SCL_PORT, &GPIO_InitStruct);
    
    /* 初始状态置高 */
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
}

/**
  * @brief  产生起始信号
  */
void Soft_I2C_Start(void)
{
    // 【关键】进入临界区，防止 FreeRTOS 任务切换或中断打断时序
    taskENTER_CRITICAL();
    
    I2C_SDA_1();
    I2C_SCL_1();
    I2C_Delay_us(5); // 建立时间
    
    I2C_SDA_0(); // SDA由高变低，产生起始信号
    I2C_Delay_us(5);
    
    I2C_SCL_0(); // 拉低SCL，钳住总线，准备发送数据
    I2C_Delay_us(5);
}

/**
  * @brief  产生停止信号
  */
void Soft_I2C_Stop(void)
{
    I2C_SDA_0();
    I2C_SCL_0();
    I2C_Delay_us(5);
    
    I2C_SCL_1(); // 先拉高SCL
    I2C_Delay_us(5);
    
    I2C_SDA_1(); // 再拉高SDA，产生停止信号
    I2C_Delay_us(5);
    
    // 【关键】退出临界区，恢复系统调度
    taskEXIT_CRITICAL();
}

/**
  * @brief  发送一个字节
  * @param  byte: 要发送的数据
  * @return 0:成功
  */
uint8_t Soft_I2C_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        // 发送最高位
        if (byte & 0x80)
            I2C_SDA_1();
        else
            I2C_SDA_0();
        
        byte <<= 1; // 左移一位
        
        I2C_SCL_1(); // 拉高SCL，通知从机读取
        I2C_Delay_us(5); // 脉冲宽度
        
        I2C_SCL_0(); // 拉低SCL，准备下一位
        I2C_Delay_us(5);
    }
    return 0;
}

/**
  * @brief  等待应答
  * @return 0:有应答, 1:无应答
  */
uint8_t Soft_I2C_WaitAck(void)
{
    uint8_t ucErrTime = 0;
    
    I2C_SDA_1(); // 释放SDA，让从机控制
    I2C_Delay_us(1);
    
    I2C_SCL_1(); // 拉高SCL
    I2C_Delay_us(1);
    
    // 读取SDA状态
    if (I2C_READ_SDA())
    {
        ucErrTime = 1; // 高电平表示无应答
    }
    else
    {
        ucErrTime = 0; // 低电平表示有应答
    }
    
    I2C_SCL_0(); // 拉低SCL，结束应答检测
    I2C_Delay_us(1);
    
    return ucErrTime;
}

/**
  * @brief  发送应答信号
  * @param  ack: 0=应答, 1=非应答
  */
void Soft_I2C_SendAck(uint8_t ack)
{
    if (ack)
        I2C_SDA_1();
    else
        I2C_SDA_0();
        
    I2C_Delay_us(1);
    I2C_SCL_1();
    I2C_Delay_us(5);
    I2C_SCL_0();
    I2C_Delay_us(1);
}