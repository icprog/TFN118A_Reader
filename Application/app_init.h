#ifndef _APP_INIT_H
#define _APP_INIT_H
#include "nrf.h"
#include "sys.h"
#include "app_key.h"


//io定义
#ifdef TFN118A

#define Motor_Pin_Num 0
#define Motor_Run NRF_GPIO->OUTSET = 1 << Motor_Pin_Num
#define Motor_Stop NRF_GPIO->OUTCLR = 1 << Motor_Pin_Num

#define ADC_Pin_Num ADC_CONFIG_PSEL_AnalogInput2

//电量采集参数定义
#define OneThreshold 1     //
#define TwoThreshold 2
#define ThreeThreshold 3
#define FourThreshold 4
#define FiveThreshold 5
typedef enum
{
    battery_OneFifth = 0,
    battery_TwoFifth    = 1,
    battery_ThreeFifth     = 2,
		battery_FourFifth     = 3,
    battery_Full     = 4
} battery_typedef;


#endif

//中断优先级定义
typedef enum
{
    APP_IRQ_PRIORITY_HIGHEST = 0,
    APP_IRQ_PRIORITY_HIGH    = 1,
    APP_IRQ_PRIORITY_MID     = 2,
    APP_IRQ_PRIORITY_LOW     = 3
} app_irq_priority_t;


#define RADIO_PRIORITY		APP_IRQ_PRIORITY_HIGH
#define RTC_PRIORITY		APP_IRQ_PRIORITY_HIGHEST
#define TIM_PRIORITY      	APP_IRQ_PRIORITY_HIGHEST
#define PORT_PRIORITY    	APP_IRQ_PRIORITY_HIGH


//ext function
extern void rtc_init(void);//1s定时，用来时间定时和射频周期发送
extern void rtc_update_interval(void);//增加随机时间
extern void xosc_hfclk_start(void);//射频发送，需要启动外部16M晶振
extern void xosc_hfclk_stop(void);//停止射频发送时，关闭外部晶振

#ifdef TFN118A
extern void motor_init(void);//震动马达初始化
extern void motor_run_state(u8 state);//振动马达状态
extern void battery_check_init(void);//电量采集初始化
extern u8 battery_check_read(void);//电量采集
#endif

#endif

