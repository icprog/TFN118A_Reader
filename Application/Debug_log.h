/*******************************************************************************
** 版权:		
** 文件名: 		Debug_log.h
** 版本：  		1.0
** 工作环境: 	MDK-ARM 5.23
** 作者: 		cc
** 生成日期: 	2017-07-29
** 功能:		  
** 相关文件:	
** 修改日志：	
** 版权所有   
** 使用说明
需要先初始化，比如调用一下函数
#ifdef LOG_ON
	debug_log_init();
#endif
当然需要定义LOG_ON
//调用debug_printf，需要
#define ENABLE_DEBUG_PRINT

*******************************************************************************/


#ifndef __debug_log_h__
#define __debug_log_h__


// c
#include <stdbool.h>

#ifdef LOG_ON
#define ENABLE_UART_LOG

#define ENABLE_LOG_PRINT

#define ENABLE_DEBUG_PRINT
#endif


#if defined(ENABLE_UART_LOG)
#	include "simple_uart.h"
#include "app_init.h"
#	include <string.h>
#	include <stdarg.h>
#	include <stdio.h>
	static char uart_buffer[256];
	static void uart_printf(const char *format, ...)
	{
		va_list args;
		int char_num = 0;
		
		if (!format)
			return;
	
		va_start(args, format);
		//char_num = vsiprintf(uart_buffer, format, args);
		//char_num = _vsprintf(uart_buffer, format, args);
		char_num = vsprintf(uart_buffer, format, args);
		va_end(args);
	
		UART_Send((uint8_t*)uart_buffer, (uint16_t)char_num);
	}
#else	// LCD
//#	include "lcd_log.h"
#endif


// must be call before system_clock_init() on stm32f4
bool	debug_log_init(void);


#if defined(ENABLE_UART_LOG)
#	define debug_log(...)	do { uart_printf(__VA_ARGS__); } while(0)
#	define debug_log_1(...)	do { uart_printf(__VA_ARGS__); } while(0)
#	define debug_log_2(...)	do { uart_printf(__VA_ARGS__); } while(0)
#	define debug_log_3(...)	do { uart_printf(__VA_ARGS__); } while(0)
#	define debug_log_4(...)	do { uart_printf(__VA_ARGS__); } while(0)
#else
#	define  debug_log(...)    do { } while(0)
#	define  debug_log_1(...)    do { } while(0)
#	define  debug_log_2(...)    do { } while(0)
#	define  debug_log_3(...)    do { } while(0)
#	define  debug_log_4(...)    do { } while(0)
#endif


#if defined(ENABLE_LOG_PRINT)
#	define log_printf(...)	do { debug_log(__VA_ARGS__); } while(0)
#else
#	define log_printf(...)	do { } while(0) // empty
#endif

#if defined(ENABLE_DEBUG_PRINT)
#	define debug_printf(...)	do { debug_log(__VA_ARGS__); } while(0)
#	define debug_printf_1(...)	do { debug_log_1(__VA_ARGS__); } while(0)
#	define debug_printf_2(...)	do { debug_log_2(__VA_ARGS__); } while(0)
#	define debug_printf_3(...)	do { debug_log_3(__VA_ARGS__); } while(0)
#	define debug_printf_4(...)	do { debug_log_4(__VA_ARGS__); } while(0)
#else
#	define debug_printf(...)	do { } while(0) // empty
#	define debug_printf_1(...)	do { } while(0) // empty
#	define debug_printf_2(...)	do { } while(0) // empty
#	define debug_printf_3(...)	do { } while(0) // empty
#	define debug_printf_4(...)	do { } while(0) // empty
#endif						


#endif	// header guard
