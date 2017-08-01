#include "debug_log.h"

bool debug_log_init(void)
{
	bool result = true;
	
#if defined(ENABLE_UART_LOG)
//	result = UART_Init();
	UART_Init();
#else	// LCD
//	BSP_LCD_Init();
//	BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER);
//	BSP_LCD_SelectLayer(1);
//	
//	LCD_LOG_Init();
//	LCD_LOG_SetHeader((uint8_t*)"This is the header");
//	LCD_LOG_SetFooter((uint8_t*)"This is the footer");
#endif

	return result;
}
