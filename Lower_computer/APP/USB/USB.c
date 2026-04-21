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


static void MSC_Application(void) //USB大容量存储区测试代码
{
    FRESULT res;
    uint32_t byteswritten;  //file write/read counts
    //uint8_t wtext[] = "This is USB Mass Storage Application Example!\r\n";
    uint8_t wtext[] = "这是一个STM32CubeMX_FATFS_FREERTOS_USB测试!\r\n";
    
    #if 1
    /*以下代码测试ok，可以向STM32_USB.txt中写入数据*/
    
    uint32_t bytesread;       // 读文件计数
    uint8_t rtext[1024];      // 读取的buff
    
    shellPrint(&shell,"\r\n MSC_Application USB \r\n");
    
    //1.挂载USB
    res = f_mount(&USBHFatFS,(const TCHAR*)USBHPath,1);
    if(res != FR_OK)
    {
        Error_Handler();
        shellPrint(&shell," mount USB fail: %d \r\n",res);
    }
    else
    {
        shellPrint(&shell," mount USB success!!! \r\n");
 
        //2.打开
        res = f_open(&USBHFile, "STM32_USB.txt", FA_CREATE_ALWAYS | FA_WRITE);
        if(res != FR_OK)
        {
            Error_Handler();
            shellPrint(&shell," open file error : %d\r\n",res);
        }
        else
        {
            shellPrint(&shell," open file success!!! \r\n");
            
            //3.写数据
            res = f_write(&USBHFile, wtext, sizeof(wtext), (void *)&byteswritten);    //在文件内写入wtext内的内容
            if(res != FR_OK)
            {
                Error_Handler();
                shellPrint(&shell," write file error : %d\r\n",res);    
                
            }
            shellPrint(&shell," write file success!!! writen count: %d \r\n",byteswritten);
            shellPrint(&shell," write Data : %s\r\n",wtext);
            
            //4.关闭文件
            res = f_close(&USBHFile);
        }
    }
    
    shellPrint(&shell," read USB mass storage data.\r\n");
    //5.打开文件
    res = f_open(&USBHFile, "STM32_USB.txt", FA_READ);                //打开文件，权限为只读
    if(res != FR_OK)
    {                                    //返回值不为0（出现问题）
        Error_Handler();
        shellPrint(&shell," open file error : %d\r\n",res);        //打印问题代码
    }
    else
    {
        shellPrint(&shell," open file success!!! \r\n");
        
        //6.读取txt文件数据
        res = f_read(&USBHFile, rtext, sizeof(rtext), (UINT*)&bytesread);//读取文件内容放到rtext中
        if(res)
        {                                                    //返回值不为0（出现问题）
            Error_Handler();
            shellPrint(&shell," read error!!! %d\r\n",res);                    //打印问题代码
        }
        else
        {
            shellPrint(&shell," read success!!! \r\n");
            shellPrint(&shell," read Data : %s\r\n",rtext);                   //打印读取到的数据
        }
        
        //7.读写一致性检测
        if(bytesread == byteswritten)                                        //如果读写位数一致
        { 
            shellPrint(&shell," bytesread == byteswritten，写入与读出数据一致。\r\n");    //打印文件系统工作正常
        }
        
        //8.关闭文件
        res = f_close(&USBHFile);
    }
    #endif
    
    #if 0
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
            break;
        
        default:
            break;
    }
}


// USB 主任务
void USB_Task(void *pvParameters)
{
	shellPrint(&shell, "\r\nU	SB_Task start\r\n");
    for(;;)
    {

				UsbTest();
        osDelay(pdMS_TO_TICKS(50));
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