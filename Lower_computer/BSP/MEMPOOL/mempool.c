#include "mempool.h"
#include <string.h>

/* ============ 静态内存池存储 ============ */

/* Flash数据缓冲池 */
static uint8_t g_mempool_flash_data[MEMPOOL_FLASH_SIZE * MEMPOOL_FLASH_BLOCKS];

/* 网络缓冲池 */
static uint8_t g_mempool_net_data[MEMPOOL_NET_SIZE * MEMPOOL_NET_BLOCKS];

/* 日志缓冲池 */
static uint8_t g_mempool_log_data[MEMPOOL_LOG_SIZE * MEMPOOL_LOG_BLOCKS];

/* DMA缓冲池 */
static uint8_t g_mempool_dma_data[MEMPOOL_DMA_SIZE * MEMPOOL_DMA_BLOCKS];

/* ============ 内存池管理器 ============ */

MemPool_Manager_t g_mempool_managers[MEMPOOL_TYPE_MAX] = {0};

/* ============ 内存池初始化 ============ */

uint8_t MemPool_Init(void) {
    /* 初始化Flash缓冲池 */
    g_mempool_managers[MEMPOOL_TYPE_FLASH].type = MEMPOOL_TYPE_FLASH;
    g_mempool_managers[MEMPOOL_TYPE_FLASH].block_size = MEMPOOL_FLASH_SIZE;
    g_mempool_managers[MEMPOOL_TYPE_FLASH].block_count = MEMPOOL_FLASH_BLOCKS;
    g_mempool_managers[MEMPOOL_TYPE_FLASH].pool_base = g_mempool_flash_data;
    g_mempool_managers[MEMPOOL_TYPE_FLASH].lock = 0;

    /* 初始化网络缓冲池 */
    g_mempool_managers[MEMPOOL_TYPE_NET].type = MEMPOOL_TYPE_NET;
    g_mempool_managers[MEMPOOL_TYPE_NET].block_size = MEMPOOL_NET_SIZE;
    g_mempool_managers[MEMPOOL_TYPE_NET].block_count = MEMPOOL_NET_BLOCKS;
    g_mempool_managers[MEMPOOL_TYPE_NET].pool_base = g_mempool_net_data;
    g_mempool_managers[MEMPOOL_TYPE_NET].lock = 0;

    /* 初始化日志缓冲池 */
    g_mempool_managers[MEMPOOL_TYPE_LOG].type = MEMPOOL_TYPE_LOG;
    g_mempool_managers[MEMPOOL_TYPE_LOG].block_size = MEMPOOL_LOG_SIZE;
    g_mempool_managers[MEMPOOL_TYPE_LOG].block_count = MEMPOOL_LOG_BLOCKS;
    g_mempool_managers[MEMPOOL_TYPE_LOG].pool_base = g_mempool_log_data;
    g_mempool_managers[MEMPOOL_TYPE_LOG].lock = 0;

    /* 初始化DMA缓冲池 */
    g_mempool_managers[MEMPOOL_TYPE_DMA].type = MEMPOOL_TYPE_DMA;
    g_mempool_managers[MEMPOOL_TYPE_DMA].block_size = MEMPOOL_DMA_SIZE;
    g_mempool_managers[MEMPOOL_TYPE_DMA].block_count = MEMPOOL_DMA_BLOCKS;
    g_mempool_managers[MEMPOOL_TYPE_DMA].pool_base = g_mempool_dma_data;
    g_mempool_managers[MEMPOOL_TYPE_DMA].lock = 0;

    return MEMPOOL_OK;
}

/* ============ 内存池分配 ============ */

void* MemPool_Alloc(MemPool_Type_t type) {
    if (type >= MEMPOOL_TYPE_MAX) {
        return NULL;
    }

    MemPool_Manager_t *mgr = &g_mempool_managers[type];
    uint16_t block_size = mgr->block_size + sizeof(MemPool_BlockHeader_t);
    uint8_t *Alloc_pool_base = mgr->pool_base;

    /* 加锁 */
    mgr->lock = 1;

    /* 查找空闲块 */
    for (uint16_t i = 0; i < mgr->block_count; i++) {
        MemPool_BlockHeader_t *header = 
										(MemPool_BlockHeader_t *)(Alloc_pool_base + i * block_size);

        if (!header->used) {
            /* 标记为已使用 */
            header->used = 1;
            header->magic = 0xDEADBEEF;
            header->alloc_time = 0;  // 可以在这里记录时间戳

            /* 更新统计信息 */
            mgr->used_count++;
            mgr->alloc_count++;
            if (mgr->used_count > mgr->peak_count) {
                mgr->peak_count = mgr->used_count;
            }

            /* 解锁 */
            mgr->lock = 0;

            /* 返回数据区指针（跳过header） */
            return (void *)(header + 1);
        }
    }

    /* 分配失败 */
    mgr->fail_count++;
    mgr->lock = 0;
    return NULL;
}

/* ============ 内存池释放 ============ */

uint8_t MemPool_Free(MemPool_Type_t type, void *ptr) {
    if (type >= MEMPOOL_TYPE_MAX || ptr == NULL) {
        return MEMPOOL_ERR_INVALID_PTR;
    }

    MemPool_Manager_t *mgr = &g_mempool_managers[type];
    uint16_t block_size = mgr->block_size + sizeof(MemPool_BlockHeader_t);
    uint8_t *pool = mgr->pool_base;

    /* 获取header指针 */
    MemPool_BlockHeader_t *header = (MemPool_BlockHeader_t *)ptr - 1;

    /* 加锁 */
    mgr->lock = 1;

    /* 验证魔数 */
    if (header->magic != 0xDEADBEEF) {
        mgr->lock = 0;
        return MEMPOOL_ERR_MAGIC;
    }

    /* 检查是否已释放 */
    if (!header->used) {
        mgr->lock = 0;
        return MEMPOOL_ERR_DOUBLE_FREE;
    }

    /* 验证指针是否在池范围内 */
    uint8_t *pool_end = pool + mgr->block_count * block_size;
    if ((uint8_t *)header < pool || (uint8_t *)header >= pool_end) {
        mgr->lock = 0;
        return MEMPOOL_ERR_INVALID_PTR;
    }

    /* 标记为未使用 */
    header->used = 0;
    header->magic = 0;

    /* 清空数据（安全考虑） */
    memset(ptr, 0, mgr->block_size);

    /* 更新统计信息 */
    mgr->used_count--;
    mgr->free_count++;

    /* 解锁 */
    mgr->lock = 0;

    return MEMPOOL_OK;
}

/* ============ 统计信息 ============ */

MemPool_Manager_t* MemPool_GetStats(MemPool_Type_t type) {
    if (type >= MEMPOOL_TYPE_MAX) {
        return NULL;
    }
    return &g_mempool_managers[type];
}

/* ============ 健康检查 ============ */

uint8_t MemPool_HealthCheck(void) {
    for (uint16_t i = 0; i < MEMPOOL_TYPE_MAX; i++) {
        MemPool_Manager_t *mgr = &g_mempool_managers[i];
        uint16_t block_size = mgr->block_size + sizeof(MemPool_BlockHeader_t);
        uint8_t *pool = mgr->pool_base;

        uint16_t used_count = 0;

        /* 遍历所有块，验证完整性 */
        for (uint16_t j = 0; j < mgr->block_count; j++) {
            MemPool_BlockHeader_t *header = (MemPool_BlockHeader_t *)(pool + j * block_size);

            if (header->used) {
                if (header->magic != 0xDEADBEEF) {
                    return MEMPOOL_ERR_MAGIC;
                }
                used_count++;
            }
        }

        /* 检查统计信息是否一致 */
        if (used_count != mgr->used_count) {
            return MEMPOOL_ERR_OVERFLOW;
        }
    }

    return MEMPOOL_OK;
}

/* ============ 清除内存池 ============ */

void MemPool_ResetStats(MemPool_Type_t type) {
    if (type == MEMPOOL_TYPE_MAX) {
        /* 重置所有 */
        for (uint16_t i = 0; i < MEMPOOL_TYPE_MAX; i++) {
            g_mempool_managers[i].alloc_count = 0;
            g_mempool_managers[i].free_count = 0;
            g_mempool_managers[i].fail_count = 0;
            g_mempool_managers[i].peak_count = 0;
        }
    } else if (type < MEMPOOL_TYPE_MAX) {
        g_mempool_managers[type].alloc_count = 0;
        g_mempool_managers[type].free_count = 0;
        g_mempool_managers[type].fail_count = 0;
        g_mempool_managers[type].peak_count = 0;
    }
}

/* ============ 调试信息 ============ */

void MemPool_PrintDebugInfo(MemPool_Type_t type) {
    if (type == MEMPOOL_TYPE_MAX) {
        /* 打印所有 */
        for (uint16_t i = 0; i < MEMPOOL_TYPE_MAX; i++) {
            MemPool_Manager_t *mgr = &g_mempool_managers[i];
            // printf("MemPool[%d]: Used=%d/%d, Peak=%d, Alloc=%d, Free=%d, Fail=%d\n",
            //        i, mgr->used_count, mgr->block_count, mgr->peak_count,
            //        mgr->alloc_count, mgr->free_count, mgr->fail_count);
        }
    } else if (type < MEMPOOL_TYPE_MAX) {
        MemPool_Manager_t *mgr = &g_mempool_managers[type];
        // printf("MemPool[%d]: Used=%d/%d, Peak=%d, Alloc=%d, Free=%d, Fail=%d\n",
        //        type, mgr->used_count, mgr->block_count, mgr->peak_count,
        //        mgr->alloc_count, mgr->free_count, mgr->fail_count);
    }
}
