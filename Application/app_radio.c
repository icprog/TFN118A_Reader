#include "app_radio.h"
#include "string.h" 

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



extern uint8_t packet[PACKET_PAYLOAD_MAXSIZE];
extern uint8_t DeviceID[4];
extern uint8_t para_record[PARA_RECORD_LEN];

extern uint8_t ActiveMode;//���ڷ������־
//��Ƶ���շ������
uint8_t radio_rcvok = 0;
uint8_t radio_sndok = 0;
uint8_t m_packet[PACKET_PAYLOAD_MAXSIZE];
//��ǩ״̬��
uint8_t State_LP_Alarm;//�͵籨��
uint8_t State_Key_Alram;//��������
const static uint8_t State_WithSensor = 1;//1:���б�ǩ 0���Ǵ��б�ǩ
//uint8_t State_WithWin;
uint8_t State_Mode;//ģʽ
//��������
uint8_t M_Seq;//��Ϣ���к�0~7
//payload����
uint8_t payload_length;
#define RADIO_RX_OT 	825

void radio_pwr(uint8_t txpower);
static void Radio_Period_Send(uint8_t cmdflag,uint8_t winflag);

//�ļ�����
File_Typedef f_para;//�ļ������������ݻ���

//��д��-��¼��ǩ
TID_Typedef  TID_RECORD[CAPACITY];
/*
Description:��Ƶ����
Input:state :
Output:��
Return:��
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
static void radio_select(uint8_t ch,uint8_t dir)
{
	uint8_t channel;
	radio_on();//��������
	channel = (ch == DATA_CHANNEL)?RADIO_CHANNEL_DATA:RADIO_CHANNEL_CONFIG;
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
Description:��Ƶ���ڷ���
Input:state :
Output:��
Return:��
*/
void Raio_Deal(void)
{
//	static uint8_t wincount;
//	uint32_t ot;
//	UpdateRunPara();//�����ڲ�����-��Ҫ���·��书��
//	wincount++;
//	if(wincount >= win_interval)
//	{
//		wincount = 0;
//		Radio_Period_Send(WithoutCmd,WithWin);//���ʹ����մ���
//		radio_select(CONFIG_CHANNEL,RADIO_RX);
//		ot = RADIO_RX_OT;//���մ�ʱ��
//		while(ot--)
//		{
//			if(radio_rcvok)
//				break;
//		}
//	}
//	else
//	{
//		if(ActiveMode)
//			Radio_Period_Send(WithoutCmd,WithoutWin);//���Ͳ������մ�
//	}
//	radio_off();
	radio_select(DATA_CHANNEL,RADIO_RX);
}

/***********************************************************
Description:���ڷ�����Ƶ��Ϣ,���ȴ��������
Input��	cmdflag - ����� 1���������� 0�����淢��
				winflag - �Ƿ�Я������
Output����
Return:��
************************************************************/
static void Radio_Period_Send(uint8_t cmdflag,uint8_t winflag)
{
	uint32_t ot = 0;
	my_memset(packet,0,PACKET_PAYLOAD_MAXSIZE);
	packet[RADIO_S0_IDX] = RADIO_S0_DIR_UP;
	packet[RADIO_LENGTH_IDX] = 0; //payload���ȣ���������
	my_memcpy(packet+TAG_ID_IDX,DeviceID,4);//2~5��ǩID
	if(cmdflag)
	{
		memcpy(packet,m_packet,payload_length+RADIO_HEAD_LENGTH);
	}
	else
	{
		packet[TAG_STATE_IDX] |= State_LP_Alarm << TAG_LOWPWR_Pos;//�͵�ָʾ
		packet[TAG_STATE_IDX] |= State_Key_Alram << TAG_KEY_Pos;//����ָʾ
		packet[TAG_STATE_IDX] |= State_WithSensor << TAG_WITHSENSOR_Pos;//����ָʾ
		packet[TAG_STATE_IDX] |= winflag << TAG_WITHWIN_Pos;//���մ�ָʾ
		packet[TAG_STATE_IDX] |= State_Mode << TAG_MODE_Pos;//ģʽָʾ
		packet[TAG_VERSION_IDX] |= TAG_HDVER_NUM << TAG_HDVERSION_POS;//Ӳ���汾��
		packet[TAG_VERSION_IDX] |= TAG_SFVER_NUM << TAG_SFVERSION_POS;//����汾��
		packet[TAG_STYPE_IDX] = TAG_SENSORTYPE_SchoolWatch;//��ǩ����
		packet[TAG_SDATA_IDX] |= M_Seq << TAG_MSEQ_Pos;//��������
		payload_length = TAG_SDATA_IDX;//PAYLOAD���� 
		packet[RADIO_LENGTH_IDX] = payload_length ;
		packet[TAG_SDATA_IDX+1] = Get_Xor(packet,payload_length+1);//S0+max_length+PAYLOAD
	}
	if(cmdflag)
	{
		radio_select(CONFIG_CHANNEL,RADIO_TX);
	}
	else
	{
		radio_select(DATA_CHANNEL,RADIO_TX);
	}
	while(radio_sndok == 0)
	{
		ot++;
		if(ot > RADIO_OVER_TIME)
			break;
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
		case TAGP_PWR_N30DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg30dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_N20DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg20dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_N16DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg16dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_N12DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg12dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_N8DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg8dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_N4DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_Neg4dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_P0DBM:
			NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
			break;
		case TAGP_PWR_P4DBM:
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
		return 1;
	else
		return 0;
}

/*
Description:ID�Ա�
Input:src:
Output:
Return:��
*/
static uint8_t ID_CMP(const uint8_t *src,const uint8_t *dest)
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
void tag_record(uint8_t *p_packet)
{
	uint8_t i,j;
	for(i=0;i<CAPACITY;i++)
	{
		if(p_packet[TAG_ID_IDX]==0xff)
		{
			j=i;//��¼��������
		}
		else if(0 == ID_CMP(&p_packet[TAG_ID_IDX],&TID_RECORD[i].TID[0]))
		{
			j=i;
			break;
		}
	}
}
/*
Description:��Ƶ����������жϺ�����
Input:src:
Output:
Return:��
*/
void Radio_Cmd_Deal(void)
{
	uint8_t cmd;
	payload_length = 0;
	// if(radio_rcvok)
	// {
		if((packet[RADIO_S0_IDX]&RADIO_S0_DIR_Msk) == RADIO_S0_DIR_DOWN)
		{
			my_memcpy(m_packet,packet,packet[RADIO_LENGTH_IDX]+2);
			if(ID_CMP(DeviceID,packet + TAG_ID_IDX)== 00)//��Ե�
			{
				cmd = m_packet[CMD_IDX];
				switch(cmd)
				{
					case FILE_CMD_READ://�ļ���ȡ-��Ե�
						f_para.mode = m_packet[FILE_MODE_IDX];
						f_para.offset = m_packet[FILE_OFFSET_IDX]<<8|m_packet[FILE_OFFSET_IDX+1];
						f_para.length = m_packet[FILE_LENGTH_IDX];
						if(TRUE == Read_Para(f_para,m_packet))
						{
							payload_length = CMD_FIX_LENGTH + f_para.length;
						}
						else//ֻ����ִ��״̬
						{
							payload_length = CMD_FIX_LENGTH;
						}
						m_packet[RADIO_LENGTH_IDX] = payload_length;
						m_packet[payload_length+RADIO_HEAD_LENGTH-1]=Get_Xor(m_packet,payload_length+1);
						Radio_Period_Send(WithCmd,WithoutWin);
						break;
					case FILE_CMD_WRITE://�ļ�д��-��Ե�
						f_para.mode = m_packet[FILE_MODE_IDX];
						f_para.offset = m_packet[FILE_OFFSET_IDX]<<8|m_packet[FILE_OFFSET_IDX+1];
						f_para.length = m_packet[FILE_LENGTH_IDX];
						Write_Para(f_para,m_packet);
						payload_length = CMD_FIX_LENGTH;
						m_packet[RADIO_LENGTH_IDX] = payload_length;
						m_packet[payload_length+RADIO_HEAD_LENGTH-1]=Get_Xor(m_packet,payload_length+1);
						Radio_Period_Send(WithCmd,WithoutWin);
						break;
					default:break;
						
				}							
			}
			else if(ID_BROADCAST_MBYTE == packet[TAG_ID_IDX])//�㲥
			{
				
			}

		}
		else if((packet[RADIO_S0_IDX]&RADIO_S0_DIR_Msk) == RADIO_S0_DIR_UP)//����
		{
			my_memcpy(m_packet,packet,packet[RADIO_LENGTH_IDX]+2);//������
			if(0 == memcmp(&m_packet[READER_ID_IDX],DeviceID,RADIO_RID_LENGTH))//�������
			{
				
			}
			else if()
			cmd = m_packet[CMD_IDX];
		}
	// }
}
/************************************************* 
Description:RADIO�жϴ������
Input:��
Output:��
Return:��
*************************************************/  
void RADIO_IRQHandler(void)
{
	if(NRF_RADIO->EVENTS_END)//EVENTS_END
	{
		NRF_RADIO->EVENTS_END = 0;
		if(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle)
		{	
			if(NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk)
			{
				if(1 == Xor_Check(packet,packet[2]+2))
				{
					Radio_Cmd_Deal();//�����
					radio_rcvok= 1;
				}

			}
			NRF_RADIO->TASKS_START = 1U;//����������Ƶ
		}
		else if(NRF_RADIO->STATE == RADIO_STATE_STATE_TxIdle)
		{
			radio_sndok= 1;
		}
	}
	
}
