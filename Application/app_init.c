
#include "app_init.h"
#include "app_var.h"
#include "simple_uart.h"
#include "Debug_log.h"

#define rtc_interval 1  //��λs
#define rtc_base ((32768*rtc_interval) - 1)
/************************************************* 
@Description:���õ�Ƶʱ��ʱ��Դ  
@Input:
��1����source : 1:ѡ���ⲿ���� XOSC 0���ڲ�rc ROSC
@Output:��
@Return:��
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
@Description:����0~255����� 
@Input:��
@Output:������ֵ
@Return:��
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
@Description:rtc��ʼ��  
@Input:
@Output:��
@Return:��
*************************************************/  
void rtc0_init(void)
{
	lfclk_init(1);//1��XOSC 0��ROSC
	NRF_RTC0->PRESCALER = 0;//32.768khz Լ����0.03ms
	NRF_RTC0->CC[0] = rtc_base;//
	NRF_RTC0->EVENTS_COMPARE[0] = 0;//EVENTS_TICK
	NRF_RTC0->INTENSET =  RTC_INTENCLR_COMPARE0_Msk;//
//	NRF_RTC0->TASKS_START = 1;
	
	NVIC_SetPriority(RTC0_IRQn,RTC_PRIORITY);
	NVIC_ClearPendingIRQ(RTC0_IRQn);
	NVIC_EnableIRQ( RTC0_IRQn );
}
/************************************************* 
@Description:rtc��������
@Input:
@Output:��
@Return:��
*************************************************/  
void rtc0_start(void)
{
	NRF_RTC0->TASKS_START = 1;
}

/************************************************* 
@Description:rtcֹͣ����
@Input:
@Output:��
@Return:��
*************************************************/  
void rtc0_stop(void)
{
	NRF_RTC0->TASKS_STOP = 1;
}
/*
@Description:�㲥�������0~7.65ms�������ʱ
@Input:��
@Output:��
@Return:��
*/
void rtc_update_interval(void)
{
	uint8_t advDelay = random_vector_generate();//0~255 0~7.65ms
	NRF_RTC0->CC[0] = rtc_base + advDelay;
}
/*
@Description:�����ⲿ����
@Input:��
@Output:��
@Return:��
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
@Description:�ر��ⲿ����
@Input:��
@Output:��
@Return:��
*/
void xosc_hfclk_stop(void)
{
	/**/
	if((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Xtal) == 1)
	{
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
	}		
}
/*
@Description:��ʱ����ʼ��
@Input:���뾫ȷ��ʱ
@Output:��
@Return:��
*/
void timer0_init(uint8_t delayms)
{
	NRF_TIMER0->SHORTS     = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
    NRF_TIMER0->MODE           = TIMER_MODE_MODE_Timer;        // Set the timer in Timer Mode.
	NRF_TIMER0->TASKS_CLEAR    = 1;                            // clear the task first to be usable for later.
    NRF_TIMER0->PRESCALER      = 4;                            // 1us
    NRF_TIMER0->BITMODE        = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos); // 16 bit mode.
    NRF_TIMER0->INTENSET      = (TIMER_INTENSET_COMPARE0_Enabled<<TIMER_INTENSET_COMPARE0_Pos);
	NRF_TIMER0->CC[0]       = (uint32_t)delayms * 1000;
//	NRF_TIMER0->TASKS_START    = 1; // Start timer. 
	NVIC_SetPriority(TIMER0_IRQn, 2);
	NVIC_EnableIRQ(TIMER0_IRQn);
}
/*
@Description:��ʱ����������
@Input:
@Output:��
@Return:��
*/
void timer0_start(void)
{
	NRF_TIMER0->TASKS_START = 1;
}
/*
@Description:��ʱ��ֹͣ����
@Input:
@Output:��
@Return:��
*/	
void timer0_stop(void)
{
	NRF_TIMER0->TASKS_START = 1;
}
	


/*
@Description:���ڳ�ʼ��
@Input:��
@Output:��
@Return:��
*/
void UART_Init(void)
{
    simple_uart_config(RTS_PIN_NUMBER, TX_PIN_NUMBER, CTS_PIN_NUMBER, RX_PIN_NUMBER, HWFC);  
	NRF_UART0->INTENSET = (UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos)
						|(UART_INTENSET_ERROR_Enabled << UART_INTENSET_ERROR_Pos);
	NVIC_SetPriority(UART0_IRQn, UART0_PRIORITY);
    NVIC_EnableIRQ(UART0_IRQn);
}

/************************************************* 
Description:app��ʼ��
Input:��
Output:
Return:��
*************************************************/ 
void app_init(void)
{
	SystemParaInit();
	UART_Init();
	#ifdef LOG_ON
	debug_log_init();
	#endif
//	debug_printf("app_reader start");
	Radio_Init();//��Ƶ��ʼ��
	TID_RECORD_Clear();//��ջ���
	timer0_init(TIM0_TIME);//50ms��ʱ
	radio_select(DATA_CHANNEL,RADIO_RX);//�������״̬
	timer0_start();//��ʼ����
}




