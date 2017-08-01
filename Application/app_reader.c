/*******************************************************************************
** 版权:		
** 文件名: 		app_reader.c
** 版本：  		1.0
** 工作环境: 	MDK-ARM 5.23
** 作者: 		cc
** 生成日期: 	2017-07-26
** 功能:		  
** 相关文件:	app_key.h
** 修改日志：	
** 版权所有   
*******************************************************************************/
#include "app_reader.h"
#include "sys.h"
#include "app_var.h"
#include "app_uart.h"
#include "simple_uart.h"
#include "crc16.h"
#include "app_radio.h"
#include "rtc.h"
extern Payload_Typedef cmd_packet;//命令射频处理
extern uint8_t radio_run_channel;//射频运行通道 
extern TID_Typedef  TID_RECORD[CAPACITY];
uint8_t Work_Mode = Idle;//工作模式
uint8_t tx_buf[254];//串口发送
extern UART_Typedef U_Master;//定义串口缓冲
extern uint8_t Hour;//一个小时过后，停止时间更新功能
extern rtc_typedef Global_Time;
//设备ID
extern uint8_t DeviceID[4];
void list_tag(void)
{
	
}
/************************************************* 
@Description:根据串口命令，工作在不同的模式下
@Input:无
@Output:无
@Return:无
*************************************************/ 
void app_process(void)
{
	uint8_t info_len;
	uint16_t crc;
	uint16_t crc_idx;
	switch(Work_Mode)
	{
		case Idle:
			if(RADIO_RUN_DATA_CHANNEL!= radio_run_channel)
			{
				radio_select(DATA_CHANNEL,RADIO_RX);
			}
			Stop_Update_Time();
			break;
		case List_Tag:
			
			break;
		case List_Reader:
			break;
		case File_Deal:
			if(U_Master.tx_en)
			{
				U_Master.tx_en = 0;
				U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
				U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
				info_len = (uint8_t)(cmd_packet.packet[RADIO_LENGTH_IDX] - CMD_FIX_LENGTH);//信息长度
				U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
				U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
				U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
				U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
				my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//读写器ID
				U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//定位信息
				U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
				if(cmd_packet.packet[CMD_IDX] ==FILE_CMD_READ)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_READ_FILE;
				else if(cmd_packet.packet[CMD_IDX] ==FILE_CMD_WRITE)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_WRITE_FILE;
				my_memcpy(&U_Master.tx_buf[U_DATA_IDX],&cmd_packet.packet[EXCUTE_STATE_IDX],info_len);//读写器ID
				U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
				crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
				crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
				U_Master.tx_buf[crc_idx] = (crc>>8);
				U_Master.tx_buf[crc_idx+1] = crc;//crc
				U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
				UART_Send(U_Master.tx_buf,U_Master.len);
				Work_Mode = Idle;
			}
			break;
		case Tag_Report:
			break;
	}
}
