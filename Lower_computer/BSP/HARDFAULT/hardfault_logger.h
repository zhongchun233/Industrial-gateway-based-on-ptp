/**
 * @file    hardfault_logger.h
 * @brief   HardFault异常捕获和日志记录
 * @author  Kiro
 * @date    2026-05-03
 */

#ifndef __HARDFAULT_LOGGER_H__
#define __HARDFAULT_LOGGER_H__

#include <stdint.h>

/* ============ HardFault信息结构 ============ */

typedef struct __attribute__((packed)) {
    /* Cortex-M7寄存器 */
    uint32_t r0;        // 参数/返回值
    uint32_t r1;        // 参数
    uint32_t r2;        // 参数
    uint32_t r3;        // 参数
    uint32_t r12;       // 中间寄存器
    uint32_t lr;        // 返回地址
    uint32_t pc;        // 程序计数器（异常发生地址）
    uint32_t xpsr;      // 程序状态寄存器

    /* 故障信息 */
    uint32_t cfsr;      // 可配置故障状态寄存器
    uint32_t hfsr;      // 硬故障状态寄存器
    uint32_t dfsr;      // 调试故障状态寄存器
    uint32_t afsr;      // 辅助故障状态寄存器
    uint32_t mmar;      // 内存管理故障地址
    uint32_t bfar;      // 总线故障地址

    /* 时间戳 */
    uint32_t timestamp; // 异常发生时间
} HardFault_Info_t;

/* ============ 函数声明 ============ */

/**
 * @brief  初始化HardFault日志系统
 */
void HardFault_Logger_Init(void);

/**
 * @brief  记录HardFault信息到故障日志
 * @param  fault_info: HardFault信息指针
 */
void HardFault_Logger_Save(HardFault_Info_t *fault_info);

/**
 * @brief  获取HardFault信息描述
 * @param  fault_info: HardFault信息指针
 * @param  desc: 描述缓冲区
 * @param  len: 缓冲区大小
 */
void HardFault_Logger_GetDescription(HardFault_Info_t *fault_info,
                                     char *desc, uint16_t len);

/**
 * @brief  分析HardFault原因
 * @param  fault_info: HardFault信息指针
 * @retval 故障代码
 */
uint16_t HardFault_Logger_Analyze(HardFault_Info_t *fault_info);

#endif /* __HARDFAULT_LOGGER_H__ */
