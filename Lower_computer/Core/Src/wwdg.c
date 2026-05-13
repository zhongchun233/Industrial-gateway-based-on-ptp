#include "wwdg.h"


WWDG_HandleTypeDef hwwdg;

void WWDG_Init(void)
{
	// 1. 开 WWDG 时钟
	__HAL_RCC_WWDG1_CLK_ENABLE();

	// 2. 配置参数
	hwwdg.Instance = WWDG1;
	hwwdg.Init.Prescaler = WWDG_PRESCALER_8;	// 分频 8
	hwwdg.Init.Window = 127;					// 窗口值 0x7F
	hwwdg.Init.Counter = 127;					// 重载值 0x7F
	hwwdg.Init.EWIMode = WWDG_EWI_DISABLE;		// 关闭提前唤醒中断

	// 3. 初始化 WWDG → 初始化后立即开始运行
	if (HAL_WWDG_Init(&hwwdg) != HAL_OK)
	{
		Error_Handler();
	}
}
