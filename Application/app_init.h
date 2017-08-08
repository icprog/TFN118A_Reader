#ifndef _APP_INIT_H
#define _APP_INIT_H
#include "nrf.h"
#include "sys.h"



//串口定义
#define RX_PIN_NUMBER  11    // UART RX pin number.
#define TX_PIN_NUMBER 9   // UART TX pin number.
#define CTS_PIN_NUMBER 6   // UART Clear To Send pin number. Not used if HWFC is set to false
#define RTS_PIN_NUMBER 7    // Not used if HWFC is set to false 
#define HWFC           false // UART hardware flow control
	
#define TIM0_TIME 50  //50MS计数一次

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
#define PORT_PRIORITY    	APP_IRQ_PRIORITY_HIGH
#define UART0_PRIORITY 		APP_IRQ_PRIORITY_HIGHEST
#define TIM0_PRIORITY      	APP_IRQ_PRIORITY_LOW

//ext function
extern void rtc_init(void);//1s定时，用来时间定时和射频周期发送
extern void rtc_update_interval(void);//增加随机时间
extern void xosc_hfclk_start(void);//射频发送，需要启动外部16M晶振
extern void xosc_hfclk_stop(void);//停止射频发送时，关闭外部晶振
extern void app_init(void);
void UART_Init(void);


#endif

