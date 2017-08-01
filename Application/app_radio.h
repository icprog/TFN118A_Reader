#ifndef _APP_RADIO_H
#define _APP_RADIO_H
#include "radio_config.h"
#include "nrf.h"
#include "app_init.h"
#include "app_var.h"


typedef enum
{
	RADIO_RUN_CONFIG_CHANNEL ,
	RADIO_RUN_DATA_CHANNEL
}RADIO_CHANNEL;

typedef enum
{
	Time_NoUpdate = 0,
	Time_Update=1
}Time_Update_Typedef;

extern void TID_RECORD_Clear(void);
extern void radio_pwr(uint8_t txpower);
extern void Raio_Deal(void);//…‰∆µ÷‹∆⁄∑¢ÀÕ
extern void radio_select(uint8_t ch,uint8_t dir);
uint8_t ID_CMP(const uint8_t *src,const uint8_t *dest);
#endif


