#include "FLASH.h"
#include "stm32h7xx_hal.h"
#include "shell.h"
#include "shell_port.h"
#include "mempool.h"
/* ============ CRC32 校验 ============ */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71642, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA44E5D6, 0x8D079D40,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856534D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5A6BACA5, 0x2D6D02C5, 0xB40D7D7F, 0xC30DAED9,
    0x5A0EF57A, 0x2D1EBAEC, 0xB40EBDB6, 0xC7D7D820, 0x59B33D83, 0x2EB40D15, 0xB7BE701F, 0xC0BEF789,
    0x1EDB7794, 0x69F3D6E2, 0xF0D5B712, 0x87DCB584, 0x1D7A7127, 0x6A5148B1, 0xF302B90B, 0x8405D0DA,
    0x3050A6EB, 0x47079A7D, 0xDEA580C7, 0xA9D65C51, 0x3D9970F2, 0x4AA56364, 0xD3D6F4DE, 0xA4D1C448,
    0x3E0C1289, 0x49161A1F, 0xD0D8A5A5, 0xA7D15233, 0x39B94FA0, 0x4EAA8A36, 0xD7D6DF8C, 0xA0D5A51A,
    0x6CA6D5FF, 0x1BA3A46D, 0x8207D1D7, 0xF5D0E541, 0x6B6B51E2, 0x1C1C9174, 0x8507D1CE, 0xF20B51C8,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x6D6D06C6, 0x1A6D96B0, 0x8306D0A0, 0xF4D5D636,
    0xD67D3C9B, 0xA1D1AC0D, 0x38D8C2B7, 0x4FDFF821, 0xD1D6DF82, 0xA6D5AF14, 0x3F3D4AEE, 0x48C1DA78,
    0xD8D8D8E9, 0xAFD5A87F, 0x36D3D8C5, 0x41D4B853, 0xDF6D68F0, 0xA8D5D866, 0x31D3D3DC, 0x46D3D3AA,
    0x7A6D7D4A, 0x0D6D6DBC, 0x946D6D06, 0xE36D6D90, 0x7D6D6D33, 0x0A6D6DA5, 0x936D6D1F, 0xE46D6D89,
    0x74D3D3B8, 0x03D3D32E, 0x9AD3D394, 0xEDD3D302, 0x73D3D3A1, 0x04D3D337, 0x9DD3D38D, 0xEAD3D31B,
    0xBE902B69, 0xC95B3BFF, 0x50F08B45, 0x27F09BD3, 0xB9F09B70, 0xCEF09BE6, 0x57F09B5C, 0x20F09BCA,
    0xB0F09BFB, 0xC7F09B6D, 0x5EF09BD7, 0x29F09B41, 0xB7F09BE2, 0xC0F09B74, 0x59F09BCE, 0x2EF09B58,
    0xE2D3D3DD, 0x95D3D34B, 0x0CD3D3F1, 0x7BD3D367, 0xE5D3D3C4, 0x92D3D352, 0x0BD3D3E8, 0x7CD3D37E,
    0xECD3D34F, 0x9BD3D3D9, 0x02D3D363, 0x75D3D3F5, 0xEBD3D356, 0x9CD3D3C0, 0x05D3D37A, 0x72D3D3EC,
};

uint32_t FLASH_CalcCRC32(uint8_t *data, uint16_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

uint8_t FLASH_VerifyCRC32(uint8_t *data, uint16_t len, uint32_t crc) {
    return (FLASH_CalcCRC32(data, len) == crc) ? FLASH_OK : FLASH_ERR_CRC;
}

/* ============ Flash 底层操作 ============ */

/**
 * @brief  擦除指定地址所在的Flash扇区
 * @note   该函数会先解锁Flash，擦除指定扇区后重新锁定Flash
 * @param  addr: 要擦除的扇区起始地址
 * @retval Flash操作状态
 *         - FLASH_OK: 擦除成功
 *         - FLASH_ERR_ERASE: 擦除失败
 * @details
 *         - 该函数会自动计算地址对应的扇区号
 *         - 擦除操作使用电压范围3(FLASH_VOLTAGE_RANGE_3)
 *         - 每次只擦除1个扇区
 *         - 擦除失败时会自动锁定Flash
 *         - 支持的地址必须位于Flash有效范围内
 *         - STM32H743: 0x081F0000位于扇区7 (256KB)
 */
uint8_t FLASH_EraseSector(uint32_t addr) {
    FLASH_EraseInitTypeDef EraseInit;
    uint32_t SectorError = 0;

    HAL_FLASH_Unlock();

    EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInit.Sector = FLASH_SECTOR_7;
    EraseInit.NbSectors = 1;
	EraseInit.Banks = FLASH_BANK_2;
    EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&EraseInit, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        return FLASH_ERR_ERASE;
    }

    HAL_FLASH_Lock();
    return FLASH_OK;
}


uint8_t FLASH_WriteData(uint32_t addr, uint8_t *data, uint16_t len) {
    if (len == 0 || data == NULL || (addr % 32 != 0)) return FLASH_ERR_WRITE;

    HAL_FLASH_Unlock();
    static  __ALIGNED(32) uint32_t flash_word[8]  = {0};
    for (uint16_t i = 0; i < len; i += 32) {

        uint16_t copy_len = (len - i) >= 32 ? 32 : (len - i);

        memcpy(flash_word, &data[i], copy_len);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr + i, (uint32_t)flash_word) != HAL_OK) {
            HAL_FLASH_Lock();
            return FLASH_ERR_WRITE;
        }
    }

    HAL_FLASH_Lock();
    return FLASH_OK;
}

uint8_t FLASH_ReadData(uint32_t addr, uint8_t *data, uint16_t len) {
    if (len == 0 || data == NULL) return FLASH_OK;
    memcpy( (void *)addr,data, len);
    return FLASH_OK;
}

/* ============ 故障日志操作 ============ */

uint8_t FLASH_AddFaultRecord(Fault_Record_t *record) {
    Fault_Log_t *log;
    uint8_t ret;

    if (record == NULL) return FLASH_ERR_WRITE;

    /* 从内存池分配缓冲区 */
    log = (Fault_Log_t *)MemPool_Alloc(MEMPOOL_TYPE_FLASH);
    if (log == NULL) return FLASH_ERR_WRITE;

    /* 读取故障日志 */
    ret = FLASH_ReadFaultLog(log);
    if (ret != FLASH_OK) {// 读取失败（可能是第一次写入或数据损坏），初始化日志结构
        memcpy((uint8_t *)log, (void *)ADDR_FAULT_LOG, sizeof(Fault_Log_t));// 读取原始数据
        debugShellPrintf(&shell, "FLASH_AddFaultRecord: failed to read fault log\r\n");
    }

    /* 环形缓冲：满了则删除最旧的 */
    if (log->record_count >= 10) {
        memmove(&log->records[0], &log->records[1], sizeof(Fault_Record_t) * 9);
        log->record_count = 9;
    }

    /* 添加新记录 */
    memcpy(&log->records[log->record_count], record, sizeof(Fault_Record_t));
    log->record_count++;

    /* 计算CRC */
    log->crc32 = FLASH_CalcCRC32((uint8_t *)&log->records, sizeof(log->records));

    /* 擦除Flash扇区 */
    ret = FLASH_EraseSector(ADDR_FAULT_LOG);
    if (ret != FLASH_OK) {
        MemPool_Free(MEMPOOL_TYPE_FLASH, log);
        debugShellPrintf(&shell, "FLASH_AddFaultRecord: failed to erase sector\r\n");
        return ret;
    }

    /* 写入Flash */
    ret = FLASH_WriteData(ADDR_FAULT_LOG, (uint8_t *)log, sizeof(Fault_Log_t));

    /* 释放内存池 */
    MemPool_Free(MEMPOOL_TYPE_FLASH, log);
    if(ret != FLASH_OK) {
        debugShellPrintf(&shell, "FLASH_AddFaultRecord: failed to write fault log\r\n");
    }else {
        debugShellPrintf(&shell, "FLASH_AddFaultRecord: success\r\n");
    }
    return ret;
}

uint8_t FLASH_ReadFaultLog(Fault_Log_t *log) {
    if (log == NULL) return FLASH_ERR_WRITE;

    FLASH_ReadData(ADDR_FAULT_LOG, (uint8_t *)log, sizeof(Fault_Log_t));

    if (log->magic != 0xA5A5A5A5) {
        memset(log, 0, sizeof(Fault_Log_t));
        log->magic = 0xA5A5A5A5;
        return FLASH_OK;
    }

    if (FLASH_VerifyCRC32((uint8_t *)&log->records, sizeof(log->records), log->crc32) != FLASH_OK) {
        return FLASH_ERR_CRC;
    }

    return FLASH_OK;
}

uint8_t FLASH_ClearFaultLog(void) {
    Fault_Log_t *log;
    uint8_t ret;

    /* 从内存池分配缓冲区 */
    log = (Fault_Log_t *)MemPool_Alloc(MEMPOOL_TYPE_FLASH);
    if (log == NULL) return FLASH_ERR_WRITE;

    memset(log, 0, sizeof(Fault_Log_t));
    log->magic = 0xA5A5A5A5;

    ret = FLASH_EraseSector(ADDR_FAULT_LOG);
    if (ret != FLASH_OK) {
        MemPool_Free(MEMPOOL_TYPE_FLASH, log);
        return ret;
    }

//    ret = FLASH_WriteData(ADDR_FAULT_LOG, (uint8_t *)log, sizeof(Fault_Log_t));

    /* 释放内存池 */
    MemPool_Free(MEMPOOL_TYPE_FLASH, log);

    return ret;
}

/* ============ 初始化 ============ */

void FLASH_Init(void) {
    Fault_Log_t *fault_log;

    /* 从内存池分配缓冲区 */
    fault_log = (Fault_Log_t *)MemPool_Alloc(MEMPOOL_TYPE_FLASH);
    if (fault_log == NULL) {
        debugShellPrintf(&shell, "FLASH_Init: MemPool_Alloc failed\n\r");
        return;
    }
    FLASH_ReadData(ADDR_FAULT_LOG, (uint8_t *)fault_log, sizeof(Fault_Log_t));
    if (fault_log->magic != 0xA5A5A5A5) {
//        FLASH_ClearFaultLog();
			debugShellPrintf(&shell, "FLASH_Init: fault_log.magic is failed!!!\n\r");
			if(FLASH_EraseSector(ADDR_FAULT_LOG) != HAL_OK)
			{
				debugShellPrintf(&shell, "FLASH_Init: Erase Sector is failed!!!\n\r");						
			}else{
				debugShellPrintf(&shell, "FLASH_Init: Erase Sector is success!!!\n\r");
			}

    } else {
        if (FLASH_VerifyCRC32((uint8_t *)&fault_log->records, sizeof(fault_log->records), fault_log->crc32) != FLASH_OK) {
            debugShellPrintf(&shell, "FLASH_Init: fault_log.crc32 is failed!!!\n\r");
        } else {
            debugShellPrintf(&shell, "FLASH_Init: fault_log loaded success!!!, record_count=%d\n\r", fault_log->record_count);
        }
    }

    /* 释放内存池 */
    MemPool_Free(MEMPOOL_TYPE_FLASH, fault_log);
}

/* ============ 用户自定义单元扩展示例 ============
 *
 * 如需添加新的数据单元（如CAN配置、系统参数等），参考以下步骤：
 *
 * 1. 在FLASH.h中定义数据结构体：
 *    typedef struct __attribute__((packed)) {
 *        uint32_t param1;
 *        uint32_t param2;
 *        ...
 *    } MyConfig_t;
 *
 * 2. 定义容器结构体（包含魔数、版本、CRC）：
 *    typedef struct __attribute__((packed)) {
 *        uint32_t magic;           // 0xD5D5D5D5 (每个单元用不同的魔数)
 *        uint16_t version;
 *        uint16_t reserved;
 *        MyConfig_t config;
 *        uint32_t crc32;
 *    } MyConfig_Container_t;
 *
 * 3. 在FLASH.c中实现Save/Read函数：
 *
 *    uint8_t FLASH_SaveMyConfig(MyConfig_t *config) {
 *        MyConfig_Container_t container;
 *        uint8_t ret;
 *
 *        if (config == NULL) return FLASH_ERR_WRITE;
 *
 *        container.magic = 0xD5D5D5D5;
 *        container.version = 1;
 *        memcpy(&container.config, config, sizeof(MyConfig_t));
 *        container.crc32 = FLASH_CalcCRC32((uint8_t *)&container.config, sizeof(MyConfig_t));
 *
 *        // 保存到主分区
 *        ret = FLASH_EraseSector(ADDR_USER_UNIT1);
 *        if (ret != FLASH_OK) return ret;
 *        ret = FLASH_WriteData(ADDR_USER_UNIT1, (uint8_t *)&container, sizeof(MyConfig_Container_t));
 *        if (ret != FLASH_OK) return ret;
 *
 *        // 保存到备份分区
 *        ret = FLASH_EraseSector(ADDR_USER_UNIT1_BACKUP);
 *        if (ret != FLASH_OK) return ret;
 *        return FLASH_WriteData(ADDR_USER_UNIT1_BACKUP, (uint8_t *)&container, sizeof(MyConfig_Container_t));
 *    }
 *
 *    uint8_t FLASH_ReadMyConfig(MyConfig_t *config) {
 *        MyConfig_Container_t container;
 *        uint8_t ret;
 *
 *        if (config == NULL) return FLASH_ERR_WRITE;
 *
 *        // 先读主分区
 *        FLASH_ReadData(ADDR_USER_UNIT1, (uint8_t *)&container, sizeof(MyConfig_Container_t));
 *
 *        if (container.magic != 0xD5D5D5D5) {
 *            // 主分区无效，读备份
 *            FLASH_ReadData(ADDR_USER_UNIT1_BACKUP, (uint8_t *)&container, sizeof(MyConfig_Container_t));
 *            if (container.magic != 0xD5D5D5D5) {
 *                memset(config, 0, sizeof(MyConfig_t));
 *                return FLASH_OK;
 *            }
 *        }
 *
 *        // 校验CRC
 *        ret = FLASH_VerifyCRC32((uint8_t *)&container.config, sizeof(MyConfig_t), container.crc32);
 *        if (ret != FLASH_OK) {
 *            // 主分区CRC错误，试试备份
 *            FLASH_ReadData(ADDR_USER_UNIT1_BACKUP, (uint8_t *)&container, sizeof(MyConfig_Container_t));
 *            ret = FLASH_VerifyCRC32((uint8_t *)&container.config, sizeof(MyConfig_t), container.crc32);
 *            if (ret != FLASH_OK) return FLASH_ERR_CRC;
 *        }
 *
 *        memcpy(config, &container.config, sizeof(MyConfig_t));
 *        return FLASH_OK;
 *    }
 *
 * 4. 在FLASH_Init()中添加初始化检查：
 *
 *    void FLASH_Init(void) {
 *        // ... 故障日志初始化 ...
 *
 *        MyConfig_Container_t my_config;
 *        FLASH_ReadData(ADDR_USER_UNIT1, (uint8_t *)&my_config, sizeof(MyConfig_Container_t));
 *        if (my_config.magic != 0xD5D5D5D5) {
 *            MyConfig_t default_config = { ... };
 *            FLASH_SaveMyConfig(&default_config);
 *        }
 *    }
 *
 * 5. 在FLASH.h中声明新函数：
 *    uint8_t FLASH_SaveMyConfig(MyConfig_t *config);
 *    uint8_t FLASH_ReadMyConfig(MyConfig_t *config);
 */

