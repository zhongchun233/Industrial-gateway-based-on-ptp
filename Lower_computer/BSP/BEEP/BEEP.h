#ifndef __BEEP_H__  //Avoid repeated including same files later
#define __BEEP_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
/* ================= 用户配置区 ================= */
// 在这里修改引脚，适配你的开发板
#define PRJ_BEEP_PORT    GPIOB
#define PRJ_BEEP_PIN     GPIO_PIN_2

/* ============================================= */


// 告警模式（工业标准）
typedef enum {
    BEEP_OFF,
    BEEP_ON,
    BEEP_SHORT,       // 短滴 50ms
    BEEP_LONG,        // 长滴 500ms
    BEEP_ALARM,       // 紧急：50ms 响 / 100ms 停
    BEEP_WARN         // 警告：200ms 响 / 800ms 停
} BeepMode;

void beep_init(void);
void beep_set_mode(BeepMode mode);
BeepMode beep_get_mode(void);
void beep_task(void *argument);

#endif 
