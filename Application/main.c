#include "app_init.h"
#include "radio_config.h"
#include "app_reader.h"
#include "app_uart.h"
extern uint8_t rtc_flag;//定时，射频发送

uint8_t tx_cnt;//测试标志位

int main(void)
{
	app_init();
	while(1)
	{
		Uart_Deal();
		app_process();
//		if(rtc_flag)
//		{
//			tx_cnt++;
//			rtc_flag = 0;
//			Reader_RadioDeal();//射频功能		
//		}
		__WFI();
	}
}



