/*******************************************************************************
** ��Ȩ:		
** �ļ���: 		app_reader.c
** �汾��  		1.0
** ��������: 	MDK-ARM 5.23
** ����: 		cc
** ��������: 	2017-07-26
** ����:		  
** ����ļ�:	app_key.h
** �޸���־��	
** ��Ȩ����   
*******************************************************************************/
#include "app_reader.h"
#include "sys.h"
#include "app_var.h"
#include "app_uart.h"
#include "simple_uart.h"
#include "crc16.h"
#include "app_radio.h"
#include "rtc.h"
extern Payload_Typedef cmd_packet;//������Ƶ����
extern uint8_t radio_run_channel;//��Ƶ����ͨ�� 
extern TID_Typedef  TID_RECORD[CAPACITY];
uint8_t Work_Mode = Idle;//����ģʽ
uint8_t tx_buf[254];//���ڷ���
extern UART_Typedef U_Master;//���崮�ڻ���
extern uint8_t Hour;//һ��Сʱ����ֹͣʱ����¹���
extern rtc_typedef Global_Time;
//�豸ID
extern uint8_t DeviceID[4];
void list_tag(void)
{
	
}
/************************************************* 
@Description:���ݴ�����������ڲ�ͬ��ģʽ��
@Input:��
@Output:��
@Return:��
*************************************************/ 
void app_process(void)
{
	uint8_t info_len;
	uint16_t crc;
	uint16_t crc_idx;
	switch(Work_Mode)
	{
		case Idle:
			if(RADIO_RUN_DATA_CHANNEL!= radio_run_channel)
			{
				radio_select(DATA_CHANNEL,RADIO_RX);
			}
			Stop_Update_Time();
			break;
		case List_Tag:
			
			break;
		case List_Reader:
			break;
		case File_Deal:
			if(U_Master.tx_en)
			{
				U_Master.tx_en = 0;
				U_Master.tx_buf[U_HEADER_IDX] = pkt_head1;
				U_Master.tx_buf[U_HEADER_IDX+1] = pkt_head2;
				info_len = (uint8_t)(cmd_packet.packet[RADIO_LENGTH_IDX] - CMD_FIX_LENGTH);//��Ϣ����
				U_Master.len = info_len + U_AfterLEN_FIX_LEN;//��Ϣ����+����Э�鳤�Ⱥ���Ĺ̶�����
				U_Master.tx_buf[U_LEN_IDX] =  U_Master.len>>8;
				U_Master.tx_buf[U_LEN_IDX+1] =  U_Master.len;
				U_Master.tx_buf[U_PROTOCOL_IDX] = U_PROTOCOL_VER;//Э��
				my_memcpy(&U_Master.tx_buf[U_ID_IDX],DeviceID,RADIO_ID_LENGTH);//��д��ID
				U_Master.tx_buf[U_TXGPS_IDX] = U_TXGPS_Value;//��λ��Ϣ
				U_Master.tx_buf[U_SEQ_IDX] = U_SEQ_Value;//��ˮ��
				if(cmd_packet.packet[CMD_IDX] ==FILE_CMD_READ)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_READ_FILE;
				else if(cmd_packet.packet[CMD_IDX] ==FILE_CMD_WRITE)
					U_Master.tx_buf[U_CMD_IDX] = U_CMD_WRITE_FILE;
				my_memcpy(&U_Master.tx_buf[U_DATA_IDX],&cmd_packet.packet[EXCUTE_STATE_IDX],info_len);//��д��ID
				U_Master.len = ((U_Master.tx_buf[U_LEN_IDX]<<8)|U_Master.tx_buf[U_LEN_IDX+1]);//����Э�������ݳ��ȣ�Э��汾~��Ϣ����
				crc = crc16(&U_Master.tx_buf[U_LEN_IDX],(U_Master.len+U_LENTH_LEN));//����~��Ϣ����
				crc_idx = U_HEAD_LEN + U_LENTH_LEN + U_Master.len; //֡ͷ����+���ȳ���+���Ⱥ���ĳ���
				U_Master.tx_buf[crc_idx] = (crc>>8);
				U_Master.tx_buf[crc_idx+1] = crc;//crc
				U_Master.len = U_Master.len + U_FIX_LEN;//֡ͷ2+����2+У��2+���ݳ���
				UART_Send(U_Master.tx_buf,U_Master.len);
				Work_Mode = Idle;
			}
			break;
		case Tag_Report:
			break;
	}
}
