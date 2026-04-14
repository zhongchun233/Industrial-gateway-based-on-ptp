#include "OLED_SHOW.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h" // 包含刚才修正的 OLED 头文件
#include "stdio.h"    // 用于 sprintf

// 任务句柄 (可选，用于在其他地方控制任务)
TaskHandle_t xOledTaskHandle = NULL;

/**
  * @brief  OLED 显示任务
  * @param  pvParameters: 任务参数
  */
void OLED_Task(void *pvParameters)
{
    uint32_t count = 0;
    uint8_t load = 0;
    char buffer[20]; // 缓冲区，用于格式化字符串

    // 1. 初始化 OLED (只执行一次)
    // 注意：Soft_I2C_Init() 内部已经初始化了 GPIO
    // 如果你的 I2C 初始化在 main.c 做过了，这里可以只调用 OLED_Init()
    OLED_Init();
    
    // 开机欢迎语
    OLED_ShowString(0, 0, "Gateway System", OLED_8X16);
    OLED_ShowString(0, 2, "Status: OK", OLED_8X16);
    OLED_Update(); // 刷新显存到屏幕
    vTaskDelay(pdMS_TO_TICKS(1000)); // 显示1秒后清空

    for(;;)
    {
        // --- 模拟业务逻辑 ---
        count++;
        load = (count % 100); // 模拟一个 0-99 的负载值

        // --- 准备显示数据 ---
        
        // 第 0 行：显示计数
        // 格式： "Count: 00000"
        sprintf(buffer, "Count: %ld", count);
        OLED_ShowString(0, 0, buffer, OLED_8X16);

        // 第 2 行：显示模拟负载
        // 格式： "Load:  XX %"
        sprintf(buffer, "Load:  %d %%", load);
        OLED_ShowString(0, 2, buffer, OLED_8X16);
        // 第 4 行：显示任务状态
        // 格式： "Task: Running"
        OLED_ShowString(0, 4, "Task: Running ", OLED_8X16);

        // --- 刷新显示 ---
        // 注意：I2C 传输比较慢，这里会占用一点时间
        OLED_Update(); 

        // --- 延时 ---
        // 使用 FreeRTOS 的延时，释放 CPU 给其他任务
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }
}

/**
  * @brief  创建 OLED 任务
  *         建议在 main.c 的 vTaskStartScheduler() 之前调用
  */
void Create_OLED_Task(void)
{
    xTaskCreate(
        OLED_Task,            // 任务函数
        "OLED_Task",          // 任务名称 (调试用)
        256,                  // 堆栈大小 (根据 sprintf 的复杂度，256 比较安全)
        NULL,                 // 任务参数
        2,                    // 任务优先级 (2 表示普通优先级)
        &xOledTaskHandle      // 任务句柄
    );
}