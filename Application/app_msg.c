/*******************************************************************************
** ��Ȩ:		
** �ļ���: 		app_msg.c
** �汾��  		1.0
** ��������: 	MDK-ARM 5.23
** ����: 		cc
** ��������: 	2017-07-22
** ����:		  
** ����ļ�:	app_msg.h
** �޸���־��	
** ��Ȩ����   
*******************************************************************************/

#include "app_msg.h"
#include "app_var.h"
#include "sys.h"
#include "nrf_nvmc.h"
#include "string.h"
#include "Debug_log.h"
extern Payload_Typedef cmd_packet;//������Ƶ����
extern uint8_t DeviceID[4];
MSG_Store_Typedef MSG_Store;
extern ROM_BaseAddr_Typedef   ROM_BaseAddr;//ROM����ַ����
//��Ϣ����
//��Ϣ����
Message_Typedef Msg_Packet;
typedef enum
{
	pkt_seq0=0,
	pkt_seq1=1,
	pkt_seq2=2,
	pkt_seq3=3
}pkt_seq;

/************************************************* 
@Description:��������-��Ϣ��ʼ��
@Input:��
@Output:��
@Return:��
*************************************************/ 
void MSG_Addr_Init(void)
{
	MSG_Store.MSG1_ROM = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-5);
	MSG_Store.MSG2_ROM = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-6);
	MSG_Store.MSG3_ROM = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-7);
	MSG_Store.NEW_MSG_ROM = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-8);
	MSG_Store.MSG_ADDR[0] = &MSG_Store.MSG1_ROM;
	MSG_Store.MSG_ADDR[1] = &MSG_Store.MSG2_ROM;
	MSG_Store.MSG_ADDR[2] = &MSG_Store.MSG3_ROM;
}


/************************************************* 
@Description:��������-����������Ϣ
@Input:��
@Output:��
@Return:��
*************************************************/ 
void MSG_Erase_ALL(void)
{
	uint32_t msg_addr;
	//��Ϣ1
	msg_addr = MSG_Store.MSG1_ROM;
	nrf_nvmc_page_erase(msg_addr);
	//��Ϣ2
	msg_addr = MSG_Store.MSG2_ROM;
	nrf_nvmc_page_erase(msg_addr);
	//��Ϣ3
	msg_addr = MSG_Store.MSG3_ROM;
	nrf_nvmc_page_erase(msg_addr);
	//����������Ϣ��¼
	msg_addr  = MSG_Store.NEW_MSG_ROM;
	nrf_nvmc_page_erase(msg_addr);
	
	memset(&MSG_Store,0,sizeof(MSG_Store_Typedef));
	MSG_Addr_Init();
	
}		
/************************************************* 
@Description:��������-�����Ϣ���к�
@Input:��
@Output:��
@Return:��
*************************************************/ 
uint8_t MSG_ROM_Check(uint32_t temp_addr)
{
	if(*(uint8_t*)temp_addr <= MSG_SEQ_Max_Value)  //������к�
		return TRUE;
	return FALSE;
}
/************************************************* 
@Description:��������-����������Ϣ,��Ϣ������������Ϣ�洢�������š���ȡ��Ϣ���к�
���flash����Ϣ��������Ϣ�����Ų�����Ҫ��ȫ��������
@Input:��
@Output:��
@Return:��
*************************************************/ 
void MSG_Find_New(void)
{
	uint8_t msg_idx;
	//��ȡ�洢����Ϣ����
	MSG_Store.MSG_Num += MSG_ROM_Check(MSG_Store.MSG1_ROM);
	MSG_Store.MSG_Num += MSG_ROM_Check(MSG_Store.MSG2_ROM);
	MSG_Store.MSG_Num += MSG_ROM_Check(MSG_Store.MSG3_ROM);
	if(0 == MSG_Store.MSG_Num)
	{
		MSG_Erase_ALL();
		return;
	}
	//��ȡ������Ϣ������
	msg_idx = *(uint8_t*)MSG_Store.NEW_MSG_ROM;
	if(msg_idx <= MSG_IDX_MAX_Value)
	{
		MSG_Store.MSG_IDX = (MSG_IDX_Typedef)msg_idx;
	}
	else
	{
		MSG_Erase_ALL();
		return;
	}
	//��ȡ���к�
	MSG_Store.MSG_Seq = *(uint8_t*)(*MSG_Store.MSG_ADDR[MSG_Store.MSG_IDX]);
	if(MSG_Store.MSG_Seq > MSG_SEQ_Max_Value)
	{
		MSG_Erase_ALL();
		return;
	}
}
/************************************************* 
@Description:��������-��Ϣ��д��FLASH,�滻��ɵ���Ϣ
@Input:idx ������Ϣ�����ţ�һ����Ϣ
@Output:��
@Return:��
*************************************************/ 
void MSG_Write(uint8_t idx,u8* buff)
{
	uint32_t msg_addr;
	uint8_t m_idx;
	m_idx = (idx + MSG_FLASH_NUM - MSG_IDX_MAX_Value)%MSG_FLASH_NUM;//�����ż�ȥ2
	msg_addr = *MSG_Store.MSG_ADDR[m_idx];
	nrf_nvmc_page_erase(msg_addr);
	nrf_nvmc_write_bytes(msg_addr,buff,(buff[MSG_LEN_IDX]+MSG_FLASH_HEAD_LEN));
	MSG_Store.MSG_IDX = (MSG_Store.MSG_IDX + 1)%MSG_FLASH_NUM;//������+1
	msg_addr = MSG_Store.NEW_MSG_ROM;
	nrf_nvmc_page_erase(msg_addr);
	nrf_nvmc_write_byte(msg_addr,MSG_Store.MSG_IDX);//д������������
}

/************************************************* 
@Description:��ǩ-��Ϣ����λ
@Input:��
@Output:��
@Return:��
*************************************************/ 
void MSG_Packet_ReSet(void)
{
	
	MSG_Store.MSG_BUFF_IDX = MSG_STORE_IDX;//һ����Ϣ�洢��������λ
	Msg_Packet.msg_pkt_seq = 0;//�������λ
}

/************************************************* 
@Description:��ǩ-��Ϣͷ���
@Input:��
@Output:��
@Return:��
*************************************************/ 
u16 MessageHeadCheck(uint8_t msg_head)
{
	uint16_t cmd_state;
	uint8_t pkt_sq = ((msg_head &MSG_PKT_Seq_Msk)>>MSG_PKT_Seq_Pos);
	uint8_t pkt_end = ((msg_head &MSG_PKT_END_Msk)>>MSG_PKT_END_Pos);
	uint8_t message_seq = ((msg_head & MSG_SEQ_Msk)>>MSG_SEQ_Pos);
	if(message_seq==MSG_Store.MSG_Seq)
	{
		return MSG_REPEAT_ERROR;
	}
	//����ż��
	if(Msg_Packet.msg_pkt_seq == pkt_sq)
	{
		if(pkt_end)//һ������δ����
		{
//			debug_printf("\r\n���հ�%d",pkt_sq);
			Msg_Packet.msg_pkt_seq++;
			if(Msg_Packet.msg_pkt_seq>pkt_seq3)
				Msg_Packet.msg_pkt_seq = pkt_seq0;
		}
		else
		{
			Msg_Packet.msg_pkt_seq = pkt_seq0;
		}
		return (msg_head<<8)|MSG_SUCCESS;
	}
	else 
	{
		Msg_Packet.msg_pkt_seq = 0;
		Msg_Packet.msg_head = MSG_HEAD_Msk|message_seq|pkt_end|Msg_Packet.msg_pkt_seq;//�ظ���ȷ�İ����
		cmd_state = (Msg_Packet.msg_head<<8)|MSG_PKT_SEQ_ERROR;
		return cmd_state;
	}
	
}

/************************************************* 
@Description:��ǩ-��Ϣ����
@Input:p_mpacket��Ƶ����
@Output:��
@Return:��
*************************************************/ 
u16 Message_Deal(uint8_t *p_mpacket)
{
	u16 cmd_state;
	cmd_state = MessageHeadCheck(p_mpacket[MSG_HEAD_IDX]);
	if(MSG_SUCCESS == (cmd_state&MSG_ERROR_Msk))
	{
		u8 pkt_end = ((p_mpacket[MSG_HEAD_IDX] &MSG_PKT_END_Msk)>>MSG_PKT_END_Pos);
		//�洢��Ϣ
		if(MSG_Store.MSG_BUFF_IDX >= MSG_PKT_MAX_LEN)//��Ϣ���ȳ�����󻺳���
			return MSG_ERROR;
		Msg_Packet.msg_pkt_len = p_mpacket[RADIO_LENGTH_IDX] - MSG_PAYLOAD_FIX_LEN -MSG_HEAD_LEN;//��Ϣ���ݳ���
		my_memcpy(&MSG_Store.MSG_BUFF[MSG_Store.MSG_BUFF_IDX],&p_mpacket[MSG_DATA_IDX],Msg_Packet.msg_pkt_len);
		MSG_Store.MSG_BUFF_IDX +=Msg_Packet.msg_pkt_len; 
		if(0 == pkt_end)//��������д��flash��
		{
			MSG_Store.MSG_Seq = ((p_mpacket[MSG_HEAD_IDX] &MSG_SEQ_Msk)>>MSG_SEQ_Pos);//���°����
			MSG_Store.MSG_BUFF[MSG_SEQ_IDX] = MSG_Store.MSG_Seq;//�洢�����
			MSG_Store.MSG_BUFF[MSG_LEN_IDX] = MSG_Store.MSG_BUFF_IDX - MSG_FLASH_HEAD_LEN;//MSG_BUFF_IDX-2
			MSG_Write(MSG_Store.MSG_IDX,MSG_Store.MSG_BUFF);
			cmd_state = MSG_START_END_VALUE;
//			debug_printf("\r\n���������");
			MSG_Packet_ReSet();//�����������λ
			return cmd_state;
		}
		cmd_state = (p_mpacket[MSG_HEAD_IDX]<<8)|0x00;
	}
	return cmd_state;
}
/************************************************* 
@Description:��д��-��ȡҪ�·�����Ϣ
@Input:
@Output:
@Return:1:��ȡ�ɹ���0����ȡʧ��
*************************************************/ 
uint8_t Message_Get(uint8_t msg_head)
{
	uint8_t Diff;//��ֵ
	uint8_t i,j;
	uint32_t addr;
	uint8_t msg_len;
	uint8_t reminder;//����
	uint8_t tag_msg_seq;//��ǩ��Ϣ���
	tag_msg_seq = (msg_head&MSG_SEQ_Msk)>>MSG_SEQ_Pos;
	if(tag_msg_seq!=MSG_Store.MSG_Seq)
	{
		Diff = ((MSG_Store.MSG_Seq + MSG_SEQ_MAX_NUM - tag_msg_seq)%MSG_SEQ_MAX_NUM);
		Msg_Packet.MSG_PUSH_SEQ = (Diff>MSG_FLASH_NUM)?((MSG_Store.MSG_Seq + MSG_SEQ_MAX_NUM - MSG_FLASH_NUM +1)%8)://��Ϣ����������3�����д����ż�ȥ2
		(tag_msg_seq + MSG_SEQ_MAX_NUM + 1)%8;//��Ϣ���С��3�������Ϊ��ǩ��Ϣ���+1
		for(i=0;i<MSG_SEQ_MAX_NUM;i++)
		{
			if(Msg_Packet.MSG_PUSH_SEQ == *(uint8_t*)(*MSG_Store.MSG_ADDR[i]))
			{
				
				addr = *MSG_Store.MSG_ADDR[i];
				msg_len = *(uint8_t *)(addr+MSG_LEN_IDX);
				nrf_nvmc_read_bytes(addr,MSG_Store.MSG_PUSH,(msg_len+MSG_FLASH_HEAD_LEN));//��ȡ����
				//��ȡ�ְ���������ÿ���ְ��ĳ���
				Msg_Packet.PKT_PUSH_NUM = msg_len>>MSG_PACKET_OFFSET;
				for(j=0;j<Msg_Packet.PKT_PUSH_NUM;)
				{
					Msg_Packet.PKT_PUSH_LEN[j] = MSG_PACKET_MAX_VALUE;
					//��ȡÿ������
					my_memcpy(&Msg_Packet.PKT_PUSH_BUF[j][0],&MSG_Store.MSG_PUSH[MSG_STORE_IDX+(j<<MSG_PACKET_OFFSET)],MSG_PACKET_MAX_VALUE);
					j++;
				}
				reminder = msg_len%MSG_PACKET_MAX_VALUE;
				if(reminder)
				{
					//��ȡÿ������
					my_memcpy(&Msg_Packet.PKT_PUSH_BUF[j][0],&MSG_Store.MSG_PUSH[MSG_STORE_IDX+(j<<MSG_PACKET_OFFSET)],reminder);
					Msg_Packet.PKT_PUSH_LEN[j] = reminder;
					Msg_Packet.PKT_PUSH_NUM++;//�·��ķְ�����
				}
				else
				{
//					Msg_Packet.PKT_PUSH_LEN[j] = 0;
				}
				Msg_Packet.PKT_PUSH_SEQ = 0;//�ְ������0
				return TRUE;
			}
		}
		return FALSE;
	}
	return FALSE;
}

/************************************************* 
@Description:��д��-������Ϣ����
@Input:
@Output:
@Return:
*************************************************/ 
void Radio_MSG_Start(uint8_t *msg_buf,uint8_t* src)
{
	uint8_t len;
	msg_buf[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0����
	my_memcpy(&msg_buf[TAG_ID_IDX],&src[TAG_ID_IDX],RADIO_ID_LENGTH);//2~5Ŀ��ID
	my_memcpy(&msg_buf[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9��д��ID
	msg_buf[CMD_IDX] = MESSAGE_CMD;
	msg_buf[CMD_IDX+1] = MSG_START_END_VALUE>>8;
	msg_buf[CMD_IDX+2] = MSG_START_END_VALUE;
	len = CMD_FIX_LENGTH + MSG_START_END_LEN;
	msg_buf[RADIO_LENGTH_IDX] = len;
	msg_buf[len+RADIO_HEAD_LENGTH-1]=Get_Xor(msg_buf,(len+1));
}

/************************************************* 
@Description:��д��-��Ϣ����
@Input:
@Output:
@Return:
*************************************************/ 
void Radio_MSG_Push(uint8_t* src)
{
	cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0����
	my_memcpy(&cmd_packet.packet[TAG_ID_IDX],&src[TAG_ID_IDX],RADIO_ID_LENGTH);//2~5Ŀ��ID
	my_memcpy(&cmd_packet.packet[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9��д��ID
	cmd_packet.packet[CMD_IDX] = MESSAGE_CMD;//��Ϣ����
	Msg_Packet.MSG_PUSH_HEAD = 0;
	Msg_Packet.MSG_PUSH_HEAD |=MSG_HEAD_Msk;//����ָʾ����ʾЯ����Ϣ����
	Msg_Packet.MSG_PUSH_HEAD |=	((Msg_Packet.MSG_PUSH_SEQ<<MSG_SEQ_Pos)&MSG_SEQ_Msk);//�·�����Ϣ���
//	Msg_Packet.PKT_PUSH_NUM--;//���ͳɹ��ټ�һ
	if(Msg_Packet.PKT_PUSH_NUM-1)//��Ϣ������һ
	{
		Msg_Packet.MSG_PUSH_HEAD |= MSG_PKT_END_Msk;//���а�Ҫ����
	}
	Msg_Packet.MSG_PUSH_HEAD |= (Msg_Packet.PKT_PUSH_SEQ&MSG_PKT_Seq_Msk);//�����
	//��Ϣͷ
	cmd_packet.packet[MSG_HEAD_IDX] = Msg_Packet.MSG_PUSH_HEAD;
	//������Ϣ����
	my_memcpy(&cmd_packet.packet[MSG_DATA_IDX],&Msg_Packet.PKT_PUSH_BUF[Msg_Packet.PKT_PUSH_SEQ][0],
				Msg_Packet.PKT_PUSH_LEN[Msg_Packet.PKT_PUSH_SEQ]);
	//��Ե�̶�����+��Ϣͷ+��Ϣ���ݳ���
	cmd_packet.length = CMD_FIX_LENGTH + MSG_HEAD_LEN + Msg_Packet.PKT_PUSH_LEN[Msg_Packet.PKT_PUSH_SEQ];
	//��Ƶ����
	cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length ;
	//���
	cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,(cmd_packet.length+1));
	//��++
//	Msg_Packet.PKT_PUSH_SEQ++;//���ͳɹ��ټ�1
}

