
#include "app_init.h"

#define rtc_interval 1  //��λs
#define rtc_base (32768*rtc_interval) - 1
/************************************************* 
Description:���õ�Ƶʱ��ʱ��Դ  
Input:
��1����source : 1:ѡ���ⲿ���� XOSC 0���ڲ�rc ROSC
Output:��
Return:��
*************************************************/  
static void lfclk_init(uint8_t source)
{
	uint8_t lfclksrc;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	//ѡ��ʱ��Դ
	lfclksrc = source ? CLOCK_LFCLKSRC_SRC_Xtal : CLOCK_LFCLKSRC_SRC_RC;
	NRF_CLOCK->LFCLKSRC = lfclksrc << CLOCK_LFCLKSRC_SRC_Pos;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) 
	{
	}	
}

/************************************************* 
Description:����0~255����� 
Input:��
Output:������ֵ
Return:��
*************************************************/ 
//uint8_t rng_value = 0;
static uint8_t random_vector_generate()
{
	static uint8_t rng_value = 0;
	rng_value = NRF_RNG->VALUE;
	NRF_RNG->EVENTS_VALRDY = 0;
	NRF_RNG->SHORTS = RNG_SHORTS_VALRDY_STOP_Enabled << RNG_SHORTS_VALRDY_STOP_Pos;
	NRF_RNG->TASKS_START = 1;
	return rng_value;
//	NRF_RNG->EVENTS_VALRDY = 0;
//	NRF_RNG->TASKS_START = 1;
//	while(NRF_RNG->EVENTS_VALRDY == 0);
//	rng_value = NRF_RNG->VALUE;
//	NRF_RNG->TASKS_STOP = 1;
//	return rng_value;
}
/************************************************* 
Description:rtc��ʼ��  
Input:
Output:��
Return:��
*************************************************/  
void rtc_init(void)
{
	lfclk_init(1);//1��XOSC 0��ROSC
	NRF_RTC0->PRESCALER = 0;//32.768khz Լ����0.03ms
	NRF_RTC0->CC[0] = rtc_base;//
	NRF_RTC0->EVENTS_COMPARE[0] = 0;//EVENTS_TICK
	NRF_RTC0->INTENSET =  RTC_INTENCLR_COMPARE0_Msk;//
	NRF_RTC0->TASKS_START = 1;
	
	NVIC_SetPriority(RTC0_IRQn,RTC_PRIORITY);
	NVIC_ClearPendingIRQ(RTC0_IRQn);
	NVIC_EnableIRQ( RTC0_IRQn );
}

/*
Description:�㲥�������0~7.65ms�������ʱ
Input:��
Output:��
Return:��
*/
void rtc_update_interval(void)
{
	uint8_t advDelay = random_vector_generate();//0~255 0~7.65ms
	NRF_RTC0->CC[0] = rtc_base + advDelay;
}
/*
Description:�����ⲿ����
Input:��
Output:��
Return:��
*/
void xosc_hfclk_start(void)
{
	/*���ⲿ����δ����ʱ���������ⲿ����*/
	if((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Xtal)!=1)
	{
		/*clear event*/
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
		/* Start 16 MHz crystal oscillator */		
		NRF_CLOCK->TASKS_HFCLKSTART = 1;

		/* Wait for the external oscillator to start up */
		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) 
		{
			
		}
		//�ȴ�HFCLK running
		while((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) == 0)
		{
		}
	}		
}

/*
Description:�ر��ⲿ����
Input:��
Output:��
Return:��
*/
void xosc_hfclk_stop(void)
{
	/**/
	if((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Xtal) == 1)
	{
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
	}		
}

#ifdef TFN118A
/*
Description:������ʼ��
Input:��
Output:��
Return:��
*/
void motor_init(void)
{
	NRF_GPIO->PIN_CNF[Motor_Pin_Num] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
										| (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
										| (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
										| (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
										| (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);	
}
/*
Description:�����״̬
Input:state :1:����� 0:���ֹͣ��
Output:��
Return:��
*/
void motor_run_state(u8 state)
{
	if( 1 == state )
		Motor_Run;
	else
		Motor_Stop;
}

/*
Description:�����ɼ���ʼ��
Input:��
Output:��
Return:��
*/
void battery_check_init(void)
{
    NRF_ADC->CONFIG = (ADC_CONFIG_RES_10bit << 0)//����10λ
                  | (0 << 2) //ADC����ֵ��������
                  | (0 << 5) //ѡ���ڲ�1.2VΪ�ο���ѹ
                  | (ADC_Pin_Num << 8);//���ò�����
 
    NRF_ADC->ENABLE = 0x01; 
}
/*
Description:�����ɼ�
Input:��
Output:
Return:��
*/
u8 battery_check_read(void)
{
		uint8_t adc_value,bat_range;
		if(adc_value < OneThreshold )
		{
			bat_range = battery_OneFifth;
		}
		else if(adc_value < TwoThreshold)
		{
			bat_range = battery_TwoFifth;
		}
		else if(adc_value < ThreeThreshold)
		{
			bat_range = battery_ThreeFifth;
		}
		else if(adc_value < FourThreshold)
		{
			bat_range = battery_FourFifth;
		}
		else
		{
			
		}
		return bat_range;
}
#endif


