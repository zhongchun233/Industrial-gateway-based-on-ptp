#ifndef __FLASH_H__
#define __FLASH_H__

#include "main.h"
#include <stdint.h>
#include <string.h>

/* ============ Flash 分区规划 ============
 * STM32H743IIT6 Flash: 2MB (0x08000000 - 0x081FFFFF)
 *
 * 扇区0-3:   16KB each   (0x08000000 - 0x0800FFFF)
 * 扇区4:     64KB        (0x08010000 - 0x0801FFFF)
 * 扇区5-7:   256KB each  (0x08020000 - 0x081FFFFF)
 *
 * 0x081F0000 位于扇区7 (256KB扇区)
 *
 * 使用方案：
 * 0x081F0000 - 0x081FFFFF (64KB)  : 故障日志 + 备份 + 预留
 *
 * 扩展说明：参考 FLASH_AddFaultRecord() 和 FLASH_ReadFaultLog() 的实现方式
 *          可以轻松添加新的数据单元。每个单元需要：
 *          1. 定义数据结构体和容器结构体
 *          2. 实现 Save/Read 函数
 *          3. 在 FLASH_Init() 中添加初始化检查
 */

#define FLASH_BASE_ADDR           0x081F0000
#define FLASH_SECTOR_SIZE_CUSTOM  0x40000     // 256KB per sector (STM32H743扇区7)

/* 分区地址定义 - 在同一个256KB扇区内分配 */
#define ADDR_FAULT_LOG            (FLASH_BASE_ADDR + 0x0000)      // 12KB
#define ADDR_FAULT_LOG_BACKUP     (FLASH_BASE_ADDR + 0x3000)      // 12KB
#define ADDR_USER_UNIT1           (FLASH_BASE_ADDR + 0x6000)      // 4KB
#define ADDR_USER_UNIT1_BACKUP    (FLASH_BASE_ADDR + 0x7000)      // 4KB
#define ADDR_USER_UNIT2           (FLASH_BASE_ADDR + 0x8000)      // 4KB
#define ADDR_USER_UNIT2_BACKUP    (FLASH_BASE_ADDR + 0x9000)      // 4KB

/* ============ 数据结构定义 ============ */
typedef struct __attribute__((packed)) {
    uint8_t year;           // 年
    uint8_t month;          // 月
    uint8_t day;            // 日
    uint8_t hour;           // 小时
    uint8_t minute;         // 分钟
    uint8_t second;         // 秒
    uint16_t millisecond;    // 毫秒
} Flash_Record_time_t;  // 大小：8字节

/* 故障记录结构 */
typedef struct __attribute__((packed)) {
    Flash_Record_time_t time;     // 时间信息
    uint8_t fault_code;          // 故障代码
    uint8_t  fault_level;         // 故障等级 (0=info, 1=warn, 2=error, 3=critical)
    uint8_t  version_major;         // 大版本
    uint8_t  version_minor;          // 小版本号
    uint32_t fault_data;          // 故障相关数据
    uint32_t crc32;         // 4字节CRC校验（仅对fault_desc部分进行校验）
    uint8_t     fault_desc[256];      // 故障描述
    
} Fault_Record_t;  // 大小：272字节

/* 故障日志容器：必须 32字节对齐，不能用 packed！*/
typedef struct {
    uint32_t magic;         // 4
    uint16_t record_count;  // 2
    uint16_t reserved;      // 2
    uint32_t crc32;         // 4
    Fault_Record_t records[10];  // 2760字节
    uint8_t reserve_pad[12];     // 精确补齐 12 字节 → 总 2784 = 32×87

} Fault_Log_t;  // 强制 32 字节对齐

/* ============ 函数声明 ============ */

/* 初始化 */
void FLASH_Init(void);

/* 故障日志操作 */
uint8_t FLASH_AddFaultRecord(Fault_Record_t *record);
uint8_t FLASH_ReadFaultLog(Fault_Log_t *log);
uint8_t FLASH_ClearFaultLog(void);

/* 通用操作（供用户自定义单元使用） */
uint8_t FLASH_WriteData(uint32_t addr, uint8_t *data, uint16_t len);
uint8_t FLASH_ReadData(uint32_t addr, uint8_t *data, uint16_t len);
uint8_t FLASH_EraseSector(uint32_t addr);

/* CRC校验（供用户自定义单元使用） */
uint32_t FLASH_CalcCRC32(uint8_t *data, uint16_t len);
uint8_t FLASH_VerifyCRC32(uint8_t *data, uint16_t len, uint32_t crc);

/* 错误代码 */
#define FLASH_OK                  0x00
#define FLASH_ERR_WRITE           0x01
#define FLASH_ERR_ERASE           0x02
#define FLASH_ERR_CRC             0x03
#define FLASH_ERR_MAGIC           0x04
#define FLASH_ERR_VERSION         0x05
#define FLASH_ERR_FULL            0x06

#endif /* __FLASH_H__ */
