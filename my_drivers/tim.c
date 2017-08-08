#include "tim.h"
#include "app_var.h"
#include "rtc.h"
#include "app_uart.h"
extern rtc_typedef Global_Time;//全局时间 
extern Para_Typedef Reader_Para;
uint8_t rtc_cnt =rtc_cont;//rtc秒计数器
extern Time_Typedef Time_type;//超时处理
uint8_t sec_flag;//秒计数
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
			sec_flag = 1;
		}
		//超时
		if(Time_type.Radio_Time_En)
		{
			Time_type.Radio_Time_Cnt++;//
		}
			
	}
}

