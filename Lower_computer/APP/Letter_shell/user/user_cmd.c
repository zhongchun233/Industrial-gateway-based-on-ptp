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
#include "FLASH.h"
#include "mempool.h"
/**
 * @brief 显示当前时间的命令处理函数
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
void nowtimeCmd(int argc, char *argv[])
{

    // 打印系统启动后的tick数
	shellPrint(shellGetCurrent(), "nowtick: %d\r\n",HAL_GetTick());

    // 获取RTC实时时钟数据
	RTC_Get();

    // 打印当前日期和时间信息
	shellPrint(shellGetCurrent(),"DATA:%d,%d,%d,TIME:%d,%d,%d,week:%d,\r\n",
						NowDate.Year+2000,NowDate.Month,NowDate.Date,NowTime.Hours,
						NowTime.Minutes,NowTime.Seconds,NowDate.WeekDay);
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
nowtime, nowtimeCmd, test command);


/**
 * @brief 设置系统时间的命令处理函数
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
void settimeCmd(int argc, char *argv[])
{
    uint16_t year;            // 年份变量
    uint8_t mon, day, hour, min, sec;    // 月份、日期、小时、分钟、秒变量
    uint8_t res;            // 函数返回状态变量


    // 检查参数数量是否足够
    if (argc < 7)
    {

        // 打印使用说明和示例
        shellPrint(shellGetCurrent(), "Usage: settime <year> <mon> <day> <hour> <min> <sec>\r\n");
        shellPrint(shellGetCurrent(), "Example: settime 2026 4 23 12 0 0\r\n");
        return;
    }





    // 将字符串参数转换为数值
    year = atoi(argv[1]);    // 将年份参数转换为整数
    mon  = atoi(argv[2]);    // 将月份参数转换为整数
    day  = atoi(argv[3]);    // 将日期参数转换为整数
    hour = atoi(argv[4]);    // 将小时参数转换为整数
    min  = atoi(argv[5]);    // 将分钟参数转换为整数
    sec  = atoi(argv[6]);    // 将秒数参数转换为整数

  

    // 验证时间参数的有效性
    if (mon > 12 || day > 31 || hour > 23 || min > 59 || sec > 59)
    {

        // 打印错误信息
        shellPrint(shellGetCurrent(), "Error: Invalid time value!\r\n");
        return;
    }


    // 调用RTC设置函数
    res = RTC_Set(year, mon, day, hour, min, sec);

    // 根据返回值打印设置结果
    if (res == 0)
    {
  
        // 打印成功信息，显示设置的时间
        shellPrint(shellGetCurrent(), "RTC Set Success: %d-%d-%d %d:%d:%d\r\n", year, mon, day, hour, min, sec);
    }
    else
    {

        // 打印失败信息，显示错误代码
        shellPrint(shellGetCurrent(), "RTC Set Failed! Error Code: %d\r\n", res);
    }
}

SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
    settime, settimeCmd, set rtc time);


void  HardFault_test_Cmd(int argc, char *argv[])
{

    // 打印系统启动后的tick数
	shellPrint(shellGetCurrent(), "nowtick: %d\r\n",HAL_GetTick());

    ((void(*)(void)) 0)(); // 故意执行空指针调用，触发HardFault异常
	shellPrint(shellGetCurrent(),"已触发HardFault异常\r\n");
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
hardfault_test, HardFault_test_Cmd, test HardFault);

void  printf_HardFault_log_Cmd(int argc, char *argv[])
{
    Fault_Log_t *fault_log;
    uint8_t log_index = 0;
    /* 从内存池分配缓冲区 */
    fault_log = (Fault_Log_t *)MemPool_Alloc(MEMPOOL_TYPE_FLASH);
    if(fault_log == NULL)
    {
    shellPrint(shellGetCurrent(),"MemPool_Alloc failed!!!\r\n");
    return;
    }
    FLASH_ReadData(ADDR_FAULT_LOG, (uint8_t *)fault_log, sizeof(Fault_Log_t));

    if(argc == 1)
    {
        log_index = atoi(argv[1]);    // 将字符串参数转换为整数
        Fault_Record_t *record = &fault_log->records[log_index]; // 获取指定索引的记录
        shellPrint(shellGetCurrent(),"Record %d: Time: %02d-%02d-%02d %02d:%02d:%02d, Code: 0x%04X, Level: %d\r\n",
                log_index+1,
                record->time.year+2000, record->time.month, record->time.day,
                record->time.hour, record->time.minute, record->time.second,
                record->fault_code, record->fault_level);
        shellPrint(shellGetCurrent(),(char *)(record->fault_desc));
        shellPrint(shellGetCurrent(), "Usage: printf_HardFault_log\r\n");
        return;
    }else 
    {

        if(fault_log->magic != 0xA5A5A5A5)
        {
            shellPrint(shellGetCurrent(),"No valid fault log found!!!\r\n");
        }else
        {
            shellPrint(shellGetCurrent(),"Fault log loaded success!!!, record_count=%d\r\n", fault_log->record_count);
            for(int i=0; i<fault_log->record_count && i<10; i++)
            {
                Fault_Record_t *record = &fault_log->records[i];
                shellPrint(shellGetCurrent(),"Record %d: Time: %02d-%02d-%02d %02d:%02d:%02d, Code: 0x%04X, Level: %d,\r\n",
                i+1,
                record->time.year+2000, record->time.month, record->time.day,
                record->time.hour, record->time.minute, record->time.second,
                record->fault_code, record->fault_level);
            }
        }   

    }
    MemPool_Free(MEMPOOL_TYPE_FLASH, fault_log);
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
printf_HardFault_log,printf_HardFault_log_Cmd, read and printf HardFault log in flash);