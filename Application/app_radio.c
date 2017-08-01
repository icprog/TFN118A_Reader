#include "app_radio.h"
#include "string.h" 
#include "app_reader.h"
#include "app_uart.h"
#include "app_msg.h"
#include "Debug_log.h"
#include "rtc.h"
extern uint8_t Work_Mode;//工作模式
//需要修改的参数
#define win_interval 1//标签开接收窗口间隔
//携带命令
typedef enum
{
	WithoutCmd = 0,
	WithCmd
}CMD_Typedef;
//携带接收窗
typedef enum
{
	WithoutWin = 0,
	WithWin
}WIN_Typedef;
//是否等待发送完成
typedef enum
{
	SendNoWait=0,
	SendWait=1
}Send_Wait_Typedef;


extern uint8_t packet[PACKET_PAYLOAD_MAXSIZE];
extern uint8_t DeviceID[4];
extern uint8_t para_record[PARA_RECORD_LEN];
//读写器参数
extern Para_Typedef Reader_Para;
//射频接收发送完成
uint8_t radio_rcvok = 0;//设置自身参数
uint8_t radio_sndok = 0;
//射频缓冲区
Payload_Typedef cmd_packet;//命令射频处理
extern uint8_t radio_status;//射频运行状态
uint8_t radio_run_channel;//射频运行通道 
//标签状态字
uint8_t State_LP_Alarm;//低电报警
uint8_t State_Key_Alram;//按键报警
const static uint8_t State_WithSensor = 1;//1:传感标签 0：非传感标签
//uint8_t State_WithWin;
uint8_t State_Mode;//模式
//传感数据
uint8_t M_Seq;//消息序列号0~7
uint8_t MSG_PUSH_State = MSG_IDLE;//消息推送状态
//消息处理
extern Message_Typedef Msg_Packet;
#define RADIO_RX_OT 	825

void radio_pwr(uint8_t txpower);
static void Radio_Period_Send(uint8_t cmdflag,uint8_t winflag,uint8_t wait_send_finish);

//文件操作
File_Typedef f_para;//文件操作命令数据缓存
//串口缓冲
extern UART_Typedef U_Master;

//读写器-记录标签
TID_Typedef  TID_RECORD[CAPACITY];
void Radio_Cmd_Deal(void);//自身参数设置
//时间设置
extern uint8_t Need_Time_Set;//1：允许设置时间0：不允许设置时间
/*
@Description:清空缓冲记录
@Input:state :
@Output:无
@Return:无
*/
void TID_RECORD_Clear(void)
{
	uint8_t i;
	for(i=0;i<CAPACITY;i++)
	{
		memset(&TID_RECORD[i],0xff,sizeof(TID_Typedef));
	}
	
}
/*
@Description:射频启动
@Input:state :
@Output:无
@Return:无
*/
static void radio_on(void)
{
	xosc_hfclk_start();//外部晶振起振
}
/*
Description:射频关闭
Input:state :
Output:无
Return:无
*/
static void radio_off(void)
{
	radio_disable();
	xosc_hfclk_stop();
}

/*
Description:射频选择
Input:state :
Output:无
Return:无
*/
void radio_select(uint8_t ch,uint8_t dir)
{
	uint8_t channel;
	radio_on();//开启晶振
	channel = (ch == DATA_CHANNEL)?RADIO_CHANNEL_DATA:RADIO_CHANNEL_CONFIG;
	radio_run_channel = (ch == DATA_CHANNEL)?RADIO_RUN_DATA_CHANNEL:RADIO_RUN_CONFIG_CHANNEL;
	NRF_RADIO->DATAWHITEIV = channel;//数据白化
	if(dir == RADIO_TX)
	{
		radio_tx_carrier(RADIO_MODE_MODE_Nrf_1Mbit,channel);
		
	}
	else if(dir == RADIO_RX)
	{
		radio_rx_carrier(RADIO_MODE_MODE_Nrf_1Mbit,channel);
	}
}

/*
@Description:射频周期发送
@Input:state :
@Output:无
@Return:无
*/
void Raio_Deal(void)
{
	uint32_t ot;
	//平时处于接收状态，偶尔插入一次发送
	if(Reader_Para.radio_send_en)
	{
		Reader_Para.radio_send_en = 0;
		Radio_Period_Send(WithoutCmd,WithoutWin,SendWait);//等待发送完成
		//后续考虑是否允许通过配置频道更改射频参数
		ot = RADIO_RX_OT;//接收窗时间
		while(--ot)
		{
			if(radio_rcvok)
				break;
				
		}	
		if(radio_rcvok)//接收成功，设置自身参数
		{
			radio_rcvok = 0;
			Radio_Cmd_Deal();//命令处理
		}		
		radio_select(DATA_CHANNEL,RADIO_RX);
	}

}

/***********************************************************
@Description:周期发送射频信息,并等待发送完成
@Input：	cmdflag - 命令返回 1：返回命令 0：常规发送
				winflag - 是否携带窗口
				wait_send_finish:1:等待发送完成，0:不等待
@Output：无
@Return:无
************************************************************/
static void Radio_Period_Send(uint8_t cmdflag,uint8_t winflag,uint8_t wait_send_finish)
{
	uint32_t ot = 0;

	if(cmdflag)
	{
		memcpy(packet,cmd_packet.packet,cmd_packet.length+RADIO_HEAD_LENGTH);
		radio_select(CONFIG_CHANNEL,RADIO_TX);//配置频道
//		while()
	}
	else
	{
		my_memset(packet,0,PACKET_PAYLOAD_MAXSIZE);
		packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;
		packet[RADIO_LENGTH_IDX] = 0; //payload长度，后续更新
		my_memcpy(packet+TAG_ID_IDX,DeviceID,4);//2~5标签ID
		packet[TAG_STATE_IDX] |= State_LP_Alarm << TAG_LOWPWR_Pos;//低电指示
		packet[TAG_STATE_IDX] |= State_Key_Alram << TAG_KEY_Pos;//按键指示
		packet[TAG_STATE_IDX] |= State_WithSensor << TAG_WITHSENSOR_Pos;//传感指示
		packet[TAG_STATE_IDX] |= winflag << TAG_WITHWIN_Pos;//接收窗指示
		packet[TAG_STATE_IDX] |= State_Mode << TAG_MODE_Pos;//模式指示
		packet[TAG_VERSION_IDX] |= TAG_HDVER_NUM << TAG_HDVERSION_POS;//硬件版本号
		packet[TAG_VERSION_IDX] |= TAG_SFVER_NUM << TAG_SFVERSION_POS;//软件版本号
		packet[TAG_STYPE_IDX] = TAG_SENSORTYPE_SchoolWatch;//标签类型
		packet[TAG_SDATA_IDX] |= M_Seq << TAG_MSEQ_Pos;//传感数据
		cmd_packet.length = TAG_SDATA_IDX;//PAYLOAD长度 
		packet[RADIO_LENGTH_IDX] = cmd_packet.length ;
		packet[TAG_SDATA_IDX+1] = Get_Xor(packet,cmd_packet.length+1);//S0+max_length+PAYLOAD
		
	}
	if(cmdflag)
	{
		radio_select(CONFIG_CHANNEL,RADIO_TX);
	}
	else
	{
		radio_select(DATA_CHANNEL,RADIO_TX);
	}
	//是否等待发送完成
	if(1 == wait_send_finish)
	{
		radio_sndok = 0;
		while(radio_sndok == 0)
		{
			ot++;
			if(ot > RADIO_OVER_TIME)
				break;
		}		
	}

}

/************************************************* 
Description:射频功率选择
Input:输入发射功率
Output:无
Return:无
*************************************************/  
void radio_pwr(uint8_t txpower)
{
	switch(txpower)
	{
		case P_PWR_N30DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg30dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_N20DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg20dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_N16DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg16dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_N12DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg12dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_N8DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg8dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_N4DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg4dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_P0DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case P_PWR_P4DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		default:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
	}
}

/*
Description:异或检查
Input:src:原数组，长度
Output:
Return:无
*/
uint8_t Xor_Check(uint8_t *src,uint8_t size)
{
	uint8_t sum=0;
	uint8_t i;
	
	for(i = 0;i<size;i++)
	{
		sum ^= src[i];
	}
	if(sum == 0)
		return TRUE;
	else
		return FALSE;
}

/*
Description:ID对比
Input:src:
Output:
Return:无
*/
uint8_t ID_CMP(const uint8_t *src,const uint8_t *dest)
{
	if(src[0]!=dest[0])
		return 1;
	if(src[1]!=dest[1])
		return 1;
	if(src[2]!=dest[2])
		return 1;
	if(src[3]!=dest[3])
		return 1;
	return 0;
}
/*
Description:标签记录
Input:src:
Output:
Return:无
*/
void tag_record(uint8_t *t_packet)
{
	uint8_t i,j;
	for(i=0,j=CAPACITY;i<CAPACITY;i++)
	{
		if(TID_RECORD[i].TID[0]==READER_ID_MBYTE)
		{
			if(j > i) j=i;//记录下第一个出现0xff的索引号
		}
		else if(0 == ID_CMP(&t_packet[TAG_ID_IDX],&TID_RECORD[i].TID[0]))
		{
			j=i;
			break;
		}
	}
	if(j < CAPACITY)//允许记录
	{
		if(i == CAPACITY)//新纪录
		{
			TID_RECORD[j].TID[0] = t_packet[TAG_ID_IDX];
			TID_RECORD[j].TID[1] = t_packet[TAG_ID_IDX + 1];
			TID_RECORD[j].TID[2] = t_packet[TAG_ID_IDX + 2];
			TID_RECORD[j].TID[3] = t_packet[TAG_ID_IDX + 3];
			TID_RECORD[j].State =  t_packet[TAG_STATE_IDX];
			TID_RECORD[j].Sensor_Type = t_packet[TAG_STYPE_IDX];
			TID_RECORD[j].Sensor_Data[0] = t_packet[TAG_SDATA_IDX];
		}
		else//更新数据
		{
			TID_RECORD[j].State =  t_packet[TAG_STATE_IDX];
			TID_RECORD[j].Sensor_Type = t_packet[TAG_STYPE_IDX];
			TID_RECORD[j].Sensor_Data[0] = t_packet[TAG_SDATA_IDX];			
		}
	}
}
/*
@Description:自身参数设置
@Input:src:
@Output:
@Return:无
*/
void Radio_Cmd_Deal(void)
{
	uint8_t cmd;
	cmd_packet.length = 0;

	my_memcpy(cmd_packet.packet,packet,packet[RADIO_LENGTH_IDX]+RADIO_HEAD_LENGTH);
	if(0 == ID_CMP(DeviceID,packet + TAG_ID_IDX)== 0)//点对点
	{
		cmd = cmd_packet.packet[CMD_IDX];
		switch(cmd)
		{
			case FILE_CMD_READ://文件读取-点对点
				f_para.mode = cmd_packet.packet[FILE_MODE_IDX];
				f_para.offset = cmd_packet.packet[FILE_OFFSET_IDX]<<8|cmd_packet.packet[FILE_OFFSET_IDX+1];
				f_para.length = cmd_packet.packet[FILE_LENGTH_IDX];
				if(TRUE == Read_Para(f_para,cmd_packet.packet))
				{
					cmd_packet.length = CMD_ACK_FIX_LENGTH + f_para.length;
				}
				else//只返回执行状态
				{
					cmd_packet.length = CMD_ACK_FIX_LENGTH;
				}
				cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;//上行
				cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
				cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
				Radio_Period_Send(WithCmd,WithoutWin,SendWait);//等待发送完成
				break;
			case FILE_CMD_WRITE://文件写入-点对点
				f_para.mode = cmd_packet.packet[FILE_MODE_IDX];
				f_para.offset = cmd_packet.packet[FILE_OFFSET_IDX]<<8|cmd_packet.packet[FILE_OFFSET_IDX+1];
				f_para.length = cmd_packet.packet[FILE_LENGTH_IDX];
				Write_Para(f_para,cmd_packet.packet);
				cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;//上行
				cmd_packet.length = CMD_ACK_FIX_LENGTH;
				cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
				cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
				Radio_Period_Send(WithCmd,WithoutWin,SendWait);//等待发送完成
				break;
			default:break;	
		}							
	}
	else if(READER_ID_MBYTE == packet[TAG_ID_IDX])//广播
	{
		
	}

	
}

/************************************************* 
@Description:射频时间设置
@Input:无
@Output:无
@Return:无
*************************************************/ 
void Radio_Time_Set(uint8_t* tPacket,uint8_t* psrc)
{
	uint8_t len;
	tPacket[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0下行
	my_memcpy(&tPacket[TAG_ID_IDX],&psrc[TAG_ID_IDX],RADIO_ID_LENGTH);//2~5目标ID
	my_memcpy(&tPacket[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9读写器ID
	tPacket[CMD_IDX] = TIME_SET_CMD;//命令
	TIME_BCDToDec(&tPacket[CMD_IDX+1]);//命令参数
	len = CMD_FIX_LENGTH + TIME_PARA_LEN;
	tPacket[RADIO_LENGTH_IDX] = len;
	tPacket[len+RADIO_HEAD_LENGTH-1]=Get_Xor(tPacket,(len+1));
}

/************************************************* 
@Description:RADIO中断处理程序
@Input:无
@Output:无
@Return:无
*************************************************/
void Radio_RX_Deal(void)
{
	u16 state;
	if(TRUE == Xor_Check(packet,packet[RADIO_LENGTH_IDX]+2))
	{
		if( RADIO_RUN_DATA_CHANNEL == radio_run_channel )//数据频道采集标签信息
		{
			if(Idle ==  Work_Mode)
			{
				tag_record(packet);//标签收集
				if ( ((packet[TAG_STATE_IDX]&TAG_WIHTWIN_Msk)>>TAG_WITHWIN_Pos) == WithWin )//携带接收窗口
				{
					//消息处理
//					debug_printf("\n\r携带接收窗口");
					if(TRUE == Message_Get(packet[TAG_SDATA_IDX]))//并且有新信息发送
					{
						memcpy(Msg_Packet.MSG_PUSH_TID,packet+TAG_ID_IDX,RADIO_RID_LENGTH);
						Radio_MSG_Start(cmd_packet.packet,packet);
						cmd_packet.length = cmd_packet.packet[RADIO_LENGTH_IDX];//射频数据长度
						Radio_Period_Send(WithCmd,WithWin,SendNoWait);//配置频道下发命令
						Work_Mode = Msg_Deal;//消息处理
//						debug_printf("\n\r下发消息通知命令");
					}
					else
					{
//						debug_printf("\n\r没有消息要发送");
					}
					//时间设置
					if(1 == Need_Time_Set)
					{
						/********************************************************************
						时间设置机制：平台每天下发一次时间设置，当时间下发成功后，该标志位置位，
						当标签携带接收窗口，并且时间未更新过，则通过射频下发时间设置,一个小时过后，停止时间更新功能
						*******************************************************************/
						if( ((packet[TAG_STATE_IDX]&TAG_TIMEUPDATE_Msk)>>TAG_TIMEUPDATE_Pos) == Time_Update )
						{
							Radio_Time_Set(cmd_packet.packet,packet);
							cmd_packet.length = cmd_packet.packet[RADIO_LENGTH_IDX];//射频数据长度
							Radio_Period_Send(WithCmd,WithWin,SendNoWait);//配置频道下发命令
							Work_Mode = Time_Set;
						}
					}
				}
			}
			else if( File_Deal == Work_Mode)//文件处理
			{
				if(0 == ID_CMP(&packet[TAG_ID_IDX],&cmd_packet.packet[TAG_ID_IDX]))//目标ID
				{
					if( ((packet[TAG_STATE_IDX]&TAG_WIHTWIN_Msk)>>TAG_WITHWIN_Pos) == WithWin )//目标携带接收窗
					{
						Radio_Period_Send(WithCmd,WithWin,SendNoWait);//配置频道下发命令
					}
				}	
			}
			else if(List_Tag == Work_Mode)//列出标签
			{
				if(packet[TAG_ID_IDX]!=READER_ID_MBYTE)//标签信息收集
				{
					tag_record(packet);//标签收集
				}
			}
		}
		else if( RADIO_RUN_CONFIG_CHANNEL == radio_run_channel)
		{
			//射频设置自身参数
			if((packet[RADIO_S0_IDX]&RADIO_S0_DIR_Msk) == RADIO_S0_DIR_DOWN) //下行
			{
				radio_rcvok = 1;
			}
			//射频设置其他设备
			if( File_Deal == Work_Mode)//文件处理
			{
				if(0 == ID_CMP(&packet[TAG_ID_IDX],&cmd_packet.packet[TAG_ID_IDX]))//目标ID
				{
					//命令接收完成，串口回复
					if(packet[CMD_IDX] == FILE_CMD_READ || packet[CMD_IDX] == FILE_CMD_WRITE)
					{
						my_memcpy(cmd_packet.packet,packet,packet[RADIO_LENGTH_IDX]+RADIO_HEAD_LENGTH);
						U_Master.tx_en = 1;
					}
				}
			}
			else if( Msg_Deal == Work_Mode )//消息处理
			{
				//ID匹配时，才允许通信
				if(0 == memcmp(&packet[TAG_ID_IDX],Msg_Packet.MSG_PUSH_TID,RADIO_ID_LENGTH))
				{
					switch(MSG_PUSH_State)
					{
						case MSG_WAIT:
							state = packet[CMD_IDX+1] << 8| packet[CMD_IDX+2];
							if(MESSAGE_CMD == packet[CMD_IDX]&& MSG_START_END_VALUE == state)//消息命令及标签命令回复正确
							{
								if(Msg_Packet.PKT_PUSH_NUM)
								{
									Radio_MSG_Push(packet);//下发消息包
									Radio_Period_Send(WithCmd,WithWin,SendNoWait);//配置频道下发命令
									Msg_Packet.MSG_RE_PUSH = 0;//重发次数清0
//									debug_printf("\n\r 下发packt%d",Msg_Packet.PKT_PUSH_SEQ);	
								}
							}
							else
							{
//								debug_printf("\n\r MSG_WAIT命令回复错误");
							}
							break;
						case MSG_Packet0:			
						case MSG_Packet1:
						case MSG_Packet2:
						case MSG_Packet3:
							state = packet[CMD_IDX+1] << 8| packet[CMD_IDX+2];
							//回复的包头 == 下发的包头
							if(packet[MSG_HEAD_IDX] == Msg_Packet.MSG_PUSH_HEAD)
							{
								//命令执行成功
								if(packet[MSG_HEAD_IDX+1] == MSG_SUCCESS)
								{
									Msg_Packet.PKT_PUSH_NUM--;//发送成功再减一
									Msg_Packet.PKT_PUSH_SEQ++;//发送成功再加1
									if(Msg_Packet.PKT_PUSH_NUM > 0)//还有消息需要发送
									{
										Radio_MSG_Push(packet);//下发消息包
										Radio_Period_Send(WithCmd,WithWin,SendNoWait);//配置频道下发命令								
										Msg_Packet.MSG_RE_PUSH = 0;//重发次数清0	
//										debug_printf("\n\r 下发packt%d",Msg_Packet.PKT_PUSH_SEQ);									
									}
								}

							}
							//最后一包
							else if(MESSAGE_CMD == packet[CMD_IDX]&& MSG_START_END_VALUE == state)//发送完成
							{
								Work_Mode = Idle;
								MSG_PUSH_State = MSG_IDLE;	
//								debug_printf("\n\r 消息下发成功，回到空闲状态");	
							}
							//重发
							else 
							{
								//超过重发次数
								if(Msg_Packet.MSG_RE_PUSH > MSG_RPUSH_TIMES)
								{
									Work_Mode = Idle;
									MSG_PUSH_State = MSG_IDLE;
//									debug_printf("\n\r 消息下发成功，回到空闲状态");	
								}
								else
								{
									Radio_Period_Send(WithCmd,WithWin,SendNoWait);//配置频道下发命令
									MSG_PUSH_State = MSG_PUSH_State - 1;//回到上一个状态
//									debug_printf("\n\r 消息重发");								
								}

							}
							break;
					}					
				}

			}
		}
	}
}
/************************************************* 
@Description:RADIO中断处理程序-发送
@Input:无
@Output:无
@Return:无
*************************************************/
void Radio_TX_Deal(void)
{
	if(RADIO_RUN_CONFIG_CHANNEL == radio_run_channel)//配置频道
	{
		if(File_Deal == Work_Mode)//文件发送处理
		{
			radio_select(CONFIG_CHANNEL,RADIO_RX);//切换到配置频道接收
		}
		else if(Msg_Deal == Work_Mode)
		{
			switch(MSG_PUSH_State)
			{
				case MSG_IDLE:
					MSG_PUSH_State = MSG_WAIT;//消息开始包发送成功
//					debug_printf("\n\r MSG_WAIT 等待回复接收状态");
					break;
				case MSG_WAIT:
					MSG_PUSH_State = MSG_Packet0;//包0发送成功
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet0 等待回复接收状态");
					break;
				case MSG_Packet0:
					MSG_PUSH_State = MSG_Packet1;//包1发送成功
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet1 等待回复接收状态");
					break;	
				case MSG_Packet1:
					MSG_PUSH_State = MSG_Packet2;//包2发送成功
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet2 等待回复接收状态");
					break;	
				case MSG_Packet2:
					MSG_PUSH_State = MSG_Packet3;//包3发送成功
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet3 等待回复接收状态");
					break;			
				case MSG_Packet3:
					MSG_PUSH_State = MSG_Packet3;//包3发送成功
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r  等待回复消息结束接收状态");
					break;				
			}
			radio_select(CONFIG_CHANNEL,RADIO_RX);//切换到配置频道接收
			//超过重发次数
			if(Msg_Packet.MSG_RE_PUSH > MSG_RPUSH_TIMES)
			{
				Work_Mode = Idle;
				MSG_PUSH_State = MSG_IDLE;
			}
			
		}
		else if(Time_Set ==  Work_Mode)
		{
			radio_select(DATA_CHANNEL,RADIO_RX);//切换到配置频道接收
			Work_Mode = Idle;
		}
		
	}
	else if(RADIO_RUN_DATA_CHANNEL == radio_run_channel)
	{
		
	}
}
/************************************************* 
@Description:RADIO中断处理程序
@Input:无
@Output:无
@Return:无
*************************************************/  
uint8_t rx_cnt;
void RADIO_IRQHandler(void)
{
	if(NRF_RADIO->EVENTS_END)//EVENTS_END
	{
		NRF_RADIO->EVENTS_END = 0;
		if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle)//射频接收
		{	
			if(NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk)//CRC校验
			{
				Radio_RX_Deal();//射频处理
				rx_cnt++;
			}
			
		}
		else if(NRF_RADIO->STATE == RADIO_STATE_STATE_TxIdle)//射频发送
		{
			Radio_TX_Deal();
			radio_sndok = 1;//射频发送成功
		}
		if(RADIO_STATUS_RX == radio_status)
		{
			NRF_RADIO->TASKS_START = 1U;//重新启动射频
		}
	}
	
}
