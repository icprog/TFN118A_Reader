#ifndef _APP_RADIO_H
#define _APP_RADIO_H
#include "nrf.h"
#include "radio_config.h"
#include "app_init.h"
#include "app_var.h"

extern void radio_pwr(uint8_t txpower);
extern void Raio_Deal(void);//射频周期发送
#endif


