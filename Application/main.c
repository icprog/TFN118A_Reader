#include "app_init.h"
#include "radio_config.h"

extern uint8_t rtc_flag;//��ʱ����Ƶ����



uint8_t tx_cnt;//���Ա�־λ

int main(void)
{
	Radio_Init();//��Ƶ��ʼ��
	Raio_Deal();	
	while(1)
	{
//		if(rtc_flag)
//		{
//			tx_cnt++;
//			rtc_flag = 0;
//			Raio_Deal();//��Ƶ����		
//		}
		__WFI();
	}
}



