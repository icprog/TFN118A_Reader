#ifndef _APP_MSG
#define _APP_MSG
#include "sys.h"

/**********************************************
ÿ���洢����Ϣ�洢��ʽ
��Ϣ���+��Ϣ����+��Ϣ����
��Ϣ���ȡֵ��Χ0~7
��Ϣ����ȡֵ��Χ0~128
**********************************************/
#define MSG_IDX_MAX_Value 2
#define MSG_SEQ_IDX   0
#define MSG_LEN_IDX   1
#define MSG_STORE_IDX 2
#define MSG_PKT_MAX_LEN 130 //4����ÿ�����32�ֽڣ���Ϣͷ2�ֽ�
#define MSG_MAX_LEN 128
#define MSG_FLASH_HEAD_LEN 2
#define MSG_SEQ_MAX_NUM 8   //��Ϣ���Ϊ0~7�����Ϊ8
#define MSG_FLASH_NUM 3		//���洢3����Ϣ
#define MSG_PACKET_MAX_VALUE 32   //��Ƶ�·�һ����Ϣ���ֵ
#define MSG_PACKET_OFFSET    5  //��Ϣ����/32

//��Ϣ������
typedef enum
{
	MSG1_IDX=0,
	MSG2_IDX=1,
	MSG3_IDX=2
}MSG_IDX_Typedef;
//��Ϣ״̬
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
	uint32_t MSG1_ROM;//��Ϣ1��ַ
	uint32_t MSG2_ROM;//��Ϣ2��ַ
	uint32_t MSG3_ROM;//��Ϣ3��ַ
	uint32_t NEW_MSG_ROM;//ָʾ������Ϣ
	uint32_t *MSG_ADDR[3];
	uint8_t MSG_BUFF[MSG_PKT_MAX_LEN];	//��Ϣ����ֽ�
	uint8_t MSG_BUFF_IDX;//����������
	uint8_t MSG_PUSH[MSG_PKT_MAX_LEN];  //�洢Ҫ�·�����Ϣ
	uint8_t MSG_Seq;	//��Ϣ��ţ����´�������
	uint8_t MSG_Len;//һ����Ϣ����
	uint8_t MSG_Num; //��Ϣ����
	uint8_t MSG_IDX; //��Ϣ����0~2

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

