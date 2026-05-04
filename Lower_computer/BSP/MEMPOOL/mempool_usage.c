/**
 * @file    mempool_usage.c
 * @brief   内存池使用示例 - 工业级生产代码
 * @author  Kiro
 * @date    2026-05-03
 */

#include "mempool.h"
#include "FLASH.h"

/* ============ 初始化示例 ============ */

void System_Init(void) {
    /* 初始化内存池系统 */
    if (MemPool_Init() != MEMPOOL_OK) {
        // 处理初始化失败
        while (1);
    }

    /* 初始化Flash模块（现在使用内存池） */
    FLASH_Init();
}

/* ============ Flash模块使用示例 ============ */

void Flash_Example(void) {
    Fault_Record_t fault = {
        .time = {
            .year = 26,
            .month = 5,
            .day = 3,
            .hour = 14,
            .minute = 30,
            .second = 45,
            .millisecond = 123,
        },
        .fault_code = 0x0001,
        .fault_level = 2,  // error
        .version_major = 1,
        .version_minor = 0,
        .fault_data = 0x12345678,
    };
    strcpy((char *)fault.fault_desc, "HardFault detected");

    /* 添加故障记录（内部使用内存池） */
    if (FLASH_AddFaultRecord(&fault) != FLASH_OK) {
        // 处理错误
    }
}

/* ============ 网络模块使用示例 ============ */

void Network_Example(void) {
    /* 分配网络缓冲区 */
    uint8_t *net_buf = (uint8_t *)MemPool_Alloc(MEMPOOL_TYPE_NET);
    if (net_buf == NULL) {
        // 处理分配失败
        return;
    }

    /* 使用缓冲区接收数据 */
    // HAL_UART_Receive_DMA(huart, net_buf, MEMPOOL_NET_SIZE);

    /* 处理数据 */
    // process_network_data(net_buf);

    /* 释放缓冲区 */
    if (MemPool_Free(MEMPOOL_TYPE_NET, net_buf) != MEMPOOL_OK) {
        // 处理释放失败
    }
}

/* ============ 日志模块使用示例 ============ */

void Log_Example(void) {
    /* 分配日志缓冲区 */
    uint8_t *log_buf = (uint8_t *)MemPool_Alloc(MEMPOOL_TYPE_LOG);
    if (log_buf == NULL) {
        return;
    }

    /* 格式化日志 */
    // sprintf((char *)log_buf, "System running, uptime=%d\n", get_uptime());

    /* 发送日志 */
    // send_log(log_buf);

    /* 释放缓冲区 */
    MemPool_Free(MEMPOOL_TYPE_LOG, log_buf);
}

/* ============ DMA模块使用示例 ============ */

void DMA_Example(void) {
    /* 分配DMA缓冲区（必须在SRAM中） */
    uint8_t *dma_buf = (uint8_t *)MemPool_Alloc(MEMPOOL_TYPE_DMA);
    if (dma_buf == NULL) {
        return;
    }

    /* 配置DMA传输 */
    // HAL_DMA_Start(hdma, src_addr, (uint32_t)dma_buf, MEMPOOL_DMA_SIZE);

    /* 等待DMA完成 */
    // while (HAL_DMA_GetState(hdma) != HAL_DMA_STATE_READY);

    /* 处理数据 */
    // process_dma_data(dma_buf);

    /* 释放缓冲区 */
    MemPool_Free(MEMPOOL_TYPE_DMA, dma_buf);
}

/* ============ 监控和调试示例 ============ */

void Monitor_MemPool(void) {
    /* 健康检查 */
    if (MemPool_HealthCheck() != MEMPOOL_OK) {
        // 内存池异常
        // 可能的原因：
        // 1. 内存泄漏
        // 2. 缓冲区溢出
        // 3. 双重释放
    }

    /* 获取统计信息 */
    MemPool_Manager_t *stats = MemPool_GetStats(MEMPOOL_TYPE_FLASH);
    if (stats != NULL) {
        // printf("Flash Pool: Used=%d/%d, Peak=%d, Fail=%d\n",
        //        stats->used_count, stats->block_count,
        //        stats->peak_count, stats->fail_count);
    }

    /* 打印调试信息 */
    MemPool_PrintDebugInfo(MEMPOOL_TYPE_MAX);  // 打印所有池的信息

    /* 重置统计信息 */
    // MemPool_ResetStats(MEMPOOL_TYPE_MAX);
}

/* ============ 错误处理示例 ============ */

void Error_Handling_Example(void) {
    uint8_t *buf = (uint8_t *)MemPool_Alloc(MEMPOOL_TYPE_NET);

    if (buf == NULL) {
        /* 分配失败处理 */
        // 1. 记录错误
        // 2. 尝试释放其他不必要的缓冲区
        // 3. 降级处理
        return;
    }

    /* 使用缓冲区 */
    // ...

    /* 释放缓冲区 */
    uint8_t ret = MemPool_Free(MEMPOOL_TYPE_NET, buf);
    if (ret != MEMPOOL_OK) {
        /* 释放失败处理 */
        switch (ret) {
            case MEMPOOL_ERR_INVALID_PTR:
                // 指针无效
                break;
            case MEMPOOL_ERR_MAGIC:
                // 魔数错误，可能是缓冲区溢出
                break;
            case MEMPOOL_ERR_DOUBLE_FREE:
                // 双重释放
                break;
            default:
                break;
        }
    }
}

/* ============ 多任务使用示例 ============ */

/* 任务1：网络接收 */
void Task_NetworkRx(void *arg) {
    while (1) {
        uint8_t *buf = (uint8_t *)MemPool_Alloc(MEMPOOL_TYPE_NET);
        if (buf != NULL) {
            /* 接收数据 */
            // receive_data(buf);

            /* 发送到队列给其他任务处理 */
            // xQueueSend(data_queue, &buf, portMAX_DELAY);
        }
        // vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* 任务2：数据处理 */
void Task_DataProcess(void *arg) {
    uint8_t *buf;
    while (1) {
        /* 从队列接收缓冲区 */
        // if (xQueueReceive(data_queue, &buf, portMAX_DELAY)) {
        //     process_data(buf);
        //     MemPool_Free(MEMPOOL_TYPE_NET, buf);
        // }
    }
}

/* ============ 性能优化建议 ============

1. 预分配：
   - 在系统启动时预分配常用的缓冲区
   - 避免运行时频繁分配/释放

2. 监控：
   - 定期检查内存池使用情况
   - 监控失败次数，及时发现问题

3. 调优：
   - 根据实际使用情况调整块数量
   - 平衡内存占用和性能

4. 安全：
   - 总是检查分配返回值
   - 使用健康检查检测内存泄漏
   - 在释放前验证指针有效性

============ 内存占用统计 ============

Flash缓冲池:   2.8KB × 2 = 5.6KB
网络缓冲池:    1.5KB × 4 = 6.0KB
日志缓冲池:    512B  × 8 = 4.0KB
DMA缓冲池:     4.0KB × 2 = 8.0KB
─────────────────────────────
总计:                      23.6KB

占STM32H7总内存(352KB)的 6.7%，非常高效！

*/
