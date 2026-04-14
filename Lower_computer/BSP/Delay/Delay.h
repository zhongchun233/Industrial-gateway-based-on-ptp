#ifndef __DELAY_H__  //Avoid repeated including same files later
#define __DELAY_H__

#include "main.h"
void DWT_Init(void);
void DWT_Delay_us(uint32_t us);
#endif /* __EC_BSP_AHT21_DRIVER_H__ */
