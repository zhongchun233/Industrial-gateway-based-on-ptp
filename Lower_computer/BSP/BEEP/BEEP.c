#include "BEEP.h"
#include "cmsis_os2.h"
#include "DWT_Delay.h"
#include "FreeRTOS.h"
#include "task.h"
// --- 乐谱定义 (单位 Hz) ---
#define NOTE_C4  1320
#define NOTE_D4  2940
#define NOTE_E4  3300
#define NOTE_F4  3490
#define NOTE_G4  3920
#define NOTE_A4  4400
#define NOTE_B4  4940
#define NOTE_C5  5230
#define NOTE_D5  5870
#define NOTE_E5  6590
#define NOTE_F5  6980
#define NOTE_G5  7840
#define NOTE_A5  8800
#define NOTE_B5  9880
#define NOTE_C6  5047

TaskHandle_t xBEEPTaskHandle = NULL;

static struct {
    BeepMode mode;
    uint32_t tick;
    bool beeping;
} beep_ctrl = {0};

void bsp_beep_on(void)
{
    HAL_GPIO_WritePin(PRJ_BEEP_PORT, PRJ_BEEP_PIN, GPIO_PIN_SET);
}

void bsp_beep_off(void)
{
    HAL_GPIO_WritePin(PRJ_BEEP_PORT, PRJ_BEEP_PIN, GPIO_PIN_RESET);
}

// --- 辅助函数：播放单音 ---
// freq: 频率 (Hz), duration_ms: 持续时间 (ms)
void beep_play_tone(uint32_t freq, uint32_t duration_ms)
{
    if (freq == 0) {
        bsp_beep_off();
        DWT_Delay_ms(duration_ms);
        return;
    }

    uint32_t period_us = 1000000 / freq; // 计算周期 (us)
    uint32_t half_period_us = period_us / 2; // 半周期 (高电平或低电平时间)
    
    // 计算需要循环多少次才能达到持续时间
    // 总时间(us) / 每次循环时间(us)
    uint32_t loops = (duration_ms * 1000) / period_us;

    for (uint32_t i = 0; i < loops; i++)
    {
        bsp_beep_on();
        DWT_Delay_us(half_period_us);
        bsp_beep_off();
        DWT_Delay_us(half_period_us);
    }
}
void bsp_beep_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = PRJ_BEEP_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(PRJ_BEEP_PORT, &gpio);
    bsp_beep_off();
    beep_play_tone(NOTE_E4, 150); // 150ms
    beep_play_tone(NOTE_C4, 150);
    beep_play_tone(NOTE_C5, 150);
		bsp_beep_off();
//    beep_play_tone(NOTE_C5, 300); // 最后一个音长一点
}


// 初始化
void beep_init(void)
{
    bsp_beep_init();
    beep_ctrl.mode = BEEP_OFF;
    beep_ctrl.beeping = false;
    beep_ctrl.tick = 0;
	    xTaskCreate(
        beep_task,            // 任务函数
        "beep_task",          // 任务名称 (调试用)
        256,                  // 堆栈大小 
        NULL,                 // 任务参数
        1,                    // 任务优先级
        &xBEEPTaskHandle      // 任务句柄
    );
}

// 设置告警模式
void beep_set_mode(BeepMode mode)
{
    if (mode == beep_ctrl.mode) return;

    beep_ctrl.mode = mode;
    beep_ctrl.tick = 0;
    bsp_beep_off();
    beep_ctrl.beeping = false;
}

// 获取当前模式
BeepMode beep_get_mode(void)
{
    return beep_ctrl.mode;
}


// 蜂鸣器控制任务（1ms 调度）
void beep_task(void *argument)
{
    const uint32_t PERIOD = 1;
    // 2. 播放开机音乐 (只播放一次)
    // 旋律：C4 -> E4 -> G4 -> C5 (Do -> Mi -> Sol -> 高Do)
	
//    bsp_beep_on();
//	HAL_Delay(100);
//    beep_play_tone(1000, 50); // 150ms
//    beep_play_tone(2000, 150);
//    beep_play_tone(3000, 150);
//    beep_play_tone(4000, 100); // 最后一个音长一点
//	 bsp_beep_on();
//	HAL_Delay(1000);
    
    for (;;) {
        uint32_t now = HAL_GetTick();
        BeepMode m = beep_ctrl.mode;

        switch (m) {
            case BEEP_OFF:
                bsp_beep_off();
                break;

            case BEEP_ON:
                bsp_beep_on();
                break;

            case BEEP_SHORT:
                if (beep_ctrl.tick < 50) bsp_beep_on();
                else {
                    bsp_beep_off();
                    beep_ctrl.mode = BEEP_OFF;
                }
                break;

            case BEEP_LONG:
                if (beep_ctrl.tick < 500) bsp_beep_on();
                else {
                    bsp_beep_off();
                    beep_ctrl.mode = BEEP_OFF;
                }
                break;

            case BEEP_ALARM:
                if (!beep_ctrl.beeping && beep_ctrl.tick >= 100) {
                    bsp_beep_on();
                    beep_ctrl.beeping = true;
                    beep_ctrl.tick = 0;
                } else if (beep_ctrl.beeping && beep_ctrl.tick >= 50) {
                    bsp_beep_off();
                    beep_ctrl.beeping = false;
                    beep_ctrl.tick = 0;
                }
                break;

            case BEEP_WARN:
                if (!beep_ctrl.beeping && beep_ctrl.tick >= 800) {
                    bsp_beep_on();
                    beep_ctrl.beeping = true;
                    beep_ctrl.tick = 0;
                } else if (beep_ctrl.beeping && beep_ctrl.tick >= 200) {
                    bsp_beep_off();
                    beep_ctrl.beeping = false;
                    beep_ctrl.tick = 0;
                }
                break;
        }

        beep_ctrl.tick += PERIOD;
				now += PERIOD;
				osDelayUntil(now);
    }
}