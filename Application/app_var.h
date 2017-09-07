#ifndef _app_var_
#define _app_var_
#include "sys.h"
#include "app_radio.h"
//标签ID存储地址
#define 	ID_BEGIN	0x3D000					//1KB,硬件信息区,前4字节放ID号
//读写器内部参数
typedef struct
{
	uint8_t tx_pwr;//发射功率
	//射频周期发送
	uint8_t radio_send_en;//周期发送
	uint16_t radio_cycle_time;//周期间隔  1：表示50ms 
	uint16_t radio_time_cnt;//计数器
	uint8_t radio_time_cnt_en;//计数使能
}Para_Typedef;

#define para_area 1
#define reserved_area 2
#define user_area1 3
#define user_area2 4

//flash部分,代码存储区，大小根据mcu型号决定.
/*FICR寄存器中的CODEPAGESIZE对应着页个数，CODESIZE对应页包含的memory大小
CODEPAGESIZE*CODESIZE即为ROM的大小,pg_size=1024,pg_num = 256,256KB的代码存储区，FLASH。
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
}ROM_BaseAddr_Typedef;   //ROM基地址定义


//内部参数区长度
#define PARA_RECORD_LEN									16
//内部FLASH区类型
#define RAM_TYPE_PARA									0
#define RAM_TYPE_RESEVER								1
#define RAM_TYPE_USER1									2
#define RAM_TYPE_USER2									3

//射频频道和方向选择
#define DATA_CHANNEL									0
#define CONFIG_CHANNEL									1
#define RADIO_RX										0
#define RADIO_TX										1
//默认射频频道
#define RADIO_ADDRESS_H 0XE8
#define RADIO_ADDRESS_L 0X98DB6D5A
#define RADIO_CHANNEL_DATA						25
#define RADIO_CHANNEL_CONFIG					50

//PACKET-S0
#define RADIO_S0_IDX 0
#define RADIO_S0_DIR_UP 0 //上行 
#define RADIO_S0_DIR_DOWN 1 //下行
#define RADIO_S0_DIR_POS 0
#define RADIO_S0_DIR_Msk 0X01
//PACKET-LENGTH
#define RADIO_LENGTH_IDX 1
#define RADIO_HEAD_LENGTH 2
/**************************************************************************
							标签上报参数定义
******************************************************************************/
//PACKET-PAYLOAD-XOR
#define PYLOAD_XOR_LENGTH 1

//PACKET-PAYLOAD
#define PAYLOAD_BASE_IDX RADIO_HEAD_LENGTH

//长度
#define RADIO_TID_LENGTH 4
#define RADIO_RID_LENGTH 4
#define RADIO_ID_LENGTH	4
#define RADIO_CMD_LENGTH 1

//索引号
//#define TAG_SER_IDX                     0//顺序号
#define TAG_ID_IDX										PAYLOAD_BASE_IDX//标签ID 2~5
#define TAG_STATE_IDX 									(PAYLOAD_BASE_IDX+RADIO_TID_LENGTH)//状态字
//#define TAG_WINDOWS_IDX								6//接收窗口指示
//#define TAG_JOINTYPE_IDX								6//静态接入指示
//#define TAG_ERROR_IDX									6//数据异常指示
//#define TAG_KEY_IDX									6//按键指示
//#define TAG_SHOCK_IDX									6//振动指示
#define TAG_VERSION_IDX									(PAYLOAD_BASE_IDX+5)//版本索引号
#define TAG_STYPE_IDX									(PAYLOAD_BASE_IDX+6)//信息标签类型
#define TAG_SDATA_IDX									(PAYLOAD_BASE_IDX+7)//传感数据

//值
//取值
//顺序号,0~15静态，16动态
//#define TAG_SER_Pos											0
//#define TAG_SER_Msk											0xFF


//低压指示，1-低压
#define TAG_LOWPWR_Pos									0
#define TAG_LOWPWR_Msk									0x01
#define TAG_KEY_Pos										1
#define TAG_KEY_Msk										0x02
#define TAG_WITHSENSOR_Pos								2
#define TAG_WITHSENSOR_Msk								0x04
#define TAG_WITHWIN_Pos									3
#define TAG_WIHTWIN_Msk									0x08
#define TAG_MODE_Pos									4
#define TAG_MODE_Msk									0x10
#define TAG_TIMEUPDATE_Pos								5
#define TAG_TIMEUPDATE_Msk 								0x20
//版本信息 
#define TAG_HDVERSION_POS								4
#define TAG_SFVERSION_POS								0
#define TAG_HDVER_NUM									1
#define TAG_SFVER_NUM									1
//传感类型
#define TAG_SENSORTYPE_Pos								0
#define TAG_SENSORTYPE_SchoolWatch 			(1 << TAG_SENSORTYPE_Pos)
//传感数据
#define TAG_MSEQ_Pos 									4
/********************************************
					标签内部参数
********************************************/
#define TAGP_BRIEFNUM_IDX								0//短号，0～16
#define P_PWR_IDX									1//发射功率，0～7
#define TAGP_RPINFOSRC_IDX								2//自动上报所需携带的信息来源，0～2
#define TAGP_WORKMODE_IDX								3//工作模式，0-保存模式，1-活动模式
//#define TAGP_DELIVERID_IDX							9//工厂ID，4B，0xFFFFFFFF-待写入
//#define TAGP_SENSORTYPE_IDX							13//传感类型，0-无，1-温度，2-心率，0～2
//#define TAGP_SSRUNIT_IDX								14//传感采样周期单位，0-秒，1-分，2-时
//#define TAGP_SSRVALUE_IDX								14
/*内部参数取值*/
//短号
#define TAGP_BRIEFNUM_MAX_VALUE							16
//发射功率，0～7；0--30dbm，1--20dbm，2--16dbm,3--12dbm,4--8dbm,5--4dbm,6--0dbm,7-+4dbm
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
#define P_PWR_MAX_VALUE								7  //参数最大值

//工作模式,0-保存模式，1-活动模式
#define TAGP_WORKMODE_Pos								0
#define TAGP_WORKMODE_Msk								0x0f
#define TAGP_WORKMODE_MAX_VALUE							0x01
 
//读写器
#define READER_ID_IDX									(PAYLOAD_BASE_IDX + RADIO_TID_LENGTH) 

#define READER_UP_ID_IDX										PAYLOAD_BASE_IDX//标签ID 2~5
#define READER_UP_STATE_IDX 									(PAYLOAD_BASE_IDX+RADIO_TID_LENGTH)//状态字
#define READER_UP_VERSION_IDX									(PAYLOAD_BASE_IDX+5)//版本索引号
#define READER_UP_STYPE_IDX									(PAYLOAD_BASE_IDX+6)//信息标签类型
#define READER_UP_SDATA_IDX									(PAYLOAD_BASE_IDX+7)//传感数据
//传感类型
#define READER_UP_SENSORTYPE_Pos								0
#define READER_UP_SENSORTYPE_SchoolWatch 			(1 << READER_UP_SENSORTYPE_Pos)


//标签记录
#define Sensor_Data_Length 3
#define CAPACITY 200	//标签容量 
typedef struct
{
	uint8_t TID[4];//标签ID
	uint8_t State;//标签状态
	uint8_t RSSI;//信号强度
	uint8_t LeaveTime;//离开时间
	uint8_t VER;//版本信息	
	uint8_t Sensor_Type;//传感类型
	uint8_t Sensor_Data[Sensor_Data_Length];//传感数据，指示消息类型
	
}TID_Typedef;

//#define TAG_LOWVOLTAGE_Pos							0
//#define TAG_LOWVOLTAGE_Msk							0x01
//#define TAG_LOWVOLTAGE_WARN							0x01
//#define TAG_LOWVOLTAGE_NORMAL						0x00
/********************************************
					读写器内部参数
********************************************/
#define READERP_SENDEN_IDX              1//周期发送
#define READERP_SENDTIME_IDX 			1//发送间隔


#define READERP_SENDEN_Pos				3
#define READERP_SENDEN_Msk				0x08
#define READERP_SENDEN_MAX_VALUE		0x01

#define READERP_SENDTIME_Pos			0
#define READERP_SENDTIME_Msk			0x07
#define READERP_SENDTIME_MAX_VALUE		0x07

#define ID_TAP_REGION_First							0x00000001//标签ID
#define ID_TAP_REGION_Last							0xFEFFFFFF
#define ID_BROADCAST_TAG             				0xFFFFFFFE//广播id  MSB低地址 实际ID 0xFEFFFFFF
#define ID_RECEIVER_REGION_First					0xFF000000//接收器/读写器ID
#define ID_RECEIVER_REGION_Last						0xFFFDFFFF
#define ID_TRANSCEIVER_REGION_First					0xFFFE0000//做发卡器用(桌面)
#define ID_TRANSCEIVER_REGION_Last					0xFFFEFFFF
#define ID_RESERVER_REGION_First					0xFFFF0000//保留
#define ID_RESERVER_REGION_Last						0xFFFFFFFA
#define ID_SELF_RP									0xFFFFFFFB//本机ID，用于访问自身参数，使能回复(只在UART端出现)
#define ID_BC_TAP_RP								0xFFFFFFFC//广播ID，目标所有标签，使能回复
#define ID_BC_TAP_NRP								0xFFFFFFFD//广播ID，目标所有标签，禁止回复
#define ID_BROADCAST_READER_RP						0xFFFFFFFE//广播ID，目标所有接收器，使能回复
#define ID_RESERVER1								0xFFFFFFFF//保留
#define ID_RESERVER0								0x00000000//保留

#define READER_ID_MBYTE							0XFF


/***************************************************************************
						命令
定义	S0 		max_length		TID  	RID		CMD
数组位置0		1				2		6		10
**************************************************************************/
/********************************************************************
						内部文件（flash）操作
读写器下发文件命令
定义		保留 	模式		偏移  	长度	
数组位置	11		13			14		16				

标签命令读回复
定义		执行状态2字节	数据
数组位置	11				13

标签命令写回复
定义		执行状态2字节
数组位置	11
******************************************************************/
//执行状态
#define EXCUTE_STATE_LENGTH	2
//点对点命令固定长度
#define CMD_FIX_LENGTH 		(RADIO_TID_LENGTH+RADIO_RID_LENGTH+RADIO_CMD_LENGTH+PYLOAD_XOR_LENGTH)
//命令回复固定长度
#define CMD_ACK_FIX_LENGTH 		(CMD_FIX_LENGTH+EXCUTE_STATE_LENGTH)//
//命令索引号
#define CMD_IDX				(RADIO_HEAD_LENGTH + RADIO_TID_LENGTH + RADIO_RID_LENGTH)
//文件操作索引号
#define FILE_MODE_IDX (CMD_IDX+3)
#define FILE_OFFSET_IDX (CMD_IDX+4)
#define FILE_LENGTH_IDX (CMD_IDX+6)
#define FILE_WDATA_IDX (CMD_IDX+7)			

//命令
#define FILE_CMD_READ  								0X38  	//读文件
#define FILE_CMD_WRITE								0X39  	//写文件
//模式
#define FILE_MODE_PARA								0X01	//内部参数区
#define FILE_MODE_RESERVER							0X02	//保留区
#define FILE_MODE_USER1								0X03	//用户区1
#define FILE_MODE_USER2								0X04	//用户区2
//读取最新记录
#define FILE_OFFSET_RNEW							0XFFFF 	//读取最新记录
//数据
#define FILE_RDATA_IDX		13
//写偏移
// 0xffff写最新记录，当记录满时，擦除所有记录
// 0xfffe写最新记录，当记录满时，不擦除记录
#define FILE_OFFSET_WNEW	0XFFFF
/******************************************************************
						执行状态
					出错代码格式2字节
				出错类别代码 出错子代码
******************************************************************/
#define EXCUTE_STATE_IDX 	11

typedef enum
{
	FILE_ERR = 0X06
}ERROR_H_Typedef;

typedef enum 
{
	FILE_MODE_ERR=0X00,//操作区不存在
	FILE_BODER_ERR=0X01,//超出边界，长度/读偏移量超出
	FILE_WOFFSET_ERR=0X02,//写偏移错误
	FILE_WDATA_ERR=0X03    //数据错误
}ERROR_L_Typedef;
#define CMD_RUN_SUCCESS 0X0000//命令执行成功

typedef struct
{
	uint8_t mode;//文件操作模式
	uint8_t length;//读取长度
	uint16_t offset;//偏移位置
}File_Typedef;

//消息操作
#define MESSAGE_CMD									0XB0	//消息命令
typedef struct
{
	uint8_t msg_pkt_seq;	//包序号
//	uint8_t has_data_flag;  //有数据
	uint8_t msg_head;		//消息头
	uint8_t msg_pkt_len;	//下发的一个包中的信息长度
	uint8_t msg_idx;		//消息索引
	uint8_t MSG_PUSH_HEAD;  //下发的包头
	uint8_t PKT_PUSH_LEN[4];//包0~3长度
	uint8_t PKT_PUSH_BUF[4][32];//包0~3数据
	uint8_t PKT_PUSH_NUM;//分包个数
	uint8_t PKT_PUSH_SEQ;//分包序号
	uint8_t MSG_PUSH_SEQ;//下发的消息序号	
	uint8_t PKT_MORE;//更多包
	uint8_t MSG_RE_PUSH;//重发次数
	uint8_t MSG_PUSH_RID[4];//接收器ID
	uint8_t MSG_PUSH_TID[4];//标签ID
}Message_Typedef;
//消息操作索引号
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
	MSG_PKT_SEQ_ERROR=0X02,//包序号出错
	MSG_REPEAT_ERROR=0X01,//消息重复
	MSG_CMDPARA_ERROR = 0X03, //命令参数错误
	MSG_ERROR =0X04
}ERROR_MSG_Typedef;
//时间设置
#define TIME_PARA_LEN							4
#define TIME_SET_CMD							0XB1	//时间设置命令
#define DEVICE_TEST_CMD 						0XB2  //整机测试命令
#define ALARM_CLEAR_CMD							0XB3  //命令清除
//函数
void SystemParaInit(void);
void UpdateRunPara(void);
uint8_t Read_Para(File_Typedef f1_para,uint8_t *p_packet);
uint8_t Write_Para(File_Typedef f1_para,uint8_t *p_packet);
#endif
