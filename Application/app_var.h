#ifndef _app_var_
#define _app_var_
#include "sys.h"
#include "app_radio.h"
//��ǩID�洢��ַ
#define 	ID_BEGIN	0x3D000					//1KB,Ӳ����Ϣ��,ǰ4�ֽڷ�ID��
//��д���ڲ�����
typedef struct
{
	uint8_t tx_pwr;//���书��
	//��Ƶ���ڷ���
	uint8_t radio_send_en;//���ڷ���
	uint16_t radio_cycle_time;//���ڼ��  1����ʾ50ms 
	uint16_t radio_time_cnt;//������
	uint8_t radio_time_cnt_en;//����ʹ��
}Para_Typedef;

#define para_area 1
#define reserved_area 2
#define user_area1 3
#define user_area2 4

//flash����,����洢������С����mcu�ͺž���.
/*FICR�Ĵ����е�CODEPAGESIZE��Ӧ��ҳ������CODESIZE��Ӧҳ������memory��С
CODEPAGESIZE*CODESIZE��ΪROM�Ĵ�С,pg_size=1024,pg_num = 256,256KB�Ĵ���洢����FLASH��
*/
typedef struct
{
	uint32_t MARK_BASE;
	uint32_t PARA_BASE;
	uint32_t RESERVER_BASE;
	uint32_t USER1_BASE;
	uint32_t USER2_BASE;
	uint16_t page_size;
	uint16_t page_num;
}ROM_BaseAddr_Typedef;   //ROM����ַ����


//�ڲ�����������
#define PARA_RECORD_LEN									16
//�ڲ�FLASH������
#define RAM_TYPE_PARA									0
#define RAM_TYPE_RESEVER								1
#define RAM_TYPE_USER1									2
#define RAM_TYPE_USER2									3

//��ƵƵ���ͷ���ѡ��
#define DATA_CHANNEL									0
#define CONFIG_CHANNEL									1
#define RADIO_RX										0
#define RADIO_TX										1
//Ĭ����ƵƵ��
#define RADIO_ADDRESS_H 0XE8
#define RADIO_ADDRESS_L 0X98DB6D5A
#define RADIO_CHANNEL_DATA						25
#define RADIO_CHANNEL_CONFIG					50

//PACKET-S0
#define RADIO_S0_IDX 0
#define RADIO_S0_DIR_UP 0 //���� 
#define RADIO_S0_DIR_DOWN 1 //����
#define RADIO_S0_DIR_POS 0
#define RADIO_S0_DIR_Msk 0X01
//PACKET-LENGTH
#define RADIO_LENGTH_IDX 1
#define RADIO_HEAD_LENGTH 2
/**************************************************************************
							��ǩ�ϱ���������
******************************************************************************/
//PACKET-PAYLOAD-XOR
#define PYLOAD_XOR_LENGTH 1

//PACKET-PAYLOAD
#define PAYLOAD_BASE_IDX RADIO_HEAD_LENGTH

//����
#define RADIO_TID_LENGTH 4
#define RADIO_RID_LENGTH 4
#define RADIO_ID_LENGTH	4
#define RADIO_CMD_LENGTH 1

//������
//#define TAG_SER_IDX                     0//˳���
#define TAG_ID_IDX										PAYLOAD_BASE_IDX//��ǩID 2~5
#define TAG_STATE_IDX 									(PAYLOAD_BASE_IDX+RADIO_TID_LENGTH)//״̬��
//#define TAG_WINDOWS_IDX								6//���մ���ָʾ
//#define TAG_JOINTYPE_IDX								6//��̬����ָʾ
//#define TAG_ERROR_IDX									6//�����쳣ָʾ
//#define TAG_KEY_IDX									6//����ָʾ
//#define TAG_SHOCK_IDX									6//��ָʾ
#define TAG_VERSION_IDX									(PAYLOAD_BASE_IDX+5)//�汾������
#define TAG_STYPE_IDX									(PAYLOAD_BASE_IDX+6)//��Ϣ��ǩ����
#define TAG_SDATA_IDX									(PAYLOAD_BASE_IDX+7)//��������

//ֵ
//ȡֵ
//˳���,0~15��̬��16��̬
//#define TAG_SER_Pos											0
//#define TAG_SER_Msk											0xFF


//��ѹָʾ��1-��ѹ
#define TAG_LOWPWR_Pos									0
#define TAG_KEY_Pos										1
#define TAG_WITHSENSOR_Pos								2
#define TAG_WITHWIN_Pos									3
#define TAG_WIHTWIN_Msk									0x08
#define TAG_MODE_Pos									4
#define TAG_TIMEUPDATE_Pos								5
#define TAG_TIMEUPDATE_Msk								0x20
//�汾��Ϣ 
#define TAG_HDVERSION_POS								4
#define TAG_SFVERSION_POS								0
#define TAG_HDVER_NUM									1
#define TAG_SFVER_NUM									1
//��������
#define TAG_SENSORTYPE_Pos								0
#define TAG_SENSORTYPE_SchoolWatch 			(1 << TAG_SENSORTYPE_Pos)
//��������
#define TAG_MSEQ_Pos 									4
/********************************************
					��ǩ�ڲ�����
********************************************/
#define TAGP_BRIEFNUM_IDX								0//�̺ţ�0��16
#define P_PWR_IDX									1//���书�ʣ�0��7
#define TAGP_RPINFOSRC_IDX								2//�Զ��ϱ�����Я������Ϣ��Դ��0��2
#define TAGP_WORKMODE_IDX								3//����ģʽ��0-����ģʽ��1-�ģʽ
//#define TAGP_DELIVERID_IDX							9//����ID��4B��0xFFFFFFFF-��д��
//#define TAGP_SENSORTYPE_IDX							13//�������ͣ�0-�ޣ�1-�¶ȣ�2-���ʣ�0��2
//#define TAGP_SSRUNIT_IDX								14//���в������ڵ�λ��0-�룬1-�֣�2-ʱ
//#define TAGP_SSRVALUE_IDX								14
/*�ڲ�����ȡֵ*/
//�̺�
#define TAGP_BRIEFNUM_MAX_VALUE							16
//���书�ʣ�0��7��0--30dbm��1--20dbm��2--16dbm,3--12dbm,4--8dbm,5--4dbm,6--0dbm,7-+4dbm
#define P_PWR_Pos									4
#define P_PWR_Msk									0xF0
#define P_PWR_N30DBM									0
#define P_PWR_N20DBM									1
#define P_PWR_N16DBM									2
#define P_PWR_N12DBM									3
#define P_PWR_N8DBM									4
#define P_PWR_N4DBM									5
#define P_PWR_P0DBM									6
#define P_PWR_P4DBM									7
#define P_PWR_MAX_VALUE								7  //�������ֵ

//����ģʽ,0-����ģʽ��1-�ģʽ
#define TAGP_WORKMODE_Pos								0
#define TAGP_WORKMODE_Msk								0x0f
#define TAGP_WORKMODE_MAX_VALUE							0x01
 
//��д��
#define READER_ID_IDX									(PAYLOAD_BASE_IDX + RADIO_TID_LENGTH) 

//��ǩ��¼
#define Sensor_Data_Length 1
#define CAPACITY 200	//��ǩ���� 
typedef struct
{
	uint8_t TID[4];
	uint8_t State;
	uint8_t Sensor_Type;
	uint8_t Sensor_Data[Sensor_Data_Length];//�������ݣ�ָʾ��Ϣ����
}TID_Typedef;

//#define TAG_LOWVOLTAGE_Pos							0
//#define TAG_LOWVOLTAGE_Msk							0x01
//#define TAG_LOWVOLTAGE_WARN							0x01
//#define TAG_LOWVOLTAGE_NORMAL						0x00
/********************************************
					��д���ڲ�����
********************************************/
#define READERP_SENDEN_IDX              1//���ڷ���
#define READERP_SENDTIME_IDX 			1//���ͼ��


#define READERP_SENDEN_Pos				3
#define READERP_SENDEN_Msk				0x08
#define READERP_SENDEN_MAX_VALUE		0x01

#define READERP_SENDTIME_Pos			0
#define READERP_SENDTIME_Msk			0x07
#define READERP_SENDTIME_MAX_VALUE		0x07

#define ID_TAP_REGION_First							0x00000001//��ǩID
#define ID_TAP_REGION_Last							0xFEFFFFFF
#define ID_BROADCAST_TAG             				0xFFFFFFFE//�㲥id  MSB�͵�ַ ʵ��ID 0xFEFFFFFF
#define ID_RECEIVER_REGION_First					0xFF000000//������/��д��ID
#define ID_RECEIVER_REGION_Last						0xFFFDFFFF
#define ID_TRANSCEIVER_REGION_First					0xFFFE0000//����������(����)
#define ID_TRANSCEIVER_REGION_Last					0xFFFEFFFF
#define ID_RESERVER_REGION_First					0xFFFF0000//����
#define ID_RESERVER_REGION_Last						0xFFFFFFFA
#define ID_SELF_RP									0xFFFFFFFB//����ID�����ڷ������������ʹ�ܻظ�(ֻ��UART�˳���)
#define ID_BC_TAP_RP								0xFFFFFFFC//�㲥ID��Ŀ�����б�ǩ��ʹ�ܻظ�
#define ID_BC_TAP_NRP								0xFFFFFFFD//�㲥ID��Ŀ�����б�ǩ����ֹ�ظ�
#define ID_BROADCAST_READER_RP						0xFFFFFFFE//�㲥ID��Ŀ�����н�������ʹ�ܻظ�
#define ID_RESERVER1								0xFFFFFFFF//����
#define ID_RESERVER0								0x00000000//����

#define READER_ID_MBYTE							0XFF


/***************************************************************************
						����
����	S0 		max_length		TID  	RID		CMD
����λ��0		1				2		6		10
**************************************************************************/
/********************************************************************
						�ڲ��ļ���flash������
��д���·��ļ�����
����		���� 	ģʽ		ƫ��  	����	
����λ��	11		13			14		16				

��ǩ������ظ�
����		ִ��״̬2�ֽ�	����
����λ��	11				13

��ǩ����д�ظ�
����		ִ��״̬2�ֽ�
����λ��	11
******************************************************************/
//ִ��״̬
#define EXCUTE_STATE_LENGTH	2
//��Ե�����̶�����
#define CMD_FIX_LENGTH 		(RADIO_TID_LENGTH+RADIO_RID_LENGTH+RADIO_CMD_LENGTH+PYLOAD_XOR_LENGTH)
//����ظ��̶�����
#define CMD_ACK_FIX_LENGTH 		(CMD_FIX_LENGTH+EXCUTE_STATE_LENGTH)//
//����������
#define CMD_IDX				(RADIO_HEAD_LENGTH + RADIO_TID_LENGTH + RADIO_RID_LENGTH)
//�ļ�����������
#define FILE_MODE_IDX (CMD_IDX+3)
#define FILE_OFFSET_IDX (CMD_IDX+4)
#define FILE_LENGTH_IDX (CMD_IDX+6)
#define FILE_WDATA_IDX (CMD_IDX+7)			

//����
#define FILE_CMD_READ  								0X38  	//���ļ�
#define FILE_CMD_WRITE								0X39  	//д�ļ�
//ģʽ
#define FILE_MODE_PARA								0X01	//�ڲ�������
#define FILE_MODE_RESERVER							0X02	//������
#define FILE_MODE_USER1								0X03	//�û���1
#define FILE_MODE_USER2								0X04	//�û���2
//��ȡ���¼�¼
#define FILE_OFFSET_RNEW							0XFFFF 	//��ȡ���¼�¼
//����
#define FILE_RDATA_IDX		13
//дƫ��
// 0xffffд���¼�¼������¼��ʱ���������м�¼
// 0xfffeд���¼�¼������¼��ʱ����������¼
#define FILE_OFFSET_WNEW	0XFFFF
/******************************************************************
						ִ��״̬
					��������ʽ2�ֽ�
				���������� �����Ӵ���
******************************************************************/
#define EXCUTE_STATE_IDX 	11

typedef enum
{
	FILE_ERR = 0X06
}ERROR_H_Typedef;

typedef enum 
{
	FILE_MODE_ERR=0X00,//������������
	FILE_BODER_ERR=0X01,//�����߽磬����/��ƫ��������
	FILE_WOFFSET_ERR=0X02,//дƫ�ƴ���
	FILE_WDATA_ERR=0X03    //���ݴ���
}ERROR_L_Typedef;
#define CMD_RUN_SUCCESS 0X0000//����ִ�гɹ�

typedef struct
{
	uint8_t mode;//�ļ�����ģʽ
	uint8_t length;//��ȡ����
	uint16_t offset;//ƫ��λ��
}File_Typedef;

//��Ϣ����
#define MESSAGE_CMD									0XB0	//��Ϣ����
typedef struct
{
	uint8_t msg_pkt_seq;	//�����
//	uint8_t has_data_flag;  //������
	uint8_t msg_head;		//��Ϣͷ
	uint8_t msg_pkt_len;	//�·���һ�����е���Ϣ����
	uint8_t msg_idx;		//��Ϣ����
	uint8_t MSG_PUSH_HEAD;  //�·��İ�ͷ
	uint8_t PKT_PUSH_LEN[4];//��0~3����
	uint8_t PKT_PUSH_BUF[4][32];//��0~3����
	uint8_t PKT_PUSH_NUM;//�ְ�����
	uint8_t PKT_PUSH_SEQ;//�ְ����
	uint8_t MSG_PUSH_SEQ;//�·�����Ϣ���	
	uint8_t PKT_MORE;//�����
	uint8_t MSG_RE_PUSH;//�ط�����
	uint8_t MSG_PUSH_RID[4];//������ID
	uint8_t MSG_PUSH_TID[4];//��ǩID
}Message_Typedef;
//��Ϣ����������
#define MSG_HEAD_IDX (CMD_IDX+1)
#define MSG_DATA_IDX  (CMD_IDX+2)

#define MSG_HEAD_Pos								7
#define MSG_HEAD_Msk								0X80
#define MSG_SEQ_Pos									4
#define MSG_SEQ_Msk									0X70
#define MSG_SEQ_Max_Value							7
#define MSG_PKT_Seq_Pos								0
#define MSG_PKT_Seq_Msk								0x03
#define MSG_PKT_END_Pos								2
#define MSG_PKT_END_Msk								0x04
#define MSG_END_Pos									3
#define MSG_END_Msk									0x08
#define MSG_PAYLOAD_FIX_LEN (RADIO_TID_LENGTH+RADIO_RID_LENGTH+RADIO_CMD_LENGTH+PYLOAD_XOR_LENGTH)
#define MSG_ERROR_Msk								0xff
#define MSG_START_END_VALUE							0X0000
#define MSG_START_END_LEN							2
#define MSG_HEAD_LEN								1
#define MSG_RPUSH_TIMES								1
typedef enum 
{
	MSG_SUCCESS=0X00,
	MSG_PKT_SEQ_ERROR=0X02,//����ų���
	MSG_REPEAT_ERROR=0X01,//��Ϣ�ظ�
	MSG_CMDPARA_ERROR = 0X03, //�����������
	MSG_ERROR =0X04
}ERROR_MSG_Typedef;
//ʱ������
#define TIME_PARA_LEN							4
#define TIME_SET_CMD							0XB1	//ʱ����������
//����
void SystemParaInit(void);
void UpdateRunPara(void);
uint8_t Read_Para(File_Typedef f1_para,uint8_t *p_packet);
uint8_t Write_Para(File_Typedef f1_para,uint8_t *p_packet);
#endif
