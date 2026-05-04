#include "USB.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"    
#include "fatfs.h"
#include "ff.h"
#include "usb_host.h"
#include "shell.h"
#include "shell_port.h"
#include "shell_cfg.h"
#include "shell_ext.h"
#include "cmsis_os.h"
#include "BSP_RTC.h"
#include <string.h>
#include <stdarg.h>   
TaskHandle_t xUSBTaskHandle = NULL;
extern ApplicationTypeDef Appli_state;
extern RTC_DateTypeDef NowDate;
extern RTC_TimeTypeDef NowTime;

// 记录当前正在使用的日志日期，用来判断跨天


static FIL logFile = {0}; // 日志文件对象
static uint8_t usb_mounted = 0;
static char log_buffer[512] = {0}; // 日志缓冲区
static volatile uint8_t is_writing_log = 0; // 标志位，表示当前是否正在写入日志

static uint8_t MSC_Log_Is_NEWDAY(void)
{
    static uint16_t cur_log_year = 0;
    static uint8_t  cur_log_mon  = 0;
    static uint8_t  cur_log_day  = 0;
    if(cur_log_year != NowDate.Year + 2000 || cur_log_mon != NowDate.Month || cur_log_day != NowDate.Date)
    {
        cur_log_year = NowDate.Year + 2000;
        cur_log_mon  = NowDate.Month;
        cur_log_day  = NowDate.Date;

        return 1;
    }else
    {
        return 0;
    }
}
/**
 * @brief USB大容量存储(MSC)应用程序主函数
 * 该函数负责初始化USB存储设备，创建日志文件并写入初始标记
 */
static uint8_t MSC_Log_Start(void)
{
    FRESULT res;                        // FAT文件系统操作结果
    uint32_t byteswritten;  //file write/read counts
    char log_file_name[32] = {0}; // 日志文件名缓冲区


    RTC_Get();
    sprintf(log_file_name, "log_%04d%02d%02d.txt",
            NowDate.Year + 2000, NowDate.Month, NowDate.Date);

    if(f_mount(&USBHFatFS,(const TCHAR*)USBHPath,1) !=FR_OK)
    {
        //Fatfs Initialization Error
        shellPrint(&shell," mount USB fail!!! \r\n");        
        Error_Handler();
        usb_mounted = 0;
        return 0;
    }
    else
    {
        shellPrint(&shell," mount USB success!!! \r\n");


        if(f_open(&logFile, log_file_name, FA_OPEN_ALWAYS | FA_WRITE) !=FR_OK)
        {
            shellPrint(&shell," open log file fail!!! \r\n");
            return 0;
        }
        else
        {
            shellPrint(&shell," open log file success!!! \r\n");

            f_lseek(&logFile, f_size(&logFile));

            char header[64] = {0};
            sprintf(header, "\r\n========== New Log Day : %04d-%02d-%02d ==========\r\n",
            NowDate.Year+2000, NowDate.Month, NowDate.Date);
            f_write(&logFile, header, strlen(header), (void *)&byteswritten);
            f_sync(&logFile);
        } 
        usb_mounted = 1;
        return 1;
    }
}



void USB_Log_Printf(const char *format, ...)
{
    if(is_writing_log)
        return;
		if(MSC_Log_Is_NEWDAY())
    {
        // 跨天，关闭当前日志文件，创建新的日志文件
        taskENTER_CRITICAL();
        f_sync(&logFile);
        f_close(&logFile);
        taskEXIT_CRITICAL();
        if(!MSC_Log_Start())
        {
            shellPrint(&shell,"Failed to create new log file for new day!!!\r\n");
            return;
        }
    }
    if(!usb_mounted || Appli_state == APPLICATION_DISCONNECT)
        return;



    is_writing_log = 1;

    va_list args;
    uint32_t byteswritten;
    FRESULT res;

    RTC_Get();
    snprintf(log_buffer, sizeof(log_buffer),
                      "[%04d-%02d-%02d %02d:%02d:%02d]",
                      (int)NowDate.Year + 2000, 
                      (int)NowDate.Month, 
                      (int)NowDate.Date,  // 年月日
                      (int)NowTime.Hours, 
                      (int)NowTime.Minutes, 
                      (int)NowTime.Seconds); 
    uint8_t time_len = strlen(log_buffer);

    va_start(args, format);                    
    int final_len = vsnprintf(log_buffer + time_len, 
                sizeof(log_buffer) - time_len - 2, format, args);                                   
    va_end(args);
    final_len += time_len;
    if(final_len > 0 && final_len < (int)sizeof(log_buffer) - 2)
    {
        if(log_buffer[final_len - 1] != '\n')
        {
            log_buffer[final_len] = '\r';
            log_buffer[final_len + 1] = '\n';
            final_len += 2;
        }
    }


    res = f_write(&logFile, log_buffer, final_len, (void *)&byteswritten);
    if(res == FR_OK && byteswritten != 0)
    {
        f_sync(&logFile);
    }else
    {
        // f_sync(&logFile);
        taskENTER_CRITICAL();
        {
            f_close(&logFile);
            f_mount(NULL, (const TCHAR*)"",0);
            usb_mounted = 0;
        }
        taskEXIT_CRITICAL();       
        shellPrint(&shell,"USB write error\r\n");
    }
    is_writing_log = 0;

}
/**
 * @brief MSC日志处理函数
 * 该函数用于处理USB存储设备(MSC)的日志记录功能
 * 根据USB设备是否挂载，执行不同的操作
 */
static void MSC_Log_Application(void)
{
 

    // 如果已挂载，执行USB日志写入
    USB_Log_Printf("time:%d.\r\n", (int)NowTime.Seconds);

    
}
static void MSC_Application(void)
{


    switch(Appli_state)
    {
        case APPLICATION_READY:

             MSC_Log_Application();

            break;

        case APPLICATION_DISCONNECT:
            if(usb_mounted)
            {
                uint8_t timeout  = 20;
                while(is_writing_log && timeout--)
                {
                    vTaskDelay(1);
                }
                taskENTER_CRITICAL();
                {
                    f_sync(&logFile);
                    f_close(&logFile);
                    f_mount(NULL, (const TCHAR*)"",0);
                    usb_mounted = 0;
                }
                taskEXIT_CRITICAL();
                shellPrint(&shell,"USB MSC device is disconnected.\r\n");
            }
            break;

        default:
            break;
    }
}



void USB_Task(void *pvParameters)
{
	shellPrint(&shell, "\r\nUSB_Task start\r\n");
    for(;;)
    {

        MSC_Application();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void Create_USB_Task(void)
{
    xTaskCreate(
        USB_Task,
        "USB_Task",
        1024,      
        NULL,
        2,       
        &xUSBTaskHandle
    );
}