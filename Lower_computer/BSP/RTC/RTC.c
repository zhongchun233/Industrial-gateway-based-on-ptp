/*
 * rtc.c
 *
 *  Created on: 2021年10月20日
 *      Author: Administrator
 */
 
#include "BSP_RTC.h"
 
//以下2行全局变量，用于RTC时间的读取与读入
RTC_DateTypeDef NowDate = {0};
RTC_TimeTypeDef NowTime = {0};

void RTC_Init(void) //用户自建的带有上电BPK判断的RTC初始化
{
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1)!=0X5050){ //判断是否首次上电
	   HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR1,0X5050); //标记数值 下次不执行“首次上电”的部分
	   RTC_Set(2022,1,1,0,0,0);//写入RTC时间的操作RTC_Set(4位年,2位月,2位日,2位时,2位分,2位秒)
  }
}
 

 // 设置时间（H7 标准 HAL 库）
uint8_t RTC_Set(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_DateTypeDef sDate = {0};
    RTC_TimeTypeDef sTime = {0};

    uint8_t week = 0;		
    int16_t y = syear;
    uint8_t m = smon;
    if (m == 1 || m == 2) {
        m += 12;
        y--;
    }
    // 计算结果：0=周日, 1=周一, 2=周二, 3=周三, 4=周四, 5=周五, 6=周六
    week = (23 + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;

    // HAL库通常定义：RTC_WEEKDAY_MONDAY=1 ... RTC_WEEKDAY_SUNDAY=7
    // 我们需要把公式算出的 0(周日) 映射为 7，其他保持不变
    if (week == 0) week = 7;

		
    sDate.Year = syear - 2000;
    sDate.Month = smon;
    sDate.Date = sday;
    sDate.WeekDay = week;

    sTime.Hours = hour;
    sTime.Minutes = min;
    sTime.Seconds = sec;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        return 1;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return 2;

    return 0;
}

// 读取时间（H7 标准 HAL 库）
uint8_t RTC_Get(void)
{
    RTC_DateTypeDef sDate = {0};
    RTC_TimeTypeDef sTime = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    NowDate.Year = sDate.Year;
    NowDate.Month = sDate.Month;
    NowDate.Date = sDate.Date;
		NowDate.WeekDay = sDate.WeekDay;
    NowTime.Hours = sTime.Hours;
    NowTime.Minutes = sTime.Minutes;
    NowTime.Seconds = sTime.Seconds;


    return 0;
}
