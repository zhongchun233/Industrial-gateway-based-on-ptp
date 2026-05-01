
#include "BSP_RTC.h"
 
//����2��ȫ�ֱ���������RTCʱ��Ķ�ȡ�����
RTC_DateTypeDef NowDate = {0};
RTC_TimeTypeDef NowTime = {0};

void RTC_Init(void) //�û��Խ��Ĵ����ϵ�BPK�жϵ�RTC��ʼ��
{
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    if (RTC_ClockSource == RCC_RTCCLKSOURCE_LSI)
    {
        hrtc.Init.AsynchPrediv = 124;
        hrtc.Init.SynchPrediv = 255;
    }
    else
    {
        hrtc.Init.AsynchPrediv = 1;
        hrtc.Init.SynchPrediv = 1;
    }
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;  // ��������ֶ�
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1)!=0X5050){ //�ж��Ƿ��״��ϵ�
		 HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR1,0);
	   HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR1,0X5050); //�����ֵ �´β�ִ�С��״��ϵ硱�Ĳ���
	   RTC_Set(2022,1,1,0,0,0);//д��RTCʱ��Ĳ���RTC_Set(4λ��,2λ��,2λ��,2λʱ,2λ��,2λ��)
  }
}
 

 // ����ʱ�䣨H7 ��׼ HAL �⣩
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
    // ��������0=����, 1=��һ, 2=�ܶ�, 3=����, 4=����, 5=����, 6=����
    week = (23 + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;

    // HAL��ͨ�����壺RTC_WEEKDAY_MONDAY=1 ... RTC_WEEKDAY_SUNDAY=7
    // ������Ҫ�ѹ�ʽ����� 0(����) ӳ��Ϊ 7���������ֲ���
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

// ��ȡʱ�䣨H7 ��׼ HAL �⣩
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
