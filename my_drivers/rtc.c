#include "rtc.h"
#include "app_init.h"

uint8_t rtc_flag;//定时，射频发送
//if(1==get_uart1_ready(0xffff))
//{
/*BCD日历算法*/
uint8_t BCDInc(uint8_t *ucByte, uint8_t ucMin, uint8_t ucMax)
{
	if(*ucByte<ucMin||*ucByte>ucMax) *ucByte=ucMin;
	if(*ucByte==ucMax)
	{
		*ucByte=ucMin;
		return 1;
	}
	if((++*ucByte&0x0f)>9) *ucByte+=6;
	return 0;
}

//根据提供的年、月计算当月最后一日，支持闰年，BCD格式
uint8_t DateMaxCalc21Cn(uint8_t ucBcdYeah, uint8_t ucBcdMonth)
{
	uint8_t ucBcdtmp1;

	if(ucBcdMonth&0x10) ucBcdtmp1=(ucBcdMonth&0x01)?0x30:0x31;	//10,11,12
	else
	{
		if(ucBcdMonth==2)	//2
		{
			ucBcdtmp1=0x28;
			if((ucBcdYeah&0x01)==0)
			{
				if(ucBcdYeah&0x02)
				{
					if(ucBcdYeah&0x10) ucBcdtmp1=0x29;
				}
				else
				{
					if((ucBcdYeah&0x10)==0) ucBcdtmp1=0x29;
				}
			}
		}
		else	//1,3~9
		{
			ucBcdtmp1=(ucBcdMonth&0x08)?(ucBcdMonth-1):ucBcdMonth;	//8,9-->7,8
			ucBcdtmp1=(ucBcdtmp1&0x01)?0x31:0x30;	
		}				
	}
	return ucBcdtmp1;
}



void RTC0_IRQHandler(void)
{
	if(NRF_RTC0->EVENTS_COMPARE[0])
	{
		NRF_RTC0->EVENTS_COMPARE[0]=0UL;	//clear event
		NRF_RTC0->TASKS_CLEAR=1UL;	//clear count
		rtc_update_interval();
		rtc_flag=1;
	}
}

