#include "user_cmd.h"
#include "main.h"
#include "shell.h"
#include "shell_cfg.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"
#include "shell_ext.h"

#include <stdlib.h>
#include "BSP_RTC.h"
#include "w25q128.h"
#include "fatfs.h"
#include "ff.h"

void nowtimeCmd(int argc, char *argv[])
{
	shellPrint(shellGetCurrent(), "nowtick: %d\r\n",HAL_GetTick());
	RTC_Get();
	shellPrint(shellGetCurrent(),"DATA:%d,%d,%d,TIME:%d,%d,%d,week:%d,\r\n",
						NowDate.Year+2000,NowDate.Month,NowDate.Date,NowTime.Hours,
						NowTime.Minutes,NowTime.Seconds,NowDate.WeekDay);}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
nowtime, nowtimeCmd, show nowtime);
/** 
 * ��������: ����RTCʱ��
 * �����ʽ: settime <��> <��> <��> <ʱ> <��> <��>
 * ʾ��:    settime 2026 4 23 12 0 0
 */
void settimeCmd(int argc, char *argv[])
{
    uint16_t year;
    uint8_t mon, day, hour, min, sec;
    uint8_t res;

    // 1. ������� (��Ҫ6������: �� �� �� ʱ �� ��)
    if (argc < 7)
    {
        shellPrint(shellGetCurrent(), "Usage: settime <year> <mon> <day> <hour> <min> <sec>\r\n");
        shellPrint(shellGetCurrent(), "Example: settime 2026 4 23 12 0 0\r\n");
        return;
    }

    // 2. ����ת�� (�ַ���ת����)
    // ע�⣺����ʹ�ü򵥵�atoiת����ʵ����Ŀ�н������ӷ�Χ���
    year = atoi(argv[1]);
    mon  = atoi(argv[2]);
    day  = atoi(argv[3]);
    hour = atoi(argv[4]);
    min  = atoi(argv[5]);
    sec  = atoi(argv[6]);

    // 3. �򵥵ķ�Χ��� (��ѡ����ֹ�Ƿ�ʱ��)
    if (mon > 12 || day > 31 || hour > 23 || min > 59 || sec > 59)
    {
        shellPrint(shellGetCurrent(), "Error: Invalid time value!\r\n");
        return;
    }

    // 4. ���� RTC_Set ����
    // 
    res = RTC_Set(year, mon, day, hour, min, sec);

    if (res == 0)
    {
        shellPrint(shellGetCurrent(), "RTC Set Success: %d-%d-%d %d:%d:%d\r\n", year, mon, day, hour, min, sec);
    }
    else
    {
        shellPrint(shellGetCurrent(), "RTC Set Failed! Error Code: %d\r\n", res);
    }
}

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
    settime, settimeCmd, set rtc time);

/*
 * flash_cc936 — 从 U 盘 CC936.BIN 烧写 GBK 转换表到 W25Q128
 *
 * 用法: flash_cc936
 *   1. 在 PC 上运行 tools/gen_cc936_bin.py 生成 CC936.BIN
 *   2. 将 CC936.BIN 放到 U 盘根目录
 *   3. 插入 U 盘，等待识别，然后执行本命令
 *   4. 烧写完成后重启设备，中文 LFN 即可正常使用
 */
#define CC936_FILE_PATH   "0:/CC936.BIN"
#define CC936_BUF_SIZE    4096   /* 每次读写的缓冲区大小 */

void flashCC936Cmd(int argc, char *argv[])
{
    Shell   *sh  = shellGetCurrent();
    FIL      fil;
    FRESULT  res;
    UINT     br;
    uint8_t  buf[CC936_BUF_SIZE];
    uint32_t offset = 0;
    uint32_t file_size;

    shellPrint(sh, "[flash_cc936] 开始...\r\n");

    /* 尝试挂载 USB (已挂载则忽略错误) */
    f_mount(&USBHFatFS, USBHPath, 1);

    res = f_open(&fil, CC936_FILE_PATH, FA_READ);
    if (res != FR_OK) {
        shellPrint(sh, "[flash_cc936] 错误: 无法打开 %s (err=%d)\r\n"
                       "  请确认 U 盘已连接且含有 CC936.BIN\r\n",
                   CC936_FILE_PATH, res);
        return;
    }

    file_size = f_size(&fil);
    if (file_size < 16) {
        shellPrint(sh, "[flash_cc936] 错误: 文件太小 (%u bytes)\r\n", file_size);
        f_close(&fil);
        return;
    }
    shellPrint(sh, "[flash_cc936] 文件大小: %u bytes (%.1f KB)\r\n",
               file_size, file_size / 1024.0f);

    /* 验证魔数 */
    f_read(&fil, buf, 8, &br);
    if (br != 8 || memcmp(buf, CC936_MAGIC, 8) != 0) {
        shellPrint(sh, "[flash_cc936] 错误: 魔数不匹配，请用 gen_cc936_bin.py 重新生成\r\n");
        f_close(&fil);
        return;
    }
    f_lseek(&fil, 0);

    /* 退出内存映射模式，允许写入 */
    W25Q128_ExitMemoryMapped();

    /* 擦除所需扇区 (4KB/扇区，向上取整) */
    uint32_t sectors = (file_size + W25Q128_SECTOR_SIZE - 1) / W25Q128_SECTOR_SIZE;
    shellPrint(sh, "[flash_cc936] 擦除 %u 个扇区...\r\n", sectors);
    for (uint32_t s = 0; s < sectors; s++) {
        W25Q128_EraseSector(CC936_W25Q128_OFFSET + s * W25Q128_SECTOR_SIZE);
    }

    /* 分块读取并写入 W25Q128 */
    shellPrint(sh, "[flash_cc936] 写入中...\r\n");
    while (1) {
        res = f_read(&fil, buf, CC936_BUF_SIZE, &br);
        if (res != FR_OK || br == 0) break;
        W25Q128_Write(CC936_W25Q128_OFFSET + offset, buf, br);
        offset += br;
        shellPrint(sh, "\r  %u / %u KB", offset / 1024, file_size / 1024);
    }
    shellPrint(sh, "\r\n");
    f_close(&fil);

    if (offset != file_size) {
        shellPrint(sh, "[flash_cc936] 错误: 写入不完整 (%u/%u)\r\n", offset, file_size);
        return;
    }

    shellPrint(sh, "[flash_cc936] 写入完成，重新初始化 CC936...\r\n");
    CC936_Init();

    if (CC936_IsReady()) {
        shellPrint(sh, "[flash_cc936] 成功! 中文长文件名已可用。\r\n");
    } else {
        shellPrint(sh, "[flash_cc936] 警告: 初始化失败，请重启设备后重试。\r\n");
    }
}

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
    flash_cc936, flashCC936Cmd, burn CC936.BIN from USB to W25Q128);