#include "USB.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"    // 锟斤拷锟斤拷 sprintf
#include "fatfs.h"
#include "ff.h"
#include "usb_host.h"
#include "shell.h"
#include "shell_port.h"
#include "shell_cfg.h"
#include "shell_ext.h"
#include "cmsis_os.h"
#include "BSP_RTC.h"
#include "string.h"

TaskHandle_t xUSBTaskHandle = NULL;
extern ApplicationTypeDef Appli_state;
extern RTC_DateTypeDef NowDate;
extern RTC_TimeTypeDef NowTime;



static FIL logFile;
static uint8_t usb_mounted = 0;
static char log_buffer[512];

/**
 * @brief USB大容量存储(MSC)应用程序主函数
 * 该函数负责初始化USB存储设备，创建日志文件并写入初始标记
 */
static uint8_t MSC_Log_Start(void)
{
    FRESULT res;                        // FAT文件系统操作结果
    uint32_t byteswritten;  //file write/read counts
    if(f_mount(&USBHFatFS,(const TCHAR*)USBHPath,1) !=FR_OK)
    {
        //Fatfs Initialization Error
        shellPrint(&shell," mount USB fail!!! \r\n");        
        Error_Handler();

        return 0;
    }
    else
    {
        shellPrint(&shell," mount USB success!!! \r\n");


        if(f_open(&logFile, "log.txt", FA_OPEN_ALWAYS | FA_WRITE) !=FR_OK)
        {
            shellPrint(&shell," open log file fail!!! \r\n");

        }
        else
        {
            shellPrint(&shell," open log file success!!! \r\n");

            f_lseek(&logFile, f_size(&logFile));

            char header[] = "\r\n========== Log Start ==========\r\n";
            f_write(&logFile, header, strlen(header), (void *)&byteswritten);

        } 
        return 1;
    }
}

/**
 * @brief 写入USB日志函数
 * 此函数用于将带有时间戳的日志信息写入USB存储设备
 */
void USB_Log_Write(const char *fmt, ...)
{
    // 检查USB设备是否已挂载，若未挂载则直接返回
    if(!usb_mounted)
        return;

    FRESULT res;           // FAT文件系统操作结果
    uint32_t byteswritten; // 实际写入的字节数


    // 获取当前RTC时间
    RTC_Get();

    // 将格式化的时间戳和日志计数器信息写入日志缓冲区
    int len = snprintf(log_buffer, sizeof(log_buffer),
                      "[%04d-%02d-%02d %02d:%02d:%02d]:\r\n",
                      NowDate.Year + 2000, NowDate.Month, NowDate.Date,  // 年月日
                      NowTime.Hours, NowTime.Minutes, NowTime.Seconds); 

    // 将日志缓冲区内容写入文件
    res = f_write(&logFile, log_buffer, len, (void *)&byteswritten);
    // 检查写入结果，如果失败或写入字节数为0，则打印错误信息
    if(res != FR_OK || byteswritten == 0)
    {
        shellPrint(&shell, "Log write failed!\r\n");
    }else                                                                                          
    {                                            
        f_sync(&logFile);                                                                          
    }   
}

void USB_Log_Printf(const char *format, ...)
{
    if(!usb_mounted)
        return;

    va_list args;
    uint32_t byteswritten;
    FRESULT res;
    int len;

    RTC_Get();
    len = snprintf(log_buffer, sizeof(log_buffer),
                      "[%04d-%02d-%02d %02d:%02d:%02d]",
                      (int)NowDate.Year + 2000, (int)NowDate.Month, (int)NowDate.Date,  // 年月日
                      (int)NowTime.Hours, (int)NowTime.Minutes, (int)NowTime.Seconds); 
    uint8_t time_len = strlen(log_buffer);

    va_start(args, format);                    
    int final_len = vsnprintf(log_buffer + time_len, 
                sizeof(log_buffer) - time_len - 2, format, args);                                   
    va_end(args);

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
    if(res != FR_OK || byteswritten == 0)
    {
        // f_sync(&logFile);
        f_close(&logFile);
        shellPrint(&shell,"USB MSC device is disconnected.\r\n");
        f_mount(NULL, (const TCHAR*)"",0);
        usb_mounted = 0;
    }

}
/**
 * @brief MSC日志处理函数
 * 该函数用于处理USB存储设备(MSC)的日志记录功能
 * 根据USB设备是否挂载，执行不同的操作
 */
static void MSC_Log_Application(void)
{
 
    // 检查USB设备是否已挂载
    if(!usb_mounted)
    {
        // 如果未挂载，尝试启动MSC日志功能
        if(MSC_Log_Start())
        {
            // 如果启动成功，设置挂载标志为1
            usb_mounted = 1;
        }  

    }
    else
    {
        // 如果已挂载，执行USB日志写入
        USB_Log_Printf("time:%d.\r\n", (int)NowTime.Seconds);
        
    }
    
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
                f_close(&logFile);
                shellPrint(&shell,"USB MSC device is disconnected.\r\n");
                f_mount(NULL, (const TCHAR*)"",0);
                usb_mounted = 0;
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