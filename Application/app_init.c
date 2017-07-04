
#include "app_init.h"

#define rtc_interval 1  //单位s
#define rtc_base (32768*rtc_interval) - 1
/************************************************* 
Description:配置低频时钟时钟源  
Input:
（1）、source : 1:选择外部晶振 XOSC 0：内部rc ROSC
Output:无
Return:无
*************************************************/  
static void lfclk_init(uint8_t source)
{
	uint8_t lfclksrc;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	//选择时钟源
	lfclksrc = source ? CLOCK_LFCLKSRC_SRC_Xtal : CLOCK_LFCLKSRC_SRC_RC;
	NRF_CLOCK->LFCLKSRC = lfclksrc << CLOCK_LFCLKSRC_SRC_Pos;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) 
	{
	}	
}

/************************************************* 
Description:产生0~255随机数 
Input:无
Output:输出随机值
Return:无
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
Description:rtc初始化  
Input:
Output:无
Return:无
*************************************************/  
void rtc_init(void)
{
	lfclk_init(1);//1：XOSC 0：ROSC
	NRF_RTC0->PRESCALER = 0;//32.768khz 约等于0.03ms
	NRF_RTC0->CC[0] = rtc_base;//
	NRF_RTC0->EVENTS_COMPARE[0] = 0;//EVENTS_TICK
	NRF_RTC0->INTENSET =  RTC_INTENCLR_COMPARE0_Msk;//
	NRF_RTC0->TASKS_START = 1;
	
	NVIC_SetPriority(RTC0_IRQn,RTC_PRIORITY);
	NVIC_ClearPendingIRQ(RTC0_IRQn);
	NVIC_EnableIRQ( RTC0_IRQn );
}

/*
Description:广播间隔增加0~7.65ms的随机延时
Input:无
Output:无
Return:无
*/
void rtc_update_interval(void)
{
	uint8_t advDelay = random_vector_generate();//0~255 0~7.65ms
	NRF_RTC0->CC[0] = rtc_base + advDelay;
}
/*
Description:启动外部晶振
Input:无
Output:无
Return:无
*/
void xosc_hfclk_start(void)
{
	/*当外部晶振未启动时，才启动外部晶振*/
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
		//等待HFCLK running
		while((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) == 0)
		{
		}
	}		
}

/*
Description:关闭外部晶振
Input:无
Output:无
Return:无
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
Description:震动马达初始化
Input:无
Output:无
Return:无
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
Description:震动马达状态
Input:state :1:马达震动 0:马达停止震动
Output:无
Return:无
*/
void motor_run_state(u8 state)
{
	if( 1 == state )
		Motor_Run;
	else
		Motor_Stop;
}

/*
Description:电量采集初始化
Input:无
Output:无
Return:无
*/
void battery_check_init(void)
{
    NRF_ADC->CONFIG = (ADC_CONFIG_RES_10bit << 0)//精度10位
                  | (0 << 2) //ADC测量值等于输入
                  | (0 << 5) //选择内部1.2V为参考电压
                  | (ADC_Pin_Num << 8);//配置采样脚
 
    NRF_ADC->ENABLE = 0x01; 
}
/*
Description:电量采集
Input:无
Output:
Return:无
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


