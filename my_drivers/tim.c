#include "tim.h"
#include "app_var.h"
#include "rtc.h"
extern rtc_typedef Global_Time;//全局时间 
extern Para_Typedef Reader_Para;
#define rtc_cont 20 //1s定时
uint8_t rtc_cnt =rtc_cont;//rtc秒计数器

void TIMER0_IRQHandler()
{
	if(NRF_TIMER0->EVENTS_COMPARE[0])
	{
		NRF_TIMER0->EVENTS_COMPARE[0] = 0;
		//周期发送
		if(Reader_Para.radio_time_cnt_en)
		{
			Reader_Para.radio_time_cnt++;
			if(Reader_Para.radio_time_cnt > Reader_Para.radio_cycle_time)
			{
				Reader_Para.radio_time_cnt = 0;
				Reader_Para.radio_send_en = 1;
			}
		}
		//时钟
		rtc_cnt++;
		if(rtc_cont == rtc_cnt)
		{
			Calendar21Century(&Global_Time);//更新时间
			rtc_cnt = 0;
		}
			
	}
}

