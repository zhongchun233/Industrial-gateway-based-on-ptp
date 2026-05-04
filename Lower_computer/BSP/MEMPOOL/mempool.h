#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include <stdint.h>
#include <stddef.h>

/* ============ 内存池配置 ============ */

/* 内存池块类型定义 */
typedef enum {
    MEMPOOL_TYPE_FLASH = 0,      // Flash数据缓冲 (2.8KB)
    MEMPOOL_TYPE_NET,            // 网络缓冲 (1.5KB)
    MEMPOOL_TYPE_LOG,            // 日志缓冲 (512B)
    MEMPOOL_TYPE_DMA,            // DMA缓冲 (4KB)
    MEMPOOL_TYPE_MAX
} MemPool_Type_t;

/* 内存池块大小定义 */
#define MEMPOOL_FLASH_SIZE    2800
#define MEMPOOL_NET_SIZE      1536
#define MEMPOOL_LOG_SIZE      512
#define MEMPOOL_DMA_SIZE      4096

/* 每种类型的块数量 */
#define MEMPOOL_FLASH_BLOCKS  2    // Flash: 主 + 备份
#define MEMPOOL_NET_BLOCKS    4    // 网络: 多个并发连接
#define MEMPOOL_LOG_BLOCKS    8    // 日志: 高频使用
#define MEMPOOL_DMA_BLOCKS    2    // DMA: 主 + 备份

/* ============ 内存池块结构 ============ */

typedef struct {
    uint8_t used;                 // 使用标志
    uint8_t reserved[3];          // 对齐
    uint32_t alloc_time;          // 分配时间戳（用于检测泄漏）
    uint32_t magic;               // 魔数 0xDEADBEEF
} MemPool_BlockHeader_t;

/* ============ 内存池管理结构 ============ */

typedef struct {
    MemPool_Type_t type;          // 块类型
    uint16_t block_size;          // 块大小
    uint16_t block_count;         // 块数量
    uint16_t used_count;          // 已使用块数
    uint16_t peak_count;          // 峰值使用数
    uint32_t alloc_count;         // 总分配次数
    uint32_t free_count;          // 总释放次数
    uint32_t fail_count;          // 分配失败次数
    uint8_t *pool_base;           // 内存池基地址
    uint8_t lock;                 // 互斥锁标志
} MemPool_Manager_t;

/* ============ 全局内存池管理器 ============ */

extern MemPool_Manager_t g_mempool_managers[MEMPOOL_TYPE_MAX];

/* ============ 函数声明 ============ */

/**
 * @brief  初始化内存池系统
 * @retval 0: 成功, 非0: 失败
 */
uint8_t MemPool_Init(void);

/**
 * @brief  从指定类型的内存池分配内存
 * @param  type: 内存池类型
 * @retval 指向分配内存的指针，NULL表示分配失败
 */
void* MemPool_Alloc(MemPool_Type_t type);

/**
 * @brief  释放内存回内存池
 * @param  type: 内存池类型
 * @param  ptr: 要释放的指针
 * @retval 0: 成功, 非0: 失败
 */
uint8_t MemPool_Free(MemPool_Type_t type, void *ptr);

/**
 * @brief  获取内存池统计信息
 * @param  type: 内存池类型
 * @retval 指向管理器结构的指针
 */
MemPool_Manager_t* MemPool_GetStats(MemPool_Type_t type);

/**
 * @brief  检查内存池健康状态
 * @retval 0: 正常, 非0: 异常
 */
uint8_t MemPool_HealthCheck(void);

/**
 * @brief  重置内存池统计信息
 * @param  type: 内存池类型，MEMPOOL_TYPE_MAX表示全部
 */
void MemPool_ResetStats(MemPool_Type_t type);

/**
 * @brief  打印内存池调试信息
 * @param  type: 内存池类型，MEMPOOL_TYPE_MAX表示全部
 */
void MemPool_PrintDebugInfo(MemPool_Type_t type);

/* ============ 错误代码 ============ */

#define MEMPOOL_OK              0x00
#define MEMPOOL_ERR_INIT        0x01
#define MEMPOOL_ERR_ALLOC       0x02
#define MEMPOOL_ERR_FREE        0x03
#define MEMPOOL_ERR_INVALID_PTR 0x04
#define MEMPOOL_ERR_MAGIC       0x05
#define MEMPOOL_ERR_DOUBLE_FREE 0x06
#define MEMPOOL_ERR_OVERFLOW    0x07

#endif /* __MEMPOOL_H__ */
