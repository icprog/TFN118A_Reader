#include "app_radio.h"
#include "string.h" 
#include "app_reader.h"
#include "app_uart.h"
#include "app_msg.h"
#include "Debug_log.h"
#include "rtc.h"
extern uint8_t Work_Mode;//����ģʽ
//��Ҫ�޸ĵĲ���
#define win_interval 1//��ǩ�����մ��ڼ��
//Я������
typedef enum
{
	WithoutCmd = 0,
	WithCmd
}CMD_Typedef;
//Я�����մ�
typedef enum
{
	WithoutWin = 0,
	WithWin
}WIN_Typedef;
//�Ƿ�ȴ��������
typedef enum
{
	SendNoWait=0,
	SendWait=1
}Send_Wait_Typedef;


extern uint8_t packet[PACKET_PAYLOAD_MAXSIZE];
extern uint8_t DeviceID[4];
extern uint8_t para_record[PARA_RECORD_LEN];
//��д������
extern Para_Typedef Reader_Para;
//��Ƶ���շ������
uint8_t radio_rcvok = 0;//�����������
uint8_t radio_sndok = 0;
//��Ƶ������
Payload_Typedef cmd_packet;//������Ƶ����
extern uint8_t radio_status;//��Ƶ����״̬
uint8_t radio_run_channel;//��Ƶ����ͨ�� 
//��ǩ״̬��
uint8_t State_LP_Alarm;//�͵籨��
uint8_t State_Key_Alram;//��������
const static uint8_t State_WithSensor = 1;//1:���б�ǩ 0���Ǵ��б�ǩ
//uint8_t State_WithWin;
uint8_t State_Mode;//ģʽ
//��������
uint8_t M_Seq;//��Ϣ���к�0~7
uint8_t MSG_PUSH_State = MSG_IDLE;//��Ϣ����״̬
//��Ϣ����
extern Message_Typedef Msg_Packet;
#define RADIO_RX_OT 	825

void radio_pwr(uint8_t txpower);
static void Radio_Period_Send(uint8_t cmdflag,uint8_t winflag,uint8_t wait_send_finish);

//�ļ�����
File_Typedef f_para;//�ļ������������ݻ���
//���ڻ���
extern UART_Typedef U_Master;

//��д��-��¼��ǩ
TID_Typedef  TID_RECORD[CAPACITY];
void Radio_Cmd_Deal(void);//�����������
//ʱ������
extern uint8_t Need_Time_Set;//1����������ʱ��0������������ʱ��
/*
@Description:��ջ����¼
@Input:state :
@Output:��
@Return:��
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
@Description:��Ƶ����
@Input:state :
@Output:��
@Return:��
*/
static void radio_on(void)
{
	xosc_hfclk_start();//�ⲿ��������
}
/*
Description:��Ƶ�ر�
Input:state :
Output:��
Return:��
*/
static void radio_off(void)
{
	radio_disable();
	xosc_hfclk_stop();
}

/*
Description:��Ƶѡ��
Input:state :
Output:��
Return:��
*/
void radio_select(uint8_t ch,uint8_t dir)
{
	uint8_t channel;
	radio_on();//��������
	channel = (ch == DATA_CHANNEL)?RADIO_CHANNEL_DATA:RADIO_CHANNEL_CONFIG;
	radio_run_channel = (ch == DATA_CHANNEL)?RADIO_RUN_DATA_CHANNEL:RADIO_RUN_CONFIG_CHANNEL;
	NRF_RADIO->DATAWHITEIV = channel;//���ݰ׻�
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
@Description:��Ƶ���ڷ���
@Input:state :
@Output:��
@Return:��
*/
void Raio_Deal(void)
{
	uint32_t ot;
	//ƽʱ���ڽ���״̬��ż������һ�η���
	if(Reader_Para.radio_send_en)
	{
		Reader_Para.radio_send_en = 0;
		Radio_Period_Send(WithoutCmd,WithoutWin,SendWait);//�ȴ��������
		//���������Ƿ�����ͨ������Ƶ��������Ƶ����
		ot = RADIO_RX_OT;//���մ�ʱ��
		while(--ot)
		{
			if(radio_rcvok)
				break;
				
		}	
		if(radio_rcvok)//���ճɹ��������������
		{
			radio_rcvok = 0;
			Radio_Cmd_Deal();//�����
		}		
		radio_select(DATA_CHANNEL,RADIO_RX);
	}

}

/***********************************************************
@Description:���ڷ�����Ƶ��Ϣ,���ȴ��������
@Input��	cmdflag - ����� 1���������� 0�����淢��
				winflag - �Ƿ�Я������
				wait_send_finish:1:�ȴ�������ɣ�0:���ȴ�
@Output����
@Return:��
************************************************************/
static void Radio_Period_Send(uint8_t cmdflag,uint8_t winflag,uint8_t wait_send_finish)
{
	uint32_t ot = 0;

	if(cmdflag)
	{
		memcpy(packet,cmd_packet.packet,cmd_packet.length+RADIO_HEAD_LENGTH);
		radio_select(CONFIG_CHANNEL,RADIO_TX);//����Ƶ��
//		while()
	}
	else
	{
		my_memset(packet,0,PACKET_PAYLOAD_MAXSIZE);
		packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;
		packet[RADIO_LENGTH_IDX] = 0; //payload���ȣ���������
		my_memcpy(packet+TAG_ID_IDX,DeviceID,4);//2~5��ǩID
		packet[TAG_STATE_IDX] |= State_LP_Alarm << TAG_LOWPWR_Pos;//�͵�ָʾ
		packet[TAG_STATE_IDX] |= State_Key_Alram << TAG_KEY_Pos;//����ָʾ
		packet[TAG_STATE_IDX] |= State_WithSensor << TAG_WITHSENSOR_Pos;//����ָʾ
		packet[TAG_STATE_IDX] |= winflag << TAG_WITHWIN_Pos;//���մ�ָʾ
		packet[TAG_STATE_IDX] |= State_Mode << TAG_MODE_Pos;//ģʽָʾ
		packet[TAG_VERSION_IDX] |= TAG_HDVER_NUM << TAG_HDVERSION_POS;//Ӳ���汾��
		packet[TAG_VERSION_IDX] |= TAG_SFVER_NUM << TAG_SFVERSION_POS;//����汾��
		packet[TAG_STYPE_IDX] = TAG_SENSORTYPE_SchoolWatch;//��ǩ����
		packet[TAG_SDATA_IDX] |= M_Seq << TAG_MSEQ_Pos;//��������
		cmd_packet.length = TAG_SDATA_IDX;//PAYLOAD���� 
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
	//�Ƿ�ȴ��������
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
Description:��Ƶ����ѡ��
Input:���뷢�书��
Output:��
Return:��
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
Description:�����
Input:src:ԭ���飬����
Output:
Return:��
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
Description:ID�Ա�
Input:src:
Output:
Return:��
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
Description:��ǩ��¼
Input:src:
Output:
Return:��
*/
void tag_record(uint8_t *t_packet)
{
	uint8_t i,j;
	for(i=0,j=CAPACITY;i<CAPACITY;i++)
	{
		if(TID_RECORD[i].TID[0]==READER_ID_MBYTE)
		{
			if(j > i) j=i;//��¼�µ�һ������0xff��������
		}
		else if(0 == ID_CMP(&t_packet[TAG_ID_IDX],&TID_RECORD[i].TID[0]))
		{
			j=i;
			break;
		}
	}
	if(j < CAPACITY)//�����¼
	{
		if(i == CAPACITY)//�¼�¼
		{
			TID_RECORD[j].TID[0] = t_packet[TAG_ID_IDX];
			TID_RECORD[j].TID[1] = t_packet[TAG_ID_IDX + 1];
			TID_RECORD[j].TID[2] = t_packet[TAG_ID_IDX + 2];
			TID_RECORD[j].TID[3] = t_packet[TAG_ID_IDX + 3];
			TID_RECORD[j].State =  t_packet[TAG_STATE_IDX];
			TID_RECORD[j].Sensor_Type = t_packet[TAG_STYPE_IDX];
			TID_RECORD[j].Sensor_Data[0] = t_packet[TAG_SDATA_IDX];
		}
		else//��������
		{
			TID_RECORD[j].State =  t_packet[TAG_STATE_IDX];
			TID_RECORD[j].Sensor_Type = t_packet[TAG_STYPE_IDX];
			TID_RECORD[j].Sensor_Data[0] = t_packet[TAG_SDATA_IDX];			
		}
	}
}
/*
@Description:�����������
@Input:src:
@Output:
@Return:��
*/
void Radio_Cmd_Deal(void)
{
	uint8_t cmd;
	cmd_packet.length = 0;

	my_memcpy(cmd_packet.packet,packet,packet[RADIO_LENGTH_IDX]+RADIO_HEAD_LENGTH);
	if(0 == ID_CMP(DeviceID,packet + TAG_ID_IDX)== 0)//��Ե�
	{
		cmd = cmd_packet.packet[CMD_IDX];
		switch(cmd)
		{
			case FILE_CMD_READ://�ļ���ȡ-��Ե�
				f_para.mode = cmd_packet.packet[FILE_MODE_IDX];
				f_para.offset = cmd_packet.packet[FILE_OFFSET_IDX]<<8|cmd_packet.packet[FILE_OFFSET_IDX+1];
				f_para.length = cmd_packet.packet[FILE_LENGTH_IDX];
				if(TRUE == Read_Para(f_para,cmd_packet.packet))
				{
					cmd_packet.length = CMD_ACK_FIX_LENGTH + f_para.length;
				}
				else//ֻ����ִ��״̬
				{
					cmd_packet.length = CMD_ACK_FIX_LENGTH;
				}
				cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;//����
				cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
				cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
				Radio_Period_Send(WithCmd,WithoutWin,SendWait);//�ȴ��������
				break;
			case FILE_CMD_WRITE://�ļ�д��-��Ե�
				f_para.mode = cmd_packet.packet[FILE_MODE_IDX];
				f_para.offset = cmd_packet.packet[FILE_OFFSET_IDX]<<8|cmd_packet.packet[FILE_OFFSET_IDX+1];
				f_para.length = cmd_packet.packet[FILE_LENGTH_IDX];
				Write_Para(f_para,cmd_packet.packet);
				cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;//����
				cmd_packet.length = CMD_ACK_FIX_LENGTH;
				cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
				cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
				Radio_Period_Send(WithCmd,WithoutWin,SendWait);//�ȴ��������
				break;
			default:break;	
		}							
	}
	else if(READER_ID_MBYTE == packet[TAG_ID_IDX])//�㲥
	{
		
	}

	
}

/************************************************* 
@Description:��Ƶʱ������
@Input:��
@Output:��
@Return:��
*************************************************/ 
void Radio_Time_Set(uint8_t* tPacket,uint8_t* psrc)
{
	uint8_t len;
	tPacket[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0����
	my_memcpy(&tPacket[TAG_ID_IDX],&psrc[TAG_ID_IDX],RADIO_ID_LENGTH);//2~5Ŀ��ID
	my_memcpy(&tPacket[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9��д��ID
	tPacket[CMD_IDX] = TIME_SET_CMD;//����
	TIME_BCDToDec(&tPacket[CMD_IDX+1]);//�������
	len = CMD_FIX_LENGTH + TIME_PARA_LEN;
	tPacket[RADIO_LENGTH_IDX] = len;
	tPacket[len+RADIO_HEAD_LENGTH-1]=Get_Xor(tPacket,(len+1));
}

/************************************************* 
@Description:RADIO�жϴ������
@Input:��
@Output:��
@Return:��
*************************************************/
void Radio_RX_Deal(void)
{
	u16 state;
	if(TRUE == Xor_Check(packet,packet[RADIO_LENGTH_IDX]+2))
	{
		if( RADIO_RUN_DATA_CHANNEL == radio_run_channel )//����Ƶ���ɼ���ǩ��Ϣ
		{
			if(Idle ==  Work_Mode)
			{
				tag_record(packet);//��ǩ�ռ�
				if ( ((packet[TAG_STATE_IDX]&TAG_WIHTWIN_Msk)>>TAG_WITHWIN_Pos) == WithWin )//Я�����մ���
				{
					//��Ϣ����
//					debug_printf("\n\rЯ�����մ���");
					if(TRUE == Message_Get(packet[TAG_SDATA_IDX]))//����������Ϣ����
					{
						memcpy(Msg_Packet.MSG_PUSH_TID,packet+TAG_ID_IDX,RADIO_RID_LENGTH);
						Radio_MSG_Start(cmd_packet.packet,packet);
						cmd_packet.length = cmd_packet.packet[RADIO_LENGTH_IDX];//��Ƶ���ݳ���
						Radio_Period_Send(WithCmd,WithWin,SendNoWait);//����Ƶ���·�����
						Work_Mode = Msg_Deal;//��Ϣ����
//						debug_printf("\n\r�·���Ϣ֪ͨ����");
					}
					else
					{
//						debug_printf("\n\rû����ϢҪ����");
					}
					//ʱ������
					if(1 == Need_Time_Set)
					{
						/********************************************************************
						ʱ�����û��ƣ�ƽ̨ÿ���·�һ��ʱ�����ã���ʱ���·��ɹ��󣬸ñ�־λ��λ��
						����ǩЯ�����մ��ڣ�����ʱ��δ���¹�����ͨ����Ƶ�·�ʱ������,һ��Сʱ����ֹͣʱ����¹���
						*******************************************************************/
						if( ((packet[TAG_STATE_IDX]&TAG_TIMEUPDATE_Msk)>>TAG_TIMEUPDATE_Pos) == Time_Update )
						{
							Radio_Time_Set(cmd_packet.packet,packet);
							cmd_packet.length = cmd_packet.packet[RADIO_LENGTH_IDX];//��Ƶ���ݳ���
							Radio_Period_Send(WithCmd,WithWin,SendNoWait);//����Ƶ���·�����
							Work_Mode = Time_Set;
						}
					}
				}
			}
			else if( File_Deal == Work_Mode)//�ļ�����
			{
				if(0 == ID_CMP(&packet[TAG_ID_IDX],&cmd_packet.packet[TAG_ID_IDX]))//Ŀ��ID
				{
					if( ((packet[TAG_STATE_IDX]&TAG_WIHTWIN_Msk)>>TAG_WITHWIN_Pos) == WithWin )//Ŀ��Я�����մ�
					{
						Radio_Period_Send(WithCmd,WithWin,SendNoWait);//����Ƶ���·�����
					}
				}	
			}
			else if(List_Tag == Work_Mode)//�г���ǩ
			{
				if(packet[TAG_ID_IDX]!=READER_ID_MBYTE)//��ǩ��Ϣ�ռ�
				{
					tag_record(packet);//��ǩ�ռ�
				}
			}
		}
		else if( RADIO_RUN_CONFIG_CHANNEL == radio_run_channel)
		{
			//��Ƶ�����������
			if((packet[RADIO_S0_IDX]&RADIO_S0_DIR_Msk) == RADIO_S0_DIR_DOWN) //����
			{
				radio_rcvok = 1;
			}
			//��Ƶ���������豸
			if( File_Deal == Work_Mode)//�ļ�����
			{
				if(0 == ID_CMP(&packet[TAG_ID_IDX],&cmd_packet.packet[TAG_ID_IDX]))//Ŀ��ID
				{
					//���������ɣ����ڻظ�
					if(packet[CMD_IDX] == FILE_CMD_READ || packet[CMD_IDX] == FILE_CMD_WRITE)
					{
						my_memcpy(cmd_packet.packet,packet,packet[RADIO_LENGTH_IDX]+RADIO_HEAD_LENGTH);
						U_Master.tx_en = 1;
					}
				}
			}
			else if( Msg_Deal == Work_Mode )//��Ϣ����
			{
				//IDƥ��ʱ��������ͨ��
				if(0 == memcmp(&packet[TAG_ID_IDX],Msg_Packet.MSG_PUSH_TID,RADIO_ID_LENGTH))
				{
					switch(MSG_PUSH_State)
					{
						case MSG_WAIT:
							state = packet[CMD_IDX+1] << 8| packet[CMD_IDX+2];
							if(MESSAGE_CMD == packet[CMD_IDX]&& MSG_START_END_VALUE == state)//��Ϣ�����ǩ����ظ���ȷ
							{
								if(Msg_Packet.PKT_PUSH_NUM)
								{
									Radio_MSG_Push(packet);//�·���Ϣ��
									Radio_Period_Send(WithCmd,WithWin,SendNoWait);//����Ƶ���·�����
									Msg_Packet.MSG_RE_PUSH = 0;//�ط�������0
//									debug_printf("\n\r �·�packt%d",Msg_Packet.PKT_PUSH_SEQ);	
								}
							}
							else
							{
//								debug_printf("\n\r MSG_WAIT����ظ�����");
							}
							break;
						case MSG_Packet0:			
						case MSG_Packet1:
						case MSG_Packet2:
						case MSG_Packet3:
							state = packet[CMD_IDX+1] << 8| packet[CMD_IDX+2];
							//�ظ��İ�ͷ == �·��İ�ͷ
							if(packet[MSG_HEAD_IDX] == Msg_Packet.MSG_PUSH_HEAD)
							{
								//����ִ�гɹ�
								if(packet[MSG_HEAD_IDX+1] == MSG_SUCCESS)
								{
									Msg_Packet.PKT_PUSH_NUM--;//���ͳɹ��ټ�һ
									Msg_Packet.PKT_PUSH_SEQ++;//���ͳɹ��ټ�1
									if(Msg_Packet.PKT_PUSH_NUM > 0)//������Ϣ��Ҫ����
									{
										Radio_MSG_Push(packet);//�·���Ϣ��
										Radio_Period_Send(WithCmd,WithWin,SendNoWait);//����Ƶ���·�����								
										Msg_Packet.MSG_RE_PUSH = 0;//�ط�������0	
//										debug_printf("\n\r �·�packt%d",Msg_Packet.PKT_PUSH_SEQ);									
									}
								}

							}
							//���һ��
							else if(MESSAGE_CMD == packet[CMD_IDX]&& MSG_START_END_VALUE == state)//�������
							{
								Work_Mode = Idle;
								MSG_PUSH_State = MSG_IDLE;	
//								debug_printf("\n\r ��Ϣ�·��ɹ����ص�����״̬");	
							}
							//�ط�
							else 
							{
								//�����ط�����
								if(Msg_Packet.MSG_RE_PUSH > MSG_RPUSH_TIMES)
								{
									Work_Mode = Idle;
									MSG_PUSH_State = MSG_IDLE;
//									debug_printf("\n\r ��Ϣ�·��ɹ����ص�����״̬");	
								}
								else
								{
									Radio_Period_Send(WithCmd,WithWin,SendNoWait);//����Ƶ���·�����
									MSG_PUSH_State = MSG_PUSH_State - 1;//�ص���һ��״̬
//									debug_printf("\n\r ��Ϣ�ط�");								
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
@Description:RADIO�жϴ������-����
@Input:��
@Output:��
@Return:��
*************************************************/
void Radio_TX_Deal(void)
{
	if(RADIO_RUN_CONFIG_CHANNEL == radio_run_channel)//����Ƶ��
	{
		if(File_Deal == Work_Mode)//�ļ����ʹ���
		{
			radio_select(CONFIG_CHANNEL,RADIO_RX);//�л�������Ƶ������
		}
		else if(Msg_Deal == Work_Mode)
		{
			switch(MSG_PUSH_State)
			{
				case MSG_IDLE:
					MSG_PUSH_State = MSG_WAIT;//��Ϣ��ʼ�����ͳɹ�
//					debug_printf("\n\r MSG_WAIT �ȴ��ظ�����״̬");
					break;
				case MSG_WAIT:
					MSG_PUSH_State = MSG_Packet0;//��0���ͳɹ�
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet0 �ȴ��ظ�����״̬");
					break;
				case MSG_Packet0:
					MSG_PUSH_State = MSG_Packet1;//��1���ͳɹ�
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet1 �ȴ��ظ�����״̬");
					break;	
				case MSG_Packet1:
					MSG_PUSH_State = MSG_Packet2;//��2���ͳɹ�
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet2 �ȴ��ظ�����״̬");
					break;	
				case MSG_Packet2:
					MSG_PUSH_State = MSG_Packet3;//��3���ͳɹ�
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r MSG_Packet3 �ȴ��ظ�����״̬");
					break;			
				case MSG_Packet3:
					MSG_PUSH_State = MSG_Packet3;//��3���ͳɹ�
					Msg_Packet.MSG_RE_PUSH++;
//					debug_printf("\n\r  �ȴ��ظ���Ϣ��������״̬");
					break;				
			}
			radio_select(CONFIG_CHANNEL,RADIO_RX);//�л�������Ƶ������
			//�����ط�����
			if(Msg_Packet.MSG_RE_PUSH > MSG_RPUSH_TIMES)
			{
				Work_Mode = Idle;
				MSG_PUSH_State = MSG_IDLE;
			}
			
		}
		else if(Time_Set ==  Work_Mode)
		{
			radio_select(DATA_CHANNEL,RADIO_RX);//�л�������Ƶ������
			Work_Mode = Idle;
		}
		
	}
	else if(RADIO_RUN_DATA_CHANNEL == radio_run_channel)
	{
		
	}
}
/************************************************* 
@Description:RADIO�жϴ������
@Input:��
@Output:��
@Return:��
*************************************************/  
uint8_t rx_cnt;
void RADIO_IRQHandler(void)
{
	if(NRF_RADIO->EVENTS_END)//EVENTS_END
	{
		NRF_RADIO->EVENTS_END = 0;
		if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle)//��Ƶ����
		{	
			if(NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk)//CRCУ��
			{
				Radio_RX_Deal();//��Ƶ����
				rx_cnt++;
			}
			
		}
		else if(NRF_RADIO->STATE == RADIO_STATE_STATE_TxIdle)//��Ƶ����
		{
			Radio_TX_Deal();
			radio_sndok = 1;//��Ƶ���ͳɹ�
		}
		if(RADIO_STATUS_RX == radio_status)
		{
			NRF_RADIO->TASKS_START = 1U;//����������Ƶ
		}
	}
	
}
