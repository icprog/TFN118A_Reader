#include "app_init.h"
#include "radio_config.h"

extern uint8_t rtc_flag;//定时，射频发送



uint8_t tx_cnt;//测试标志位

int main(void)
{
	Radio_Init();//射频初始化
	Raio_Deal();	
	while(1)
	{
//		if(rtc_flag)
//		{
//			tx_cnt++;
//			rtc_flag = 0;
//			Raio_Deal();//射频功能		
//		}
		__WFI();
	}
}



