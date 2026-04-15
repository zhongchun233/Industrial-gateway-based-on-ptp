#include "DWT_Delay.h"
// 必须在 SystemClock_Config 之后调用一次
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// 简单、粗暴、精准
void DWT_Delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000UL);
    while ((DWT->CYCCNT - start) < ticks);
}

void DWT_Delay_ms(uint32_t ms) {
    DWT_Delay_us(ms * 1000); // 直接复用，没有循环调用的开销
}