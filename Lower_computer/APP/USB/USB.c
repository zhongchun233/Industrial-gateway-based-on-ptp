#include "USB.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"    // 用于 sprintf
#include "fatfs.h"
#include "ff.h"
#include "usb_host.h"
#include "shell.h"
#include "shell_port.h"
#include "shell_cfg.h"
#include "shell_ext.h"
#include "cmsis_os.h"
// 任务句柄 (可选，用于在其他地方控制任务)
TaskHandle_t xUSBTaskHandle = NULL;
extern ApplicationTypeDef Appli_state;
// 在文件开头先写 UTF-8 BOM（3字节）
const uint8_t bom[] = {0xEF, 0xBB, 0xBF};
const uint8_t text[] = "test ok！！！\r\n";

static void MSC_Application(void) //USB大容量存储区测试代码
{
    FRESULT res;
    uint32_t byteswritten;  //file write/read counts
    uint8_t wtext[] = "This is USB 设备 Mass Storage Application Example!\r\n";

    #if 1
    /*以下代码测试ok，可以向STM32_USB.txt中写入数据*/
    /*如果要使用下面的测试代码，将#if 0 改为#if 1*/
    if(f_mount(&USBHFatFS,(const TCHAR*)USBHPath,1) !=FR_OK)
    {
        //Fatfs Initialization Error
        Error_Handler();
        shellPrint(&shell," mount USB fail!!! \r\n");
    }
    else
    {
        shellPrint(&shell," mount USB success!!! \r\n");
        if(f_open(&USBHFile, "STM32_USB.txt", FA_CREATE_ALWAYS | FA_WRITE) !=FR_OK)
        {
            // "STM32_USB.txt" file open for write error
            Error_Handler();
            shellPrint(&shell," open usb file fail!!! \r\n");
        }
        else
        {
            shellPrint(&shell," open usb file success!!! \r\n");
            //write data to the text file
            res = f_write(&USBHFile, wtext, sizeof(wtext), (void *)&byteswritten);
            if((byteswritten==0) || (res != FR_OK)){
                Error_Handler();
                shellPrint(&shell," write usb file fail!!! \r\n");
            }
            else
            {
                shellPrint(&shell," write usb file success!!! \r\n");
                //close the open text file
                f_close(&USBHFile);
            }
            shellPrint(&shell," operate USB file end \r\n");    
        }
    }
    #endif
}
 
void UsbTest(void)
{
    
    
    switch(Appli_state)
    {
        case APPLICATION_READY:
            MSC_Application();
            Appli_state = APPLICATION_DISCONNECT;
            break;
        
        case APPLICATION_DISCONNECT:
            f_mount(NULL, (const TCHAR*)"",0);
            // Appli_state = APPLICATION_READY;
            break;
        
        default:
            break;
    }
}


// USB 主任务
void USB_Task(void *pvParameters)
{
	shellPrint(&shell, "\r\nUSB_Task start\r\n");
    for(;;)
    {

				UsbTest();
        osDelay(pdMS_TO_TICKS(500));
    }
}

// 创建USB任务
void Create_USB_Task(void)
{
    xTaskCreate(
        USB_Task,
        "USB_Task",
        1024,      // 堆栈必须够大
        NULL,
        2,        // 优先级 2
        &xUSBTaskHandle
    );
}