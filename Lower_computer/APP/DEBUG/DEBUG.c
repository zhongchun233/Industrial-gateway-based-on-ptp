#include "DEBUG.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"    // 用于 sprintf

// 任务句柄 (可选，用于在其他地方控制任务)
TaskHandle_t xDEBUGTaskHandle = NULL;

/**
  * @brief  OLED 显示任务
  * @param  pvParameters: 任务参数
  */
void DEBUG_Task(void *pvParameters)
{



	for(;;)
    {
				HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
        // --- 延时 ---
        // 使用 FreeRTOS 的延时，释放 CPU 给其他任务
        osDelay(pdMS_TO_TICKS(500)); 
    }
}

/**
  * @brief  创建 OLED 任务
  *      
  */
void Create_DEBUG_Task(void)
{
    xTaskCreate(
        DEBUG_Task,            // 任务函数
        "DEBUG_Task",          // 任务名称 (调试用)
        256,                  // 堆栈大小 
        NULL,                 // 任务参数
        1,                    // 任务优先级
        &xDEBUGTaskHandle      // 任务句柄
    );
}