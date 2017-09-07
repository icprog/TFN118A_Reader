#ifndef _APP_UART
#define _APP_UART
#include "sys.h"

/*----------------串口定义------------------*/
#define head_bytes 2
#define len_bytes  2
#define crc_bytes 2
#define len_finish 3
#define pkt_head1 0xAA
#define pkt_head2 0xAA
#define U_PROTOCOL_VER 0X00
/****************************************************
串口通信 上位机->接收器
描述	帧头	长度	协议版本	读写器ID	定位信息	协议流水号 
IDX		0~1		2~3	 	4			5~8			9			10
描述	命令字	信息内容
IDX		11		12

******************************************************/
#define U_HEAD_LEN 2
#define U_LENTH_LEN 2
#define U_ID_LEN 4
#define U_STATE_LEN 2
#define U_PROTOCOL_LEN 1
#define U_ConfigOT_LEN 1   //下发配置指令，超时字节
#define U_HEADER_IDX 0
#define U_LEN_IDX 2
#define U_PROTOCOL_IDX 4
#define U_ID_IDX 5
#define	U_GPS_IDX 9

#define U_FIX_LEN 6 //帧长2+长度2+校验
#define U_AfterLEN_FIX_LEN (U_ID_LEN+1+1+1+1) //buf[2] - U_ReaderFIX_LEN = 信息内容长度
/****************************************************
串口通信 接收器->上位机
描述	帧头	长度	协议版本 	读写器ID	定位信息	协议流水号 
IDX		0~1		2~3	 	4			5~8			9/9~36		10/37
描述	命令字	信息内容
IDX		11/38	12/39

******************************************************/
#ifdef GPS
#define	U_TXGPS_IDX 9
#define U_SEQ_IDX 37
#define U_TXCMD_IDX 38
#else
#define	U_TXGPS_IDX 9
#define U_SEQ_IDX 10
#define U_TXCMD_IDX 11
#define U_TXGPS_Value 0x80
#define U_SEQ_Value 0x00

#define U_CMD_IDX 11
#define U_DATA_IDX 12
#define U_FileData_IDX (U_CMD_IDX+6)
#define U_FileTimeOut_IDX (U_CMD_IDX+5)
#endif	
/****************************************************
命令 	写文件
命令字	0x22

******************************************************/
typedef enum
{
	U_CMD_LIST_TAG=0XF4,//列出标签
	U_CMD_LIST_READER=0XF5,
	U_CMD_AUTO_REPORT=0XF6,
	U_CMD_WRITE_FILE = 0XF0,
	U_CMD_READ_FILE = 0XF1,
	U_CMD_MSG_PUSH = 0X89,
	U_CMD_TIME_SET = 0X90,
	U_CMD_READER_ID = 0XF2    //读写器ID
}U_CMD_Typdef;
typedef enum
{
	U_FILETIME_ERR = 0X0604
}UFILE_State_Typedef;
//消息
typedef enum
{
	U_MSG_SUCESS=0X0000,
	U_MSG_ERR = 0X0700//超出最大长度
}UMSG_State_Typedef;

#define U_MSG_SUCCESS 0X0000
#define U_MSG_ACK_LEN 0X000B//消息命令返回长度

//时间
#define U_TIME_SUCCESS 0X0000
#define U_TIME_ERR 0X0800
#define U_TIME_ACK_LEN 0X000A//时间命令返回长度

//获取读写器ID

#define U_READER_ACK_LEN 						0X000C//读写器ID命令返回长度

#define U_CMD_DEVICE_TEST                       0XF3    //整机测试
#define U_CMD_DEVICE_TEST_LEN 					0X000A
typedef enum
{
	U_DEVICE_TEST_SUCCESS = 0X0000,
	U_DEVICE_TEST_ERR = 0X0901
}U_DEVICE_TEST_State_Typedef;

//列出标签或者读写器
typedef struct
{
	uint8_t LP_Filter_En;//低电过滤使能 1：使能，0：不使能
	uint8_t RSSI_Filter_En;//RSSI过滤使能 1：使能 0：不使能
	uint8_t RSSI_Filter_Value;//RSSI过滤值
}Filter_Typedef;
#define RADIO_RSSI_NO_Filter 127

#define U_LP_FILTEREN_Pos   0
#define U_LP_FILTEREN_Msk   0x01  //使能低电过滤
#define U_RSSI_FILTEREN_Pos 7
#define U_RSSI_FILTEREN_Msk 0x80  //使能RSSI过滤
#define U_RSSI_FILTERVALUE_Pos 0
#define U_RSSI_FILTERVALUE_Msk 0x7F  //RSSI过滤值
#define U_SEARCH_TIME_Pos     1
#define U_SEARCH_TIME_Msk     0x0E   //
#define U_LEAVE_TIME_Msk 0xf0 //离开时间
#define U_LEAVE_TIME_Pos 4

//超时
typedef struct
{
	uint8_t Radio_Time_En;//计数使能
	uint16_t Radio_Time_Cnt;//计数值
	uint16_t TimeOut_Cycle;//超时时间
	uint16_t Radio_SearchT_Cnt;
	uint8_t LeaveTime;//离开时间
}Time_Typedef;

typedef enum 
{
	PKT_HEAD1=0,//帧头1
	PKT_HEAD2,//帧头2
	PKT_PUSH_LEN,//帧长度
	PKT_DATA,//数据
	PKT_CRC
}state_typdef;

typedef struct
{
	uint8_t rx_state;//接收状态
	uint8_t has_finished;//接收完成
	uint8_t rx_idx;//接收索引号
	uint16_t rx_len;//数据长度
	uint8_t rx_buf[250];//接收缓冲区
	uint8_t tx_buf[250];//发送缓冲区
	uint16_t len;//长度
	uint8_t tx_en;//1：允许发送
}UART_Typedef;
		
void Uart_Deal(void);
void Stop_Update_Time(void);
#endif

