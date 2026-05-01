#include "USB.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"    // ���� sprintf
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
#include "stdarg.h"

TaskHandle_t xUSBTaskHandle = NULL;
extern ApplicationTypeDef Appli_state;
extern RTC_DateTypeDef NowDate;
extern RTC_TimeTypeDef NowTime;

const uint8_t bom[] = {0xEF, 0xBB, 0xBF};
const uint8_t text[] = "test ok������\r\n";

static FIL logFile;
static uint8_t usb_mounted = 0;
static uint32_t log_write_counter = 0;
static char log_buffer[512];

/**
 * @brief USB�������洢(MSC)Ӧ�ó���������
 * �ú��������ʼ��USB�洢�豸��������־�ļ���д���ʼ���
 */
static uint8_t MSC_Log_Start(void)
{
    FRESULT res;                        // FAT�ļ�ϵͳ�������
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
 * @brief д��USB��־����
 * �˺������ڽ�����ʱ�������־��Ϣд��USB�洢�豸
 */
void USB_Log_Write_Periodic(void)
{
    // ���USB�豸�Ƿ��ѹ��أ���δ������ֱ�ӷ���
    if(!usb_mounted)
        return;

    FRESULT res;           // FAT�ļ�ϵͳ�������
    uint32_t byteswritten; // ʵ��д����ֽ���


    // ��ȡ��ǰRTCʱ��
    RTC_Get();

    // ����ʽ����ʱ�������־��������Ϣд����־������
    int len = snprintf(log_buffer, sizeof(log_buffer),
                      "[%04d-%02d-%02d %02d:%02d:%02d] Log #%lu\r\n",
                      NowDate.Year + 2000, NowDate.Month, NowDate.Date,  // ������
                      NowTime.Hours, NowTime.Minutes, NowTime.Seconds,  // ʱ����
                      log_write_counter++);                             // ��־����������

    // ����־����������д���ļ�
    res = f_write(&logFile, log_buffer, len, (void *)&byteswritten);
    // ���д���������ʧ�ܻ�д���ֽ���Ϊ0�����ӡ������Ϣ
    if(res != FR_OK || byteswritten == 0)
    {
        shellPrint(&shell, "Log write failed!\r\n");
    }else
    {
        f_sync(&logFile);
    }
}

/**
 * @brief printf ��ʽ USB ��־д��
 * ֧�ֿ�ѡ�ĸ�ʽ字符串，ʹ�ñ̶ȵ�printf()
 * ׼ȷ�ӷ�ʱ�䣬ҳ�Զ�ӷ�\r\n
 */
void USB_Log_Printf(const char *format, ...)
{
    if(!usb_mounted)
        return;

    va_list args;
    uint32_t byteswritten;
    FRESULT res;
    char temp_buffer[512];
    int len;

    // ��ȡ��ǰʱ��
    RTC_Get();

    // ׼ȷ�ӷ�ʱ�䣬ҳ�Զ�ӷ�\r\n
    va_start(args, format);
    len = vsnprintf(temp_buffer, sizeof(temp_buffer) - 10, format, args);
    va_end(args);

    if(len > 0)
    {
        // ׼ȷ�ӷ�ʱ�䣬ҳ�Զ�ӷ�\r\n
        int final_len = snprintf(log_buffer, sizeof(log_buffer),
                                "[%04d-%02d-%02d %02d:%02d:%02d] %s",
                                NowDate.Year + 2000, NowDate.Month, NowDate.Date,
                                NowTime.Hours, NowTime.Minutes, NowTime.Seconds,
                                temp_buffer);

        // ׼ȷ�ӷ�\r\n
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
        if(res == FR_OK && byteswritten > 0)
        {
            f_sync(&logFile);
        }
    }
}

/**
 * @brief MSC��־��������
 * �ú������ڴ���USB�洢�豸(MSC)����־��¼����
 * ����USB�豸�Ƿ���أ�ִ�в�ͬ�Ĳ���
 */
static void MSC_Log(void)
{
 
    // ���USB�豸�Ƿ��ѹ���
    if(!usb_mounted)
    {
        // ���δ���أ���������MSC��־����
        if(MSC_Log_Start())
        {
            // ��������ɹ������ù��ر�־Ϊ1
            usb_mounted = 1;
        }  

    }
    else
    {
        // ����ѹ��أ�ִ��USB��־д��
        USB_Log_Write_Periodic();
    }
    
}
static void MSC_Application(void)
{


    switch(Appli_state)
    {
        case APPLICATION_READY:

             MSC_Log();

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
        osDelay(pdMS_TO_TICKS(100));
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