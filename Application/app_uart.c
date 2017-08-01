#include "app_uart.h"
#include "app_radio.h"
#include "app_reader.h"
#include "crc16.h"
#include "app_msg.h"
#include "simple_uart.h"
#include "Debug_log.h"
#include "rtc.h"

UART_Typedef U_Master;//���崮�ڻ���
extern Payload_Typedef cmd_packet;//������Ƶ����
const uint8_t UConfig_Self_ID[4] = {0xFF,0XFF,0XFF,0XFB};
extern uint8_t DeviceID[4];
extern uint8_t Work_Mode;//����ģʽ
extern MSG_Store_Typedef MSG_Store;//��Ϣ
rtc_typedef tmp_Time; //��ʱ����ʱ��
extern rtc_typedef Global_Time;//ȫ��ʱ�� 

/********************************************************************
ʱ�����û��ƣ�ƽ̨ÿ���·�һ��ʱ�����ã���ʱ���·��ɹ��󣬸ñ�־λ��λ��
����ǩЯ�����մ��ڣ�����ʱ��δ���¹�����ͨ����Ƶ�·�ʱ�����ã�һ��Сʱ����ֹͣʱ����¹���
*******************************************************************/
uint8_t Need_Time_Set;//1����������ʱ��0������������ʱ��
uint8_t Hour;//һ��Сʱ����ֹͣʱ����¹���
/************************************************* 
@Description:�����
@Input:
@Output:��
@Return:��
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
@Description: ֹͣ����ʱ��
@Input:
@Output:��
@Return:��
*************************************************/  
void Stop_Update_Time(void)
{
	if(Hour!=Global_Time.hour)
	{
		Need_Time_Set = Time_NoUpdate;
	}
}

/************************************************* 
@Description:�����
@Input:
@Output:��
@Return:��
*************************************************/  
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
			U_DATA_LEN = ((U_Master.rx_buf[U_LEN_IDX]<<8)|U_Master.rx_buf[U_LEN_IDX+1]) - U_AfterLEN_FIX_LEN;//��Ϣ����
			U_CMD = U_Master.rx_buf[U_CMD_IDX];//�����
			switch(U_CMD)
			{
				case U_CMD_LIST_TAG:
					Work_Mode = File_Deal;
					break;
				case U_CMD_WRITE_FILE:
					if(0 == ID_CMP(&UConfig_Self_ID[0],&U_Master.rx_buf[U_DATA_IDX]))//�������������
					{
						
					}
					else
					{
						my_memset(cmd_packet.packet,0,PACKET_PAYLOAD_MAXSIZE);
						cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0����
						my_memcpy(&cmd_packet.packet[TAG_ID_IDX],&U_Master.rx_buf[U_DATA_IDX],RADIO_ID_LENGTH);//2~5Ŀ��ID
						my_memcpy(&cmd_packet.packet[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9��д��ID
						cmd_packet.packet[CMD_IDX] = FILE_CMD_WRITE;
						U_DATA_LEN = U_DATA_LEN - RADIO_ID_LENGTH - U_ConfigOT_LEN;//�·��������
						my_memcpy(&cmd_packet.packet[CMD_IDX+1],&U_Master.rx_buf[U_FileData_IDX],U_DATA_LEN);//��������
						cmd_packet.length = CMD_FIX_LENGTH + U_DATA_LEN ;
						cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
						cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
						Work_Mode = File_Deal;
	
						
					}
				break;
				case U_CMD_READ_FILE:
					if(0 == ID_CMP(&UConfig_Self_ID[0],&U_Master.rx_buf[U_DATA_IDX]))//�������������
					{
						
					}
					else
					{
						my_memset(cmd_packet.packet,0,PACKET_PAYLOAD_MAXSIZE);
						cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0����
						my_memcpy(&cmd_packet.packet[TAG_ID_IDX],&U_Master.rx_buf[U_DATA_IDX],RADIO_ID_LENGTH);//2~5Ŀ��ID
						my_memcpy(&cmd_packet.packet[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9��д��ID
						cmd_packet.packet[CMD_IDX] = FILE_CMD_READ;
						U_DATA_LEN = U_DATA_LEN - RADIO_ID_LENGTH - U_ConfigOT_LEN;//�·��������
						my_memcpy(&cmd_packet.packet[CMD_IDX+1],&U_Master.rx_buf[U_FileData_IDX],U_DATA_LEN);//��������
						cmd_packet.length = CMD_FIX_LENGTH + U_DATA_LEN ;
						cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length;
						cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,cmd_packet.length+1);
						Work_Mode = File_Deal;
					}
				break;
				case U_CMD_MSG_PUSH:
					U_DATA_LEN = U_Master.rx_buf[U_DATA_IDX];//��Ϣ����
					if(U_DATA_LEN>MSG_MAX_LEN)//��Ϣ���ȳ�����󳤶�
					{
						state = (U_MSG_ERR<<8)|MSG_Store.MSG_Seq;//����״̬+��Ϣ���
					}
					else 
					{
						MSG_Store.MSG_Seq = (MSG_Store.MSG_Seq + 1)%MSG_SEQ_MAX_NUM;//���к�+1
						MSG_Store.MSG_BUFF[MSG_SEQ_IDX] = MSG_Store.MSG_Seq;//��Ϣ���
						my_memcpy(&MSG_Store.MSG_BUFF[MSG_LEN_IDX],&U_Master.rx_buf[U_DATA_IDX],(U_Master.rx_buf[U_DATA_IDX]+1));//��flash��д����Ϣ
						MSG_Write(MSG_Store.MSG_IDX,MSG_Store.MSG_BUFF);
						state = U_MSG_SUCCESS|MSG_Store.MSG_Seq;//����״̬+��Ϣ���
					}
					U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
					U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
					U_Master.len = U_MSG_ACK_LEN;
					U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
					U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//Э��
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//��д��ID
					U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//��λ��Ϣ
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//��ˮ��
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_MSG_PUSH;//����
					U_Master.tx_buf[U_DATA_IDX] = state>>8;
					U_Master.tx_buf[U_DATA_IDX] = state;
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//����~��Ϣ����
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //֡ͷ����+���ȳ���+���Ⱥ���ĳ���
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//֡ͷ2+����2+У��2+���ݳ���
					UART_Send(U_Master.tx_buf,U_Master.len);
//					debug_printf("\n\r�ɹ�������Ϣ");
				break;
				case U_CMD_TIME_SET:
					tmp_Time.year = U_Master.rx_buf[U_CMD_IDX+1];//��
					tmp_Time.month = U_Master.rx_buf[U_CMD_IDX+2];//��
					tmp_Time.day = U_Master.rx_buf[U_CMD_IDX+3];//��
					tmp_Time.hour = U_Master.rx_buf[U_CMD_IDX+4];//ʱ
					tmp_Time.min = U_Master.rx_buf[U_CMD_IDX+5];//��
					tmp_Time.sec = U_Master.rx_buf[U_CMD_IDX+6];//��
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
						Need_Time_Set = Time_Update;//��������ʱ��
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
					U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//Э��
					my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//��д��ID
					U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//��λ��Ϣ
					U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//��ˮ��
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_TIME_SET;//����
					U_Master.tx_buf[U_DATA_IDX] = state>>8;
					U_Master.tx_buf[U_DATA_IDX] = state;
					crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//����~��Ϣ����
					crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //֡ͷ����+���ȳ���+���Ⱥ���ĳ���
					U_Master.tx_buf[crc_idx] = (crc>>8);
					U_Master.tx_buf[crc_idx+1] = crc;//crc
					U_Master.len = U_Master.len + U_FIX_LEN;//֡ͷ2+����2+У��2+���ݳ���
					UART_Send(U_Master.tx_buf,U_Master.len);
//					debug_printf("\n\r����ʱ��");
					
				break;
			}
		}
	}
}

/************************************************* 
@Description:��¼��������
@Input:
@Output:��
@Return:��
*************************************************/  
void Uart_ReceiveBuff(uint8_t rx_temp)
{
	switch(U_Master.rx_state)
	{
		case PKT_HEAD1://֡ͷ1
			if(pkt_head1 == rx_temp)	                                
			{                                                   
				U_Master.rx_idx = 0;//������Ϊ0
				U_Master.rx_buf[U_Master.rx_idx] = rx_temp;//��������
				U_Master.rx_state = PKT_HEAD2;//״̬�л�
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
			if(U_Master.rx_idx == len_finish)//���Ƚ������
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
		case PKT_DATA://��������
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
@Description:�����ж�
@Input:
@Output:��
@Return:��
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

