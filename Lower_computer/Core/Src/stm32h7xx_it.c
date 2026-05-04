/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED_SHOW.h"
#include "DEBUG.h"
#include "shell_port.h"
#include "BEEP.h"
#include "DWT_Delay.h"
#include "USB.h"
#include "BSP_RTC.h"
#include "quadspi.h"
#include "w25q128.h"
#include "FLASH.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern HCD_HandleTypeDef hhcd_USB_OTG_FS;
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */
__asm void my_HardFault_Handler(void)
{
    extern HardFault_Handler_C;

    TST LR, #4    // if(lr[2]==1)
    ITE EQ 
    MRSEQ R0, MSP // lr[2] == 0, 
    MRSNE R0, PSP // lr[2] == 1, 
    B HardFault_Handler_C // 跳转到C函数
}
void HardFault_Handler_C(uint32_t *stack)
{
  shellPrint(&shell, "HardFault_Handler_C is running\r\n");
  shellPrint(&shell,"R0:0X%08x\r\n", stack[0]);
  shellPrint(&shell,"R1:0X%08x\r\n", stack[1]);
  shellPrint(&shell,"R2:0X%08x\r\n", stack[2]);
  shellPrint(&shell,"R3:0X%08x\r\n", stack[3]);
  shellPrint(&shell,"R12:0X%08x\r\n", stack[4]);
  shellPrint(&shell,"LR:0X%08x\r\n", stack[5]);
  shellPrint(&shell,"PC:0X%08x\r\n", stack[6]);
  shellPrint(&shell,"xPSR:0X%08x\r\n", stack[7]);

  // ========== 错误日志==========
  Fault_Record_t fault = {0};

  // 记录时间
  fault.time.year = 26;
  fault.time.month = 5;
  fault.time.day = 3;
  fault.time.hour = 0;
  fault.time.minute = 0;
  fault.time.second = 0;
  fault.time.millisecond = 0;

  // 记录错误信息
  fault.fault_code = 0x0001;  // 自定义的硬件错误代码
  fault.fault_level = 3;      // 错误级别
  fault.version_major = 1;
  fault.version_minor = 0;
  fault.fault_data = stack[6]; // PC地址

  // 格式化错误信息

  snprintf((char *)fault.fault_desc,
  sizeof(fault.fault_desc),
               "HF: R0=0x%08X\r\n"
               "R1=0x%08X\r\n"
               "R2=0x%08X\r\n"
               "R3=0x%08X\r\n"
               "R12=0x%08X\r\n"
               "LR=0x%08X\r\n"
               "PC=0x%08X\r\n"
               "xPSR=0x%08X\r\n",
               stack[0], 
               stack[1], 
               stack[2],
               stack[3],
               stack[4],
               stack[5],
               stack[6],
               stack[7]);

  // 将错误记录写入Flash
  if(FLASH_AddFaultRecord(&fault) != FLASH_OK) {
      shellPrint(&shell,"HardFault_Handler_C: Failed to add fault record\r\n");
  } else {
      shellPrint(&shell,"HardFault_Handler_C: Fault record added successfully\r\n");
  }

    // 分析函数调用栈，尝试找出可能的错误函数地址
  uint32_t *funcStackPointer = (uint32_t*)((uint32_t)stack + 8);
  shellPrint(&shell,"funcStack:\r\n");
  
  for (int i = 0; i < 1024; i++) {
      // 判断是否为有效的代码地址（假设代码区在0x08000000-0x080FFFFF）
      bool isCodeAddress = ((*funcStackPointer & 0xFFFF0000) == 0x08000000);

      uint32_t *previousAddress = (uint32_t *)(*funcStackPointer - 4 - 1);
      // 判断前一条指令是否为BL指令（ARM Thumb-2指令集中的BL指令编码为0xF000F800）
      bool ThePreviousIsBL = ((*previousAddress & 0xf800f000) == 0xf800f000);
  
      // 如果是有效的代码地址且前一条指令是BL指令，则认为这是一个可能的错误函数地址
      if (isCodeAddress && ThePreviousIsBL) {
          shellPrint(&shell,"reg val: 0x%08x\r\n", *funcStackPointer);
      }

      funcStackPointer++;
  }


  while(1);

}

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
//void HardFault_Handler(void)
//{
//  /* USER CODE BEGIN HardFault_IRQn 0 */
//    extern HardFault_Handler_C

//    TST LR, #4
//    ITE EQ
//    MRSEQ R0, MSP
//    MRSNE R0, PSP
//    B HardFault_Handler_C
//  	// shellPrint(&shell,"HardFault_Handler!!!\r\n");
//		// my_HardFault_Handler();
////	__asm volatile(
////    "tst lr, #4\n"
////    "ite eq\n"
////    "mrseq r0, msp\n"
////    "mrsne r0, psp\n"
////    "b HardFault_Handler_C\n"
////);
//  /* USER CODE END HardFault_IRQn 0 */
//  while (1)
//  {
//    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
//    /* USER CODE END W1_HardFault_IRQn 0 */
//  }
//}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM1 update interrupt.
  */
void TIM1_UP_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_IRQn 0 */

  /* USER CODE END TIM1_UP_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_IRQn 1 */

  /* USER CODE END TIM1_UP_IRQn 1 */
}

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */

  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

  /* USER CODE END USART3_IRQn 1 */
}

/**
  * @brief This function handles USB On The Go FS End Point 1 Out global interrupt.
  */
void OTG_FS_EP1_OUT_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_EP1_OUT_IRQn 0 */

  /* USER CODE END OTG_FS_EP1_OUT_IRQn 0 */
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_EP1_OUT_IRQn 1 */

  /* USER CODE END OTG_FS_EP1_OUT_IRQn 1 */
}

/**
  * @brief This function handles USB On The Go FS End Point 1 In global interrupt.
  */
void OTG_FS_EP1_IN_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_EP1_IN_IRQn 0 */

  /* USER CODE END OTG_FS_EP1_IN_IRQn 0 */
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_EP1_IN_IRQn 1 */

  /* USER CODE END OTG_FS_EP1_IN_IRQn 1 */
}

/**
  * @brief This function handles USB OTG FS global interrupt.
  */
void OTG_FS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_IRQn 0 */

  /* USER CODE END OTG_FS_IRQn 0 */
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_IRQn 1 */

  /* USER CODE END OTG_FS_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
