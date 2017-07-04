#include "nrf_gpio.h"

#define LED0 18
#define LED0_ON         NRF_GPIO->OUTSET = 1<<LED0;
#define LED0_OFF        NRF_GPIO->OUTCLR = 1<<LED0


