#ifndef __DWT_Delay_H__  //Avoid repeated including same files later
#define __DWT_Delay_H__

#include "main.h"
void DWT_Init(void);
void DWT_Delay_us(uint32_t us);
void DWT_Delay_ms(uint32_t ms);
#endif /* __EC_BSP_AHT21_DRIVER_H__ */
