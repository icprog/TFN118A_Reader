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
extern volatile Radio_State_Typedef Radio_State;//射频工作状态
extern TID_Typedef  TID_RECORD[CAPACITY];
uint8_t Work_Mode = Idle;//工作模式
uint8_t tx_buf[254];//串口发送
extern UART_Typedef U_Master;//定义串口缓冲
extern uint8_t Hour;//一个小时过后，停止时间更新功能
extern rtc_typedef Global_Time;
//设备ID
extern uint8_t DeviceID[4];
extern Time_Typedef Time_type;//超时处理
extern uint8_t sec_flag;//秒计数
//过滤
extern Filter_Typedef Filter_Radio;

typedef struct
{
	uint8_t max_pkt;//最大分包编号
	uint8_t crt_pkt;//当前分包编号
}U_PKT_Typdef;
U_PKT_Typdef U_TAG_PKT;
#define MAX_TAG_NUM  14  //每包最大标签个数
#define TAG_INFO_LEN 16  //标签长度
//#define MAX_PKT_Pos    4
//#define CRT_PKT_Msk    0x0f
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
	uint8_t info_len=0;//信息内容长度
	uint16_t crc=0;
	uint16_t crc_idx=0;
	uint16_t rfid_j=0;
	uint8_t *ptx_buf;
	uint16_t rfid_i=0;//有效rfid个数
	switch(Work_Mode)
	{
		case Idle:
			if(RADIO_RUN_DATA_CHANNEL!= Radio_State.radio_run_channel)
			{
				radio_select(DATA_CHANNEL,RADIO_RX);
			}
			Stop_Update_Time();
			break;
		case List_Tag:
			if(Time_type.Radio_Time_Cnt > Time_type.TimeOut_Cycle)//查询时间结束
			{
				Time_type.Radio_Time_Cnt = 0;
				Time_type.Radio_Time_En = 0;
				#if 0
				for(rfid_j=0;rfid_j<100;rfid_j++)//
				{
					TID_RECORD[rfid_j].TID[0] = rfid_j;
					TID_RECORD[rfid_j].Sensor_Type = 0X03;
					TID_RECORD[rfid_j].RSSI = 2;
					TID_RECORD[rfid_j].VER = 1;
					TID_RECORD[rfid_j].LeaveTime=1;
					my_memset(TID_RECORD[rfid_j].Sensor_Data,1,Sensor_Data_Length);
				}
				#endif
				//获取有效标签总数量，算出分包个数
				for(rfid_j=0;rfid_j<CAPACITY;rfid_j++)//
				{
					if(TID_RECORD[rfid_j].TID[0]!=0xff)
					{
						rfid_i++;//标签个数
					}	
				}
				U_TAG_PKT.max_pkt = rfid_i/MAX_TAG_NUM;
				if(rfid_i%MAX_TAG_NUM)
				{
					U_TAG_PKT.max_pkt++;
				}
				if(U_TAG_PKT.max_pkt)
				{
					U_TAG_PKT.max_pkt--;//最大分包编号0~15
				}
				rfid_j = 0;
				for(U_TAG_PKT.crt_pkt=0;U_TAG_PKT.crt_pkt<=U_TAG_PKT.max_pkt;U_TAG_PKT.crt_pkt++)
				{
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID

					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_LIST_TAG;//命令字
					U_Master.tx_buf[U_DATA_IDX]=U_TAG_PKT.max_pkt;//最大分包编号
					U_Master.tx_buf[U_DATA_IDX+1]=U_TAG_PKT.crt_pkt;//当前分包编号
					U_Master.tx_buf[U_DATA_IDX+2]=0X00;//设备数量
					ptx_buf = &U_Master.tx_buf[U_DATA_IDX+3];
					info_len=0;//信息内容长度清0
					for(;rfid_j<CAPACITY;)//
					{
						if(TID_RECORD[rfid_j].TID[0]!=0xff)
						{
							*ptx_buf++ = TID_RECORD[rfid_j].TID[0];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[1];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[2];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[3];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[4];
							*ptx_buf++ = TID_RECORD[rfid_j].State;
							*ptx_buf++ = TID_RECORD[rfid_j].VER;
							*ptx_buf++ = TID_RECORD[rfid_j].RSSI;
							*ptx_buf++ = TID_RECORD[rfid_j].Sensor_Type;
							for(uint8_t i=0;i<Sensor_Data_Length;i++)
							{
								*ptx_buf++ = TID_RECORD[rfid_j].Sensor_Data[i];
							}
							info_len++;
						}
						rfid_j++;
						if(info_len >= MAX_TAG_NUM)
							break;
					}
					U_Master.tx_buf[U_DATA_IDX+2] = info_len;//设备数量
					info_len = (info_len<<4) + 3;//info_len*TAG_INFO_LEN  标签信息长度+分包标记+设备数量
					U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
				}
				Work_Mode = Idle;
			}
			break;
		case List_Reader:
			if(Time_type.Radio_Time_Cnt > Time_type.TimeOut_Cycle)//查询时间结束
			{
				Time_type.Radio_Time_Cnt = 0;
				Time_type.Radio_Time_En = 0;
				#if 0
				for(rfid_j=0;rfid_j<100;rfid_j++)//
				{
					TID_RECORD[rfid_j].TID[3] = rfid_j;
					TID_RECORD[rfid_j].Sensor_Type = 0X01;
					TID_RECORD[rfid_j].RSSI = rfid_j;
				}
				#endif
				//获取有效标签总数量，算出分包个数
				for(rfid_j=0;rfid_j<CAPACITY;rfid_j++)//
				{
					if(TID_RECORD[rfid_j].TID[3]!=0xff)
					{
						rfid_i++;//标签个数
					}	
				}
				U_TAG_PKT.max_pkt = rfid_i/MAX_TAG_NUM;
				if(rfid_i%MAX_TAG_NUM)
				{
					U_TAG_PKT.max_pkt++;
				}
				if(U_TAG_PKT.max_pkt)
				{
					U_TAG_PKT.max_pkt--;//最大分包编号0~15
				}
				rfid_j = 0;
				for(U_TAG_PKT.crt_pkt=0;U_TAG_PKT.crt_pkt<=U_TAG_PKT.max_pkt;U_TAG_PKT.crt_pkt++)
				{
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID

					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_LIST_TAG;//命令字
					U_Master.tx_buf[U_DATA_IDX]=U_TAG_PKT.max_pkt;//最大分包编号
					U_Master.tx_buf[U_DATA_IDX+1]=U_TAG_PKT.crt_pkt;//当前分包编号
					U_Master.tx_buf[U_DATA_IDX+2]=0X00;//设备数量
					ptx_buf = &U_Master.tx_buf[U_DATA_IDX+3];
					info_len=0;//信息内容长度清0
					for(;rfid_j<CAPACITY;)//
					{
						if(TID_RECORD[rfid_j].TID[0]!=0xff)
						{
							*ptx_buf++ = TID_RECORD[rfid_j].TID[0];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[1];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[2];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[3];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[4];
							*ptx_buf++ = TID_RECORD[rfid_j].State;
							*ptx_buf++ = TID_RECORD[rfid_j].VER;
							*ptx_buf++ = TID_RECORD[rfid_j].RSSI;
							*ptx_buf++ = TID_RECORD[rfid_j].Sensor_Type;
							my_memcpy(ptx_buf,&TID_RECORD[rfid_j].Sensor_Data,Sensor_Data_Length);
							info_len++;
						}
						rfid_j++;
						if(info_len >= MAX_TAG_NUM)
							break;
					}
					U_Master.tx_buf[U_DATA_IDX+2] = info_len;//设备数量
					info_len = (info_len<<4) + 3;//info_len*TAG_INFO_LEN  标签信息长度+分包标记+设备数量
					U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
				}
				Work_Mode = Idle;
			}			
			break;
		case Auto_Reoprt:
		{
			if(sec_flag)
			{
				sec_flag = 0;
				for(rfid_j=0;rfid_j<CAPACITY;rfid_j++)
				{
					if(TID_RECORD[rfid_j].TID[0]!=0xff)
					{
						TID_RECORD[rfid_j].LeaveTime++;
						if(TID_RECORD[rfid_j].LeaveTime > Time_type.LeaveTime)
						{
							TID_RECORD[rfid_j].TID[0] = 0xff;
							TID_RECORD[rfid_j].TID[1] = 0xff;
							TID_RECORD[rfid_j].TID[2] = 0xff;
							TID_RECORD[rfid_j].TID[3] = 0xff;
						}
					}
				}
			}
			if(Time_type.Radio_Time_Cnt > Time_type.TimeOut_Cycle)//
			{
				Time_type.Radio_Time_Cnt = 0;
				#if 0
				for(rfid_j=0;rfid_j<100;rfid_j++)//
				{
					TID_RECORD[rfid_j].TID[0] = rfid_j;
					TID_RECORD[rfid_j].Sensor_Type = 0X01;
					TID_RECORD[rfid_j].RSSI = rfid_j;
				}
				#endif
				//获取有效标签总数量，算出分包个数
				for(rfid_j=0;rfid_j<CAPACITY;rfid_j++)//
				{
					if(TID_RECORD[rfid_j].TID[0]!=0xff)
					{
						rfid_i++;//标签个数
					}	
				}
				U_TAG_PKT.max_pkt = rfid_i/MAX_TAG_NUM;
				if(rfid_i%MAX_TAG_NUM)
				{
					U_TAG_PKT.max_pkt++;
				}
				if(U_TAG_PKT.max_pkt)
				{
					U_TAG_PKT.max_pkt--;//最大分包编号0~15
				}
				rfid_j = 0;
				for(U_TAG_PKT.crt_pkt=0;U_TAG_PKT.crt_pkt<=U_TAG_PKT.max_pkt;U_TAG_PKT.crt_pkt++)
				{
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID

					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_LIST_TAG;//命令字
					U_Master.tx_buf[U_DATA_IDX]=U_TAG_PKT.max_pkt;//最大分包编号
					U_Master.tx_buf[U_DATA_IDX+1]=U_TAG_PKT.crt_pkt;//当前分包编号
					U_Master.tx_buf[U_DATA_IDX+2]=0X00;//设备数量
					ptx_buf = &U_Master.tx_buf[U_DATA_IDX+3];
					info_len=0;//信息内容长度清0
					for(;rfid_j<CAPACITY;)//
					{
						if(TID_RECORD[rfid_j].TID[0]!=0xff)
						{
							*ptx_buf++ = TID_RECORD[rfid_j].TID[0];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[1];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[2];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[3];
							*ptx_buf++ = TID_RECORD[rfid_j].TID[4];
							*ptx_buf++ = TID_RECORD[rfid_j].State;
							*ptx_buf++ = TID_RECORD[rfid_j].VER;
							*ptx_buf++ = TID_RECORD[rfid_j].RSSI;
							*ptx_buf++ = TID_RECORD[rfid_j].Sensor_Type;
							my_memcpy(ptx_buf,&TID_RECORD[rfid_j].Sensor_Data,Sensor_Data_Length);
							info_len++;
						}
						rfid_j++;
						if(info_len >= MAX_TAG_NUM)
							break;
					}
					U_Master.tx_buf[U_DATA_IDX+2] = info_len;//设备数量
					info_len = (info_len<<4) + 3;//info_len*TAG_INFO_LEN  标签信息长度+分包标记+设备数量
					U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
				}
			}
		}
		break;
		case File_Deal:
			if(Time_type.Radio_Time_Cnt > Time_type.TimeOut_Cycle)//超时
			{
				Time_type.Radio_Time_Cnt = 0;
				Time_type.Radio_Time_En = 0;
				U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
				U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
				info_len = U_STATE_LEN;//信息长度
				U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
				U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
				U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
				U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
				my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID

				U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
				if(U_Master.rx_buf[U_CMD_IDX] ==U_CMD_READ_FILE)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_READ_FILE;
				else if(U_Master.rx_buf[U_CMD_IDX] == U_CMD_WRITE_FILE)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_WRITE_FILE;//命令字
				else if(U_Master.rx_buf[U_CMD_IDX] == U_CMD_ERASE_FILE)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_ERASE_FILE;//命令字
				U_Master.tx_buf[U_DATA_IDX] = (U_FILETIME_ERR>>8);//信息内容-状态
				U_Master.tx_buf[U_DATA_IDX+1] = (uint8_t)U_FILETIME_ERR;//信息内容-状态
				U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
				crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
				crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
				U_Master.tx_buf[crc_idx] = (crc>>8);
				U_Master.tx_buf[crc_idx+1] = crc;//crc
				U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
				UART_Send(U_Master.tx_buf,U_Master.len);
				Work_Mode = Idle;				
			}
			else
			{
				if(U_Master.tx_en)
				{
					U_Master.tx_en = 0;
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					info_len = (uint8_t)(cmd_packet.packet[RADIO_LENGTH_IDX] - CMD_ONEFIX_LENGTH);//信息长度
					U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					if(cmd_packet.packet[CMD_IDX] ==FILE_READ_CMD)
						U_Master.tx_buf[U_CMD_IDX] = U_CMD_READ_FILE;
					else if(cmd_packet.packet[CMD_IDX] ==FILE_WRITE_CMD)
						U_Master.tx_buf[U_CMD_IDX] = U_CMD_WRITE_FILE;
					else if(U_Master.rx_buf[U_CMD_IDX] == U_CMD_ERASE_FILE)
						U_Master.tx_buf[U_CMD_IDX] = U_CMD_ERASE_FILE;//命令字
					my_memcpy(&U_Master.tx_buf[U_DATA_IDX],&cmd_packet.packet[EXCUTE_STATE_IDX],info_len);//信息内容
					U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
					Work_Mode = Idle;
				}
			}
			break;
		case Device_Test:
		{
			if(Time_type.Radio_Time_Cnt > Time_type.TimeOut_Cycle)//超时
			{
				Time_type.Radio_Time_Cnt = 0;
				Time_type.Radio_Time_En = 0;
				U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
				U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
				info_len = U_STATE_LEN;//信息长度
				U_Master.len = info_len + U_AfterLEN_FIX_LEN;//信息内容+串口协议长度后面的固定长度
				U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
				U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
				U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
				my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID
				U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
				U_Master.tx_buf[U_CMD_IDX] = U_Master.rx_buf[U_CMD_IDX];//命令字
				U_Master.tx_buf[U_DATA_IDX] = (U_DEVICE_TEST_ERR>>8);//信息内容-状态
				U_Master.tx_buf[U_DATA_IDX+1] = (uint8_t)U_DEVICE_TEST_ERR;//信息内容-状态
				U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//串口协议中数据长度，协议版本~信息内容
				crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
				crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
				U_Master.tx_buf[crc_idx] = (crc>>8);
				U_Master.tx_buf[crc_idx+1] = crc;//crc
				U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
				UART_Send(U_Master.tx_buf,U_Master.len);
				Work_Mode = Idle;	
			}
			else
			{
				if(U_Master.tx_en)
				{
					U_Master.tx_en = 0;
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.len = U_CMD_DEVICE_TEST_LEN;
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_RID_LENGTH);//读写器ID
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_DEVICE_TEST;//命令
					U_Master.tx_buf[U_DATA_IDX] = (U_DEVICE_TEST_SUCCESS>>8);//信息内容-状态
					U_Master.tx_buf[U_DATA_IDX+1] = (uint8_t)U_DEVICE_TEST_SUCCESS;//信息内容-状态					
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
					Work_Mode = Idle;
				}
			}
		}
		case Tag_Report:
			break;
	}
}
