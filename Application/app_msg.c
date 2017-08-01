/*******************************************************************************
** 版权:		
** 文件名: 		app_msg.c
** 版本：  		1.0
** 工作环境: 	MDK-ARM 5.23
** 作者: 		cc
** 生成日期: 	2017-07-22
** 功能:		  
** 相关文件:	app_msg.h
** 修改日志：	
** 版权所有   
*******************************************************************************/

#include "app_msg.h"
#include "app_var.h"
#include "sys.h"
#include "nrf_nvmc.h"
#include "string.h"
#include "Debug_log.h"
extern Payload_Typedef cmd_packet;//命令射频处理
extern uint8_t DeviceID[4];
MSG_Store_Typedef MSG_Store;
extern ROM_BaseAddr_Typedef   ROM_BaseAddr;//ROM基地址定义
//消息数量
//消息处理
Message_Typedef Msg_Packet;
typedef enum
{
	pkt_seq0=0,
	pkt_seq1=1,
	pkt_seq2=2,
	pkt_seq3=3
}pkt_seq;

/************************************************* 
@Description:公共函数-消息初始化
@Input:无
@Output:无
@Return:无
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
@Description:公共函数-擦除所有消息
@Input:无
@Output:无
@Return:无
*************************************************/ 
void MSG_Erase_ALL(void)
{
	uint32_t msg_addr;
	//消息1
	msg_addr = MSG_Store.MSG1_ROM;
	nrf_nvmc_page_erase(msg_addr);
	//消息2
	msg_addr = MSG_Store.MSG2_ROM;
	nrf_nvmc_page_erase(msg_addr);
	//消息3
	msg_addr = MSG_Store.MSG3_ROM;
	nrf_nvmc_page_erase(msg_addr);
	//擦除最新消息记录
	msg_addr  = MSG_Store.NEW_MSG_ROM;
	nrf_nvmc_page_erase(msg_addr);
	
	memset(&MSG_Store,0,sizeof(MSG_Store_Typedef));
	MSG_Addr_Init();
	
}		
/************************************************* 
@Description:公共函数-检查消息序列号
@Input:无
@Output:无
@Return:无
*************************************************/ 
uint8_t MSG_ROM_Check(uint32_t temp_addr)
{
	if(*(uint8_t*)temp_addr <= MSG_SEQ_Max_Value)  //检查序列号
		return TRUE;
	return FALSE;
}
/************************************************* 
@Description:公共函数-查找最新消息,消息数量、最新消息存储区索引号、获取消息序列号
如果flash中消息数量和消息索引号不符合要求，全部擦除。
@Input:无
@Output:无
@Return:无
*************************************************/ 
void MSG_Find_New(void)
{
	uint8_t msg_idx;
	//获取存储区消息数量
	MSG_Store.MSG_Num += MSG_ROM_Check(MSG_Store.MSG1_ROM);
	MSG_Store.MSG_Num += MSG_ROM_Check(MSG_Store.MSG2_ROM);
	MSG_Store.MSG_Num += MSG_ROM_Check(MSG_Store.MSG3_ROM);
	if(0 == MSG_Store.MSG_Num)
	{
		MSG_Erase_ALL();
		return;
	}
	//获取最新消息索引号
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
	//获取序列号
	MSG_Store.MSG_Seq = *(uint8_t*)(*MSG_Store.MSG_ADDR[MSG_Store.MSG_IDX]);
	if(MSG_Store.MSG_Seq > MSG_SEQ_Max_Value)
	{
		MSG_Erase_ALL();
		return;
	}
}
/************************************************* 
@Description:公共函数-消息包写入FLASH,替换最旧的消息
@Input:idx 最新消息索引号，一包消息
@Output:无
@Return:无
*************************************************/ 
void MSG_Write(uint8_t idx,u8* buff)
{
	uint32_t msg_addr;
	uint8_t m_idx;
	m_idx = (idx + MSG_FLASH_NUM - MSG_IDX_MAX_Value)%MSG_FLASH_NUM;//索引号减去2
	msg_addr = *MSG_Store.MSG_ADDR[m_idx];
	nrf_nvmc_page_erase(msg_addr);
	nrf_nvmc_write_bytes(msg_addr,buff,(buff[MSG_LEN_IDX]+MSG_FLASH_HEAD_LEN));
	MSG_Store.MSG_IDX = (MSG_Store.MSG_IDX + 1)%MSG_FLASH_NUM;//索引号+1
	msg_addr = MSG_Store.NEW_MSG_ROM;
	nrf_nvmc_page_erase(msg_addr);
	nrf_nvmc_write_byte(msg_addr,MSG_Store.MSG_IDX);//写入最新索引号
}

/************************************************* 
@Description:标签-消息包置位
@Input:无
@Output:无
@Return:无
*************************************************/ 
void MSG_Packet_ReSet(void)
{
	
	MSG_Store.MSG_BUFF_IDX = MSG_STORE_IDX;//一条消息存储索引号置位
	Msg_Packet.msg_pkt_seq = 0;//包序号置位
}

/************************************************* 
@Description:标签-消息头检测
@Input:无
@Output:无
@Return:无
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
	//包序号检查
	if(Msg_Packet.msg_pkt_seq == pkt_sq)
	{
		if(pkt_end)//一包数据未传完
		{
//			debug_printf("\r\n接收包%d",pkt_sq);
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
		Msg_Packet.msg_head = MSG_HEAD_Msk|message_seq|pkt_end|Msg_Packet.msg_pkt_seq;//回复正确的包序号
		cmd_state = (Msg_Packet.msg_head<<8)|MSG_PKT_SEQ_ERROR;
		return cmd_state;
	}
	
}

/************************************************* 
@Description:标签-消息处理
@Input:p_mpacket射频数据
@Output:无
@Return:无
*************************************************/ 
u16 Message_Deal(uint8_t *p_mpacket)
{
	u16 cmd_state;
	cmd_state = MessageHeadCheck(p_mpacket[MSG_HEAD_IDX]);
	if(MSG_SUCCESS == (cmd_state&MSG_ERROR_Msk))
	{
		u8 pkt_end = ((p_mpacket[MSG_HEAD_IDX] &MSG_PKT_END_Msk)>>MSG_PKT_END_Pos);
		//存储消息
		if(MSG_Store.MSG_BUFF_IDX >= MSG_PKT_MAX_LEN)//消息长度超出最大缓冲区
			return MSG_ERROR;
		Msg_Packet.msg_pkt_len = p_mpacket[RADIO_LENGTH_IDX] - MSG_PAYLOAD_FIX_LEN -MSG_HEAD_LEN;//消息内容长度
		my_memcpy(&MSG_Store.MSG_BUFF[MSG_Store.MSG_BUFF_IDX],&p_mpacket[MSG_DATA_IDX],Msg_Packet.msg_pkt_len);
		MSG_Store.MSG_BUFF_IDX +=Msg_Packet.msg_pkt_len; 
		if(0 == pkt_end)//包结束，写入flash中
		{
			MSG_Store.MSG_Seq = ((p_mpacket[MSG_HEAD_IDX] &MSG_SEQ_Msk)>>MSG_SEQ_Pos);//更新包序号
			MSG_Store.MSG_BUFF[MSG_SEQ_IDX] = MSG_Store.MSG_Seq;//存储包序号
			MSG_Store.MSG_BUFF[MSG_LEN_IDX] = MSG_Store.MSG_BUFF_IDX - MSG_FLASH_HEAD_LEN;//MSG_BUFF_IDX-2
			MSG_Write(MSG_Store.MSG_IDX,MSG_Store.MSG_BUFF);
			cmd_state = MSG_START_END_VALUE;
//			debug_printf("\r\n包接收完成");
			MSG_Packet_ReSet();//包接收完成置位
			return cmd_state;
		}
		cmd_state = (p_mpacket[MSG_HEAD_IDX]<<8)|0x00;
	}
	return cmd_state;
}
/************************************************* 
@Description:读写器-获取要下发的消息
@Input:
@Output:
@Return:1:获取成功，0：获取失败
*************************************************/ 
uint8_t Message_Get(uint8_t msg_head)
{
	uint8_t Diff;//差值
	uint8_t i,j;
	uint32_t addr;
	uint8_t msg_len;
	uint8_t reminder;//余数
	uint8_t tag_msg_seq;//标签消息序号
	tag_msg_seq = (msg_head&MSG_SEQ_Msk)>>MSG_SEQ_Pos;
	if(tag_msg_seq!=MSG_Store.MSG_Seq)
	{
		Diff = ((MSG_Store.MSG_Seq + MSG_SEQ_MAX_NUM - tag_msg_seq)%MSG_SEQ_MAX_NUM);
		Msg_Packet.MSG_PUSH_SEQ = (Diff>MSG_FLASH_NUM)?((MSG_Store.MSG_Seq + MSG_SEQ_MAX_NUM - MSG_FLASH_NUM +1)%8)://消息序号想减大于3，则读写器序号减去2
		(tag_msg_seq + MSG_SEQ_MAX_NUM + 1)%8;//消息序号小于3，则序号为标签消息序号+1
		for(i=0;i<MSG_SEQ_MAX_NUM;i++)
		{
			if(Msg_Packet.MSG_PUSH_SEQ == *(uint8_t*)(*MSG_Store.MSG_ADDR[i]))
			{
				
				addr = *MSG_Store.MSG_ADDR[i];
				msg_len = *(uint8_t *)(addr+MSG_LEN_IDX);
				nrf_nvmc_read_bytes(addr,MSG_Store.MSG_PUSH,(msg_len+MSG_FLASH_HEAD_LEN));//读取数据
				//获取分包个数，及每个分包的长度
				Msg_Packet.PKT_PUSH_NUM = msg_len>>MSG_PACKET_OFFSET;
				for(j=0;j<Msg_Packet.PKT_PUSH_NUM;)
				{
					Msg_Packet.PKT_PUSH_LEN[j] = MSG_PACKET_MAX_VALUE;
					//获取每包数据
					my_memcpy(&Msg_Packet.PKT_PUSH_BUF[j][0],&MSG_Store.MSG_PUSH[MSG_STORE_IDX+(j<<MSG_PACKET_OFFSET)],MSG_PACKET_MAX_VALUE);
					j++;
				}
				reminder = msg_len%MSG_PACKET_MAX_VALUE;
				if(reminder)
				{
					//获取每包数据
					my_memcpy(&Msg_Packet.PKT_PUSH_BUF[j][0],&MSG_Store.MSG_PUSH[MSG_STORE_IDX+(j<<MSG_PACKET_OFFSET)],reminder);
					Msg_Packet.PKT_PUSH_LEN[j] = reminder;
					Msg_Packet.PKT_PUSH_NUM++;//下发的分包个数
				}
				else
				{
//					Msg_Packet.PKT_PUSH_LEN[j] = 0;
				}
				Msg_Packet.PKT_PUSH_SEQ = 0;//分包序号清0
				return TRUE;
			}
		}
		return FALSE;
	}
	return FALSE;
}

/************************************************* 
@Description:读写器-启动消息命令
@Input:
@Output:
@Return:
*************************************************/ 
void Radio_MSG_Start(uint8_t *msg_buf,uint8_t* src)
{
	uint8_t len;
	msg_buf[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0下行
	my_memcpy(&msg_buf[TAG_ID_IDX],&src[TAG_ID_IDX],RADIO_ID_LENGTH);//2~5目标ID
	my_memcpy(&msg_buf[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9读写器ID
	msg_buf[CMD_IDX] = MESSAGE_CMD;
	msg_buf[CMD_IDX+1] = MSG_START_END_VALUE>>8;
	msg_buf[CMD_IDX+2] = MSG_START_END_VALUE;
	len = CMD_FIX_LENGTH + MSG_START_END_LEN;
	msg_buf[RADIO_LENGTH_IDX] = len;
	msg_buf[len+RADIO_HEAD_LENGTH-1]=Get_Xor(msg_buf,(len+1));
}

/************************************************* 
@Description:读写器-消息发送
@Input:
@Output:
@Return:
*************************************************/ 
void Radio_MSG_Push(uint8_t* src)
{
	cmd_packet.packet[RADIO_S0_IDX] = RADIO_S0_DIR_DOWN;//S0下行
	my_memcpy(&cmd_packet.packet[TAG_ID_IDX],&src[TAG_ID_IDX],RADIO_ID_LENGTH);//2~5目标ID
	my_memcpy(&cmd_packet.packet[READER_ID_IDX],&DeviceID,RADIO_RID_LENGTH);//6~9读写器ID
	cmd_packet.packet[CMD_IDX] = MESSAGE_CMD;//消息命令
	Msg_Packet.MSG_PUSH_HEAD = 0;
	Msg_Packet.MSG_PUSH_HEAD |=MSG_HEAD_Msk;//数据指示，表示携带消息内容
	Msg_Packet.MSG_PUSH_HEAD |=	((Msg_Packet.MSG_PUSH_SEQ<<MSG_SEQ_Pos)&MSG_SEQ_Msk);//下发的消息序号
//	Msg_Packet.PKT_PUSH_NUM--;//发送成功再减一
	if(Msg_Packet.PKT_PUSH_NUM-1)//消息总数减一
	{
		Msg_Packet.MSG_PUSH_HEAD |= MSG_PKT_END_Msk;//还有包要发送
	}
	Msg_Packet.MSG_PUSH_HEAD |= (Msg_Packet.PKT_PUSH_SEQ&MSG_PKT_Seq_Msk);//包序号
	//消息头
	cmd_packet.packet[MSG_HEAD_IDX] = Msg_Packet.MSG_PUSH_HEAD;
	//拷贝消息数据
	my_memcpy(&cmd_packet.packet[MSG_DATA_IDX],&Msg_Packet.PKT_PUSH_BUF[Msg_Packet.PKT_PUSH_SEQ][0],
				Msg_Packet.PKT_PUSH_LEN[Msg_Packet.PKT_PUSH_SEQ]);
	//点对点固定长度+消息头+消息数据长度
	cmd_packet.length = CMD_FIX_LENGTH + MSG_HEAD_LEN + Msg_Packet.PKT_PUSH_LEN[Msg_Packet.PKT_PUSH_SEQ];
	//射频长度
	cmd_packet.packet[RADIO_LENGTH_IDX] = cmd_packet.length ;
	//异或
	cmd_packet.packet[cmd_packet.length+RADIO_HEAD_LENGTH-1]=Get_Xor(cmd_packet.packet,(cmd_packet.length+1));
	//包++
//	Msg_Packet.PKT_PUSH_SEQ++;//发送成功再加1
}

