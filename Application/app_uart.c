#include "app_uart.h"
#include "app_radio.h"
#include "app_reader.h"
#include "crc16.h"
#include "app_msg.h"
#include "simple_uart.h"
#include "Debug_log.h"
#include "rtc.h"
#include "tim.h"

UART_Typedef U_Master;//定义串口缓冲
extern Payload_Typedef cmd_packet;//命令射频处理
const uint8_t UConfig_Self_ID[4] = {0xFF,0XFF,0XFF,0XFB};
extern uint8_t DeviceID[4];
extern uint8_t Work_Mode;//工作模式
extern MSG_Store_Typedef MSG_Store;//消息
rtc_typedef tmp_Time; //临时缓存时间
extern rtc_typedef Global_Time;//全局时间 

Time_Typedef Time_type;//超时处理

//过滤
Filter_Typedef Filter_Radio;
/********************************************************************
时间设置机制：平台每天下发一次时间设置，当时间下发成功后，该标志位置位，
当标签携带接收窗口，并且时间未更新过，则通过射频下发时间设置，一个小时过后，停止时间更新功能
*******************************************************************/
uint8_t Need_Time_Set;//1：允许设置时间0：不允许设置时间
uint8_t Hour;//一个小时过后，停止时间更新功能
/************************************************* 
@Description:命令处理
@Input:
@Output:无
@Return:无
*************************************************/  
u8 CRC16_Check(uint8_t* CRC16)
{
	uint16_t crc_src = (CRC16[0] << 8)| CRC16[1];
	uint16_t crc_len = (U_Master.rx_buf[U_LEN_IDX]<<8)|U_Master.rx_buf[U_LEN_IDX+1] + U_LENTH_LEN;
	uint16_t crc_data = crc16(&U_Master.rx_buf[U_LEN_IDX],crc_len);
	if(crc_src == crc_data)
		return 1;
	else
		return 0;
}


/************************************************* 
@Description: 停止更新时间
@Input:
@Output:无
@Return:无
*************************************************/  
void Stop_Update_Time(void)
{
	if(Hour!=Global_Time.hour)
	{
		Need_Time_Set = Time_NoUpdate;
	}
}

/************************************************* 
@Description:命令处理
@Input:
@Output:无
@Return:无
*************************************************/  
/****************************************************
串口通信 上位机->接收器
写命令F0
参数设置信息内容
目标ID：XXXXXXXX 4字节
超时时间:0~9  0：无超时时间  单位s
保留：0000
区选择：01~04
写最新参数:FFFF
写长度：01~10
数据内容：字节数，最大16字节
-----------------------------------------------------
读命令F1
目标ID：XXXXXXXX 4字节
超时时间:0~9  0：无超时时间  单位s
保留：0000
区选择：01~04
读最新参数:FFFF  参数区 保留区 0~15 用户区1、2 0~31
长度 01~10
******************************************************/
void Uart_Deal(void)
{
	uint8_t U_CMD;
	uint8_t U_DATA_LEN;
	uint16_t state;
	uint16_t crc;
	uint8_t crc_idx;
	if(U_Master.has_finished)
	{
		U_Master.has_finished = 0;
		if(CRC16_Check(&U_Master.rx_buf[U_Master.rx_idx-1]))
		{
			U_DATA_LEN = ((U_Master.rx_buf[U_LEN_IDX]<<8)|U_Master.rx_buf[U_LEN_IDX+1]) - U_AfterLEN_FIX_LEN;//信息长度
			U_CMD = U_Master.rx_buf[U_CMD_IDX];//命令处理
			switch(U_CMD)
			{
				case U_CMD_WRITE_FILE:
				{
					if(0 == ID_CMP(&UConfig_Self_ID[0],&U_Master.rx_buf[U_DATA_IDX]))//对自身进行配置
					{
						
					}
					else
					{
						my_memset(cmd_packet.packet,0,PACKET_PAYLOAD_MAXSIZE);
						cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0下行
						my_memcpy(&cmd_packet.packet[TAG_ID_IDX],&U_Master.rx_buf[U_DATA_IDX],RADIO_ID_LENGTH);//2~5目标ID
						my_memcpy(&cmd_packet.packet[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9读写器ID
						cmd_packet.packet[CMD_IDX] = FILE_CMD_WRITE;
						U_DATA_LEN = U_DATA_LEN - RADIO_ID_LENGTH - U_ConfigOT_LEN;//下发的命令长度
						my_memcpy(&cmd_packet.packet[CMD_IDX+1],&U_Master.rx_buf[U_FileData_IDX],U_DATA_LEN);//数据内容
						cmd_packet.length = CMD_FIX_LENGTH + U_DATA_LEN ;
						cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
						cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
						Work_Mode = File_Deal;
						Time_type.TimeOut_Cycle = U_Master.rx_buf[U_FileTimeOut_IDX];//超时时间
						if(Time_type.TimeOut_Cycle)
						{
							Time_type.TimeOut_Cycle = Time_type.TimeOut_Cycle*rtc_cont;
							Time_type.Radio_Time_En = 1;//开始计数
							Time_type.Radio_Time_Cnt = 0;//计数值清0
						}
					}
				}
				break;
				case U_CMD_READ_FILE:
				{
					if(0 == ID_CMP(&UConfig_Self_ID[0],&U_Master.rx_buf[U_DATA_IDX]))//对自身进行配置
					{
						
					}
					else
					{
						my_memset(cmd_packet.packet,0,PACKET_PAYLOAD_MAXSIZE);
						cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0下行
						my_memcpy(&cmd_packet.packet[TAG_ID_IDX],&U_Master.rx_buf[U_DATA_IDX],RADIO_ID_LENGTH);//2~5目标ID
						my_memcpy(&cmd_packet.packet[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9读写器ID
						cmd_packet.packet[CMD_IDX] = FILE_CMD_READ;
						U_DATA_LEN = U_DATA_LEN - RADIO_ID_LENGTH - U_ConfigOT_LEN;//下发的命令长度
						my_memcpy(&cmd_packet.packet[CMD_IDX+1],&U_Master.rx_buf[U_FileData_IDX],U_DATA_LEN);//数据内容
						cmd_packet.length = CMD_FIX_LENGTH + U_DATA_LEN ;
						cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
						cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
						Work_Mode = File_Deal;
						Time_type.TimeOut_Cycle = U_Master.rx_buf[U_FileTimeOut_IDX];//超时时间
						if(Time_type.TimeOut_Cycle)
						{
							Time_type.TimeOut_Cycle = Time_type.TimeOut_Cycle*rtc_cont;
							Time_type.Radio_Time_En = 1;//开始计数
							Time_type.Radio_Time_Cnt = 0;//计数值清0
						}
					}
				}
				break;
				case U_CMD_MSG_PUSH:
				{
					U_DATA_LEN = U_Master.rx_buf[U_DATA_IDX];//消息长度
					if(U_DATA_LEN>MSG_MAX_LEN)//消息长度超出最大长度
					{
						
						state = U_MSG_ERR;
					}
					else 
					{
						MSG_Store.MSG_Seq = (MSG_Store.MSG_Seq + 1)%MSG_SEQ_MAX_NUM;//序列号+1
						MSG_Store.MSG_BUFF[MSG_SEQ_IDX] = MSG_Store.MSG_Seq;//消息序号
						my_memcpy(&MSG_Store.MSG_BUFF[MSG_LEN_IDX],&U_Master.rx_buf[U_DATA_IDX],(U_Master.rx_buf[U_DATA_IDX]+1));//往flash中写入消息
						MSG_Write(MSG_Store.MSG_IDX,MSG_Store.MSG_BUFF);
						state = U_MSG_SUCCESS;
					}
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.len = U_MSG_ACK_LEN;
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//读写器ID
					U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//定位信息
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_MSG_PUSH;//命令
					U_Master.tx_buf[U_DATA_IDX] = state>>8;
					U_Master.tx_buf[U_DATA_IDX+1] = state;
					U_Master.tx_buf[U_DATA_IDX+2] = MSG_Store.MSG_Seq;//消息序号
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
//					debug_printf("\n\r成功更新消息");
				}
				break;
				case U_CMD_TIME_SET:
				{
					tmp_Time.year = U_Master.rx_buf[U_CMD_IDX+1];//年
					tmp_Time.month = U_Master.rx_buf[U_CMD_IDX+2];//月
					tmp_Time.day = U_Master.rx_buf[U_CMD_IDX+3];//日
					tmp_Time.hour = U_Master.rx_buf[U_CMD_IDX+4];//时
					tmp_Time.min = U_Master.rx_buf[U_CMD_IDX+5];//秒
					tmp_Time.sec = U_Master.rx_buf[U_CMD_IDX+6];//分
					if(TRUE == RTC_BCD_Check(&tmp_Time))
					{
						//BCD
						Global_Time.year = tmp_Time.year;
						Global_Time.month =tmp_Time.month;
						Global_Time.day = tmp_Time.day;
						Global_Time.hour = tmp_Time.hour;
						Global_Time.min = tmp_Time.min;
						Global_Time.sec = tmp_Time.sec;
						Hour = tmp_Time.hour;
						state = U_TIME_SUCCESS;
						Need_Time_Set = Time_Update;//允许设置时间
					}
					else
					{
						state = U_TIME_ERR;
					}
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.len = U_TIME_ACK_LEN;
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//读写器ID
					U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//定位信息
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_TIME_SET;//命令
					U_Master.tx_buf[U_DATA_IDX] = state>>8;
					U_Master.tx_buf[U_DATA_IDX+1] = state;
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
//					debug_printf("\n\r更新时间");
				}
				break;
				case U_CMD_READER_ID:
				{
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.len = U_READER_ACK_LEN;
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//协议
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//读写器ID
					U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//定位信息
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//流水号
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_READER_ID;//命令
					my_memcpy(&U_Master.tx_buf[U_DATA_IDX],DeviceID,RADIO_ID_LENGTH);//读写器ID
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//长度~信息内容
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //帧头长度+长度长度+长度后面的长度
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//帧头2+长度2+校验2+数据长度
					UART_Send(U_Master.tx_buf,U_Master.len);
				}
				break;
				case U_CMD_LIST_TAG:
				{
					Work_Mode = List_Tag;//列出标签
					Filter_Radio.LP_Filter_En = ((U_Master.rx_buf[U_DATA_IDX]&U_LP_FILTEREN_Msk)>>U_LP_FILTEREN_Pos);
					Filter_Radio.RSSI_Filter_En = ((U_Master.rx_buf[U_DATA_IDX+1]&U_RSSI_FILTEREN_Msk)>>U_RSSI_FILTEREN_Pos);
					if(1 == Filter_Radio.RSSI_Filter_En)
					{
						Filter_Radio.RSSI_Filter_Value = ((U_Master.rx_buf[U_DATA_IDX+1]&U_RSSI_FILTERVALUE_Msk)>>U_RSSI_FILTERVALUE_Pos);
					}
					else
					{
						Filter_Radio.RSSI_Filter_Value = RADIO_RSSI_NO_Filter;//最大值
					}
					Time_type.TimeOut_Cycle = ((U_Master.rx_buf[U_DATA_IDX]&U_SEARCH_TIME_Msk)>>U_SEARCH_TIME_Pos);//查询时间
					if(Time_type.TimeOut_Cycle)
					{
						TID_RECORD_Clear();//清空缓存
						Time_type.TimeOut_Cycle = Time_type.TimeOut_Cycle*rtc_cont;
						Time_type.Radio_Time_En = 1;//开始计数
						Time_type.Radio_Time_Cnt = 0;//计数值清0
					}
					
				}
				break;
				case U_CMD_LIST_READER:
				{
					Work_Mode = List_Reader;//列出读写器
					Filter_Radio.LP_Filter_En = ((U_Master.rx_buf[U_DATA_IDX]&U_LP_FILTEREN_Msk)>>U_LP_FILTEREN_Pos);
					Filter_Radio.RSSI_Filter_En = ((U_Master.rx_buf[U_DATA_IDX+1]&U_RSSI_FILTEREN_Msk)>>U_RSSI_FILTEREN_Pos);
					if(1 == Filter_Radio.RSSI_Filter_En)
					{
						Filter_Radio.RSSI_Filter_Value = ((U_Master.rx_buf[U_DATA_IDX+1]&U_RSSI_FILTERVALUE_Msk)>>U_RSSI_FILTERVALUE_Pos);
					}
					else
					{
						Filter_Radio.RSSI_Filter_Value = RADIO_RSSI_NO_Filter;//最大值
					}
					Time_type.TimeOut_Cycle = ((U_Master.rx_buf[U_DATA_IDX]&U_SEARCH_TIME_Msk)>>U_SEARCH_TIME_Pos);//查询时间
					if(Time_type.TimeOut_Cycle)
					{
						TID_RECORD_Clear();//清空缓存
						Time_type.TimeOut_Cycle = Time_type.TimeOut_Cycle*rtc_cont;
						Time_type.Radio_Time_En = 1;//开始计数
						Time_type.Radio_Time_Cnt = 0;//计数值清0
					}
				}
				break;
				case U_CMD_AUTO_REPORT://自动上报
				{
					
					if(U_Master.rx_buf[U_DATA_IDX+2] == 0x01)
					{
						Work_Mode = Auto_Reoprt;//列出读写器
						Filter_Radio.LP_Filter_En = ((U_Master.rx_buf[U_DATA_IDX]&U_LP_FILTEREN_Msk)>>U_LP_FILTEREN_Pos);
						Filter_Radio.RSSI_Filter_En = ((U_Master.rx_buf[U_DATA_IDX+1]&U_RSSI_FILTEREN_Msk)>>U_RSSI_FILTEREN_Pos);
						if(1 == Filter_Radio.RSSI_Filter_En)
						{
							Filter_Radio.RSSI_Filter_Value = ((U_Master.rx_buf[U_DATA_IDX+1]&U_RSSI_FILTERVALUE_Msk)>>U_RSSI_FILTERVALUE_Pos);
						}
						else
						{
							Filter_Radio.RSSI_Filter_Value = RADIO_RSSI_NO_Filter;//最大值
						}
						Time_type.TimeOut_Cycle = ((U_Master.rx_buf[U_DATA_IDX]&U_SEARCH_TIME_Msk)>>U_SEARCH_TIME_Pos);//查询时间
						if(Time_type.TimeOut_Cycle)
						{
							TID_RECORD_Clear();//清空缓存
							Time_type.TimeOut_Cycle = Time_type.TimeOut_Cycle*rtc_cont;
							Time_type.Radio_Time_En = 1;//开始计数
							Time_type.Radio_Time_Cnt = 0;//计数值清0
						}
						Time_type.LeaveTime =(U_Master.rx_buf[U_DATA_IDX]&U_LEAVE_TIME_Msk)>>U_LEAVE_TIME_Pos;//秒
					}
					else
					{
						Time_type.Radio_Time_Cnt = 0;
						Time_type.Radio_Time_En = 0;
						Work_Mode = Idle;
					}
					
				}
				break;
			}
		}
	}
}

/************************************************* 
@Description:记录串口数据
@Input:
@Output:无
@Return:无
*************************************************/  
void Uart_ReceiveBuff(uint8_t rx_temp)
{
	switch(U_Master.rx_state)
	{
		case PKT_HEAD1://帧头1
			if(pkt_head1 == rx_temp)	                                
			{                                                   
				U_Master.rx_idx = 0;//索引号为0
				U_Master.rx_buf[U_Master.rx_idx] = rx_temp;//缓存数据
				U_Master.rx_state = PKT_HEAD2;//状态切换
			}
			break;
		case PKT_HEAD2:
			if(pkt_head2 ==  rx_temp)
			{
				U_Master.rx_idx++;		
				U_Master.rx_buf[U_Master.rx_idx] = rx_temp;
				U_Master.rx_state = PKT_PUSH_LEN;
			}
			else
			{
				U_Master.rx_state = PKT_HEAD1;
			}
			break;
		case PKT_PUSH_LEN:
			U_Master.rx_idx++;
			U_Master.rx_buf[U_Master.rx_idx] = rx_temp;
			if(U_Master.rx_idx == len_finish)//长度接收完成
			{
				U_Master.rx_len = (( U_Master.rx_buf[U_Master.rx_idx-1] << 8 ) | U_Master.rx_buf[U_Master.rx_idx]);
				if(U_Master.rx_len>250)
				{
					U_Master.rx_state = PKT_HEAD1;
				}
				else
				{
					U_Master.rx_state = PKT_DATA;
				}
			}
			break;
		case PKT_DATA://缓存数据
			if(U_Master.rx_len > 0)
			{
				U_Master.rx_idx++;
				U_Master.rx_buf[U_Master.rx_idx] = rx_temp;	
				U_Master.rx_len--;
			}
			if(0 == U_Master.rx_len )
			{
				U_Master.rx_state = PKT_CRC;
			}
			break;
		case PKT_CRC:
			U_Master.rx_idx++;
			U_Master.rx_buf[U_Master.rx_idx] = rx_temp;	
			U_Master.rx_len++;
			if(crc_bytes == U_Master.rx_len)
			{
				U_Master.has_finished = 1;
				U_Master.rx_state = PKT_HEAD1;
			}
			break;
		default : U_Master.rx_state = PKT_HEAD1;
	}
}
/************************************************* 
@Description:串口中断
@Input:
@Output:无
@Return:无
*************************************************/  
void UART0_IRQHandler()
{
	uint8_t temp;
	if(NRF_UART0->EVENTS_RXDRDY)
	{
		NRF_UART0->EVENTS_RXDRDY = 0;
		temp = NRF_UART0->RXD;
		Uart_ReceiveBuff(temp);
	}
	else if(NRF_UART0->EVENTS_ERROR)
	{
		NRF_UART0->EVENTS_ERROR = 0;
	}
}

