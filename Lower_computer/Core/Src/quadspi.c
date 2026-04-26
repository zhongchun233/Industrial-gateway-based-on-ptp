/**
 * quadspi.c — W25Q128 QUADSPI 外设初始化
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  TODO: 根据你的硬件原理图修改下面的 GPIO 引脚分配
 *
 *  STM32H743 常用 QUADSPI 引脚 (Bank1):
 *    CLK  → PB2  (AF9)  或  PF10 (AF9)
 *    NCS  → PB6  (AF10) 或  PG6  (AF10)
 *    IO0  → PC9  (AF9)  或  PD11 (AF9)  或 PF8 (AF10)
 *    IO1  → PC10 (AF9)  或  PD12 (AF9)  或 PF9 (AF10)
 *    IO2  → PE2  (AF9)  或  PF7  (AF9)
 *    IO3  → PA1  (AF9)  或  PD13 (AF9)  或 PF6 (AF9)
 *
 *  默认使用: PB2(CLK), PB6(NCS), PD11(IO0), PD12(IO1), PE2(IO2), PD13(IO3)
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

#include "quadspi.h"

QSPI_HandleTypeDef hqspi;

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi_in)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* CLK: PB2 (AF9) */
    gpio.Pin       = GPIO_PIN_2;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* NCS: PB6 (AF10) */
    gpio.Pin       = GPIO_PIN_6;
    gpio.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* IO0: PD11 (AF9) */
    gpio.Pin       = GPIO_PIN_11;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &gpio);

    /* IO1: PD12 (AF9) */
    gpio.Pin       = GPIO_PIN_12;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &gpio);

    /* IO2: PE2 (AF9) */
    gpio.Pin       = GPIO_PIN_2;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOE, &gpio);

    /* IO3: PD13 (AF9) */
    gpio.Pin       = GPIO_PIN_13;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &gpio);
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *hqspi_in)
{
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2 | GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);
}

void MX_QUADSPI_Init(void)
{
    hqspi.Instance = QUADSPI;

    /*
     * ClockPrescaler: QSPI_CLK = AHB3_CLK / (Prescaler + 1)
     * STM32H743 典型: AHB3=240MHz, Prescaler=1 → QSPI=120MHz
     * TODO: 根据你的时钟树调整
     */
    hqspi.Init.ClockPrescaler     = 1;
    hqspi.Init.FifoThreshold      = 4;
    hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize           = 23;   /* 2^(23+1) = 16MB */
    hqspi.Init.ChipSelectHighTime  = QSPI_CS_HIGH_TIME_2_CYCLE;
    hqspi.Init.ClockMode           = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID             = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash           = QSPI_DUALFLASH_DISABLE;

    if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
        Error_Handler();
    }
}
