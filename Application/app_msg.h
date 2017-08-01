#ifndef _APP_MSG
#define _APP_MSG
#include "sys.h"

/**********************************************
每个存储区消息存储格式
消息序号+消息长度+消息内容
消息序号取值范围0~7
消息长度取值范围0~128
**********************************************/
#define MSG_IDX_MAX_Value 2
#define MSG_SEQ_IDX   0
#define MSG_LEN_IDX   1
#define MSG_STORE_IDX 2
#define MSG_PKT_MAX_LEN 130 //4包，每包最大32字节，消息头2字节
#define MSG_MAX_LEN 128
#define MSG_FLASH_HEAD_LEN 2
#define MSG_SEQ_MAX_NUM 8   //消息序号为0~7，因此为8
#define MSG_FLASH_NUM 3		//最多存储3条消息
#define MSG_PACKET_MAX_VALUE 32   //射频下发一包消息最大值
#define MSG_PACKET_OFFSET    5  //消息长度/32

//消息索引号
typedef enum
{
	MSG1_IDX=0,
	MSG2_IDX=1,
	MSG3_IDX=2
}MSG_IDX_Typedef;
//消息状态
typedef enum
{
	MSG_IDLE=0,
	MSG_WAIT=1,
	MSG_Packet0=2,
	MSG_Packet1=3,
	MSG_Packet2=4,
	MSG_Packet3=5,
	MSG_End=6
}MSG_PUSH_STATE_Typedef;

typedef struct
{
	uint32_t MSG1_ROM;//消息1地址
	uint32_t MSG2_ROM;//消息2地址
	uint32_t MSG3_ROM;//消息3地址
	uint32_t NEW_MSG_ROM;//指示最新消息
	uint32_t *MSG_ADDR[3];
	uint8_t MSG_BUFF[MSG_PKT_MAX_LEN];	//消息最大字节
	uint8_t MSG_BUFF_IDX;//缓冲索引号
	uint8_t MSG_PUSH[MSG_PKT_MAX_LEN];  //存储要下发的消息
	uint8_t MSG_Seq;	//消息序号，更新传感数据
	uint8_t MSG_Len;//一条消息长度
	uint8_t MSG_Num; //消息数量
	uint8_t MSG_IDX; //消息索引0~2

}MSG_Store_Typedef;

u16 Message_Deal(uint8_t *p_mpacket);
void MSG_Addr_Init(void);
void MSG_Erase_ALL(void);
void MSG_Find_New(void);
void MSG_Write(uint8_t idx,u8* buff);
uint8_t Message_Get(uint8_t tag_msg_seq);
void Radio_MSG_Start(uint8_t *msg_buf,uint8_t* src);
void Radio_MSG_Push(uint8_t* src);
#endif

