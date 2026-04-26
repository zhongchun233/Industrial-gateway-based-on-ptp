#ifndef __W25Q128_H
#define __W25Q128_H

#include "main.h"
#include <stdint.h>

/* ---- W25Q128 命令 ---- */
#define W25Q128_WRITE_ENABLE                0x06
#define W25Q128_WRITE_DISABLE               0x04
#define W25Q128_READ_STATUS_REG1            0x05
#define W25Q128_READ_STATUS_REG2            0x35
#define W25Q128_WRITE_STATUS_REG            0x01
#define W25Q128_PAGE_PROGRAM                0x02
#define W25Q128_QUAD_PAGE_PROGRAM           0x32
#define W25Q128_BLOCK_ERASE_64K             0xD8
#define W25Q128_BLOCK_ERASE_32K             0x52
#define W25Q128_SECTOR_ERASE_4K             0x20
#define W25Q128_CHIP_ERASE                  0xC7
#define W25Q128_ERASE_SUSPEND               0x75
#define W25Q128_ERASE_RESUME                0x7A
#define W25Q128_POWER_DOWN                  0xB9
#define W25Q128_RELEASE_POWER_DOWN          0xAB
#define W25Q128_MANUFACTURER_ID             0x90
#define W25Q128_JEDEC_ID                    0x9F
#define W25Q128_READ_UNIQUE_ID              0x4B
#define W25Q128_READ_DATA                   0x03
#define W25Q128_FAST_READ                   0x0B
#define W25Q128_FAST_READ_DUAL_OUT          0x3B
#define W25Q128_FAST_READ_DUAL_IO           0xBB
#define W25Q128_FAST_READ_QUAD_OUT          0x6B
#define W25Q128_FAST_READ_QUAD_IO           0xEB
#define W25Q128_WORD_READ                   0xE3

/* ---- W25Q128 容量 ---- */
#define W25Q128_PAGE_SIZE                   256
#define W25Q128_SECTOR_SIZE                 4096
#define W25Q128_BLOCK_SIZE_64K              65536
#define W25Q128_CHIP_SIZE                   16777216U   /* 16MB */

/* ---- CC936 在 W25Q128 中的布局 ---- */
#define CC936_W25Q128_OFFSET                0x000000UL  /* 从 W25Q128 起始地址存储 */
#define CC936_MAGIC                         "CC936V01"  /* 8 字节魔数 */

/* STM32H743 QUADSPI 内存映射基地址 (Bank1) */
#define W25Q128_MAPPED_BASE                 0x90000000UL

/* ---- 基础驱动 API ---- */
void     W25Q128_Init(void);
void     W25Q128_Read(uint32_t addr, uint8_t *buf, uint32_t len);
void     W25Q128_Write(uint32_t addr, uint8_t *buf, uint32_t len);
void     W25Q128_EraseSector(uint32_t addr);
void     W25Q128_EraseBlock(uint32_t addr);
void     W25Q128_EraseChip(void);
uint32_t W25Q128_ReadID(void);
void     W25Q128_WaitBusy(void);

/* ---- 内存映射模式 API ---- */
void     W25Q128_EnterMemoryMapped(void);
void     W25Q128_ExitMemoryMapped(void);

/* ---- CC936 API ---- */
void     CC936_Init(void);      /* 检测魔数，成功则进入内存映射并设置 ff_convert 指针 */
uint8_t  CC936_IsReady(void);   /* 返回 1 表示 cc936 数据已就绪 */

#endif /* __W25Q128_H */
