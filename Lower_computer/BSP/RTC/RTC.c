/*
 * rtc.c
 *
 *  Created on: 2021年10月20日
 *      Author: Administrator
 */
 
#include "BSP_RTC.h"
 
//以下2行全局变量，用于RTC时间的读取与读入
uint16_t ryear; //4位年
uint8_t rmon,rday,rhour,rmin,rsec,rweek;//2位月日时分秒周


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
 
//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
uint8_t Is_Leap_Year(uint16_t year){
	if(year%4==0){ //必须能被4整除
		if(year%100==0){
			if(year%400==0)return 1;//如果以00结尾,还要能被400整除
			else return 0;
		}else return 1;
	}else return 0;
}
//设置时钟
//把输入的时钟转换为秒钟
//以1970年1月1日为基准
//1970~2099年为合法年份
 
//月份数据表
uint8_t const table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表
const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};//平年的月份日期表
 // 设置时间（H7 标准 HAL 库）
uint8_t RTC_Set(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_DateTypeDef sDate = {0};
    RTC_TimeTypeDef sTime = {0};

    sDate.Year = syear - 2000;
    sDate.Month = smon;
    sDate.Date = sday;
    sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;

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

    ryear = 2000 + sDate.Year;
    rmon = sDate.Month;
    rday = sDate.Date;
    rhour = sTime.Hours;
    rmin = sTime.Minutes;
    rsec = sTime.Seconds;
    rweek = sDate.WeekDay;

    return 0;
}

// 计算星期（简化版）
uint8_t RTC_Get_Week(uint16_t year, uint8_t month, uint8_t day)
{
    (void)year; (void)month; (void)day;
    RTC_Get();
    return rweek;
}