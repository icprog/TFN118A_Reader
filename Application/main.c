#include "app_init.h"
#include "radio_config.h"
#include "app_reader.h"
#include "app_uart.h"
extern uint8_t rtc_flag;//��ʱ����Ƶ����

uint8_t tx_cnt;//���Ա�־λ

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
//			Raio_Deal();//��Ƶ����		
//		}
		__WFI();
	}
}



