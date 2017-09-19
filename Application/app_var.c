#include "app_var.h"
#include "nrf_nvmc.h"
#include "app_msg.h"
//设备ID
uint8_t DeviceID[RADIO_TID_LENGTH] = {0x33,0x33,0x33,0x33};



Tag_Mode_Typedef Tag_Mode;//标签模式
//按键定时器40ms
#define delay_interval   40  //40ms

//设备运行默认参数
const uint8_t  para_default[PARA_RECORD_LEN] = 
{
0x04,//2~0发射方式
0x02,//自动上报周期
0x06,//发射功率
0x00,//事件上报方式
0x20,//低频激活信号强度门限
0x04//报警时间
};
//存储内部参数 
uint8_t para_record[PARA_RECORD_LEN];
/********************************************
					自定义FLASH存储区
********************************************/
//MARK
const uint8_t nvmc_flash_mark[11]={0x54,0x46,0x4E,0x31,0x31,0x38,0x41,0x00,0x00,0x00,0x00};//TFN118A
const uint8_t Rom_Record_Offset[4] = {16,16,32,32};//4个扇区对应的最大偏移量,分别对应参数区（0~15）、保留区、用户区1、用户区2
const uint8_t Rom_Record_Length[4] = {16,16,16,16};//每条记录对应的字节数
ROM_BaseAddr_Typedef   ROM_BaseAddr;//ROM基地址定义
uint32_t nrf_addr;//临时缓存flash地址

//命令
Radio_Work_Mode_Typedef Radio_Work_Mode = Stand_Send;


/*
@Description:返回最新记录ROM位置
@Input:state : temp_addr存储区地址，temp_size存储区总记录个数，temp_byte每条记录长度
@Output:返回最新记录位置 0：表示无记录，空 ；>1 对应的最新记录
@Return:无
*/
uint8_t Rom_Pos(uint32_t temp_addr,uint8_t temp_size,uint8_t temp_byte)
{
	uint8_t i,j;
	uint32_t base_addr = temp_addr;
//	temp_addr = (uint8_t*)temp_addr;
	//验证buff是否为空，返回最新记录的位置
	//不相等，继续查找,找到空，则返回rom位置，i =  1~~Rom_record_size。返回0表示配置区全空	
	for(i=0;i<temp_size;)
	{
		for(j=0;j<temp_byte;j++)
		{
			if(*(uint8_t*)temp_addr++ != 0xff)
				break;
		}
		if(j>=temp_byte) break;
		i++;
		temp_addr= base_addr + temp_byte*i;
	}
	return i;
}


/****************************************
@Description:获取设备ID
@Input:
@Output:
@Return:无
****************************************/
void GetDeviceID(void)
{
	nrf_nvmc_read_bytes(ID_BEGIN,DeviceID,RADIO_TID_LENGTH);
}


/************************************************* 
@Description:更新发射模式
@Input:mode 0:单激活
@Output:无
@Return:无
*************************************************/ 
void UpdateSendMode(uint8_t mode)
{
	//发射模式：单激活模式、主动模式
	Tag_Mode.SendMode = mode;
}

/*
@Description:标签-运行内部参数
@Input:
@Output:
@Return:无
*/
void UpdateRunPara(void)
{

	//发射模式：单激活模式、主动模式
	Tag_Mode.SendMode = (para_record[TAGP_ACTIVESEND_IDX]&TAGP_ACTIVESEND_Msk) >> TAGP_ACTIVESEND_Pos;
	//1：自适应模式使能
	Tag_Mode.AdaptiveMode = (para_record[TAGP_ADAPTIVE_IDX]&TAGP_ADAPTIVE_Msk) >> TAGP_ADAPTIVE_Pos;
	//工作模式：活动，或保留 0:活动模式
	Tag_Mode.WorkMode = (para_record[TAGP_WORKMODE_IDX] & TAGP_WORKMODE_Msk) >> TAGP_WORKMODE_Pos;
	//自动发射周期，RTC基准时间250MS
	Tag_Mode.Send_Period = (para_record[TAGP_AUTOREPORT_IDX] & TAGP_AUTOREPORT_Msk) >> TAGP_AUTOREPORT_Pos;
	Tag_Mode.Send_Period = 1<<Tag_Mode.Send_Period;//1 2 4 8 16 32 64 128
	//发射功率
	if( Active_Mode == Tag_Mode.WorkMode)
		radio_pwr((para_record[TAGP_PWR_IDX] & TAGP_PWR_Msk) >> TAGP_PWR_Pos);
	else
		radio_pwr(P_PWR_N4DBM);	
	//事件清除方式
	Tag_Mode.AlarmClr_Mode = (para_record[TAGP_EVENTCLEAR_IDX] & TAGP_EVENTCLEAR_Msk) >> TAGP_EVENTCLEAR_Pos;
	Tag_Mode.THR_BASE = (para_record[TAGP_THRBASE_IDX] & TAGP_THRRSSI_Msk) >> TAGP_THRRSSI_Pos; 
	//报警延时
	Tag_Mode.Key_Alarm_Delay = ((para_record[TAGP_KEYALARM_IDX] & TAGP_KEYALARM_Msk) >> TAGP_KEYALARM_Pos)*1000/delay_interval;
}


/*********************************************************
@Description:文件读取
@Input: f1_para：指示模式、记录偏移、字节数
p_packet：射频命令指令
@Output:
@Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令		保留 			模式		偏移  	长度	
数组位置	CMD_IDX		CMD_PARA_IDX	+1			+2		+3	
模式：00：内部参数区 01：保留区 02用户区 03用户区
偏移量 0xff读取最新参数
标签应答
定义		命令代码1字节 	执行状态2字节 	数据
数组位置	CMD_IDX			CMD_PARA_IDX	+2
					
*********************************************************/
uint8_t Read_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint8_t max_length;
	uint16_t max_offset;//最大长度、最大偏移
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令执行状态
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA://内部参数区
		{
			max_offset = Rom_Record_Offset[0];
			max_length = Rom_Record_Length[0];	
			nrf_addr = ROM_BaseAddr.PARA_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.PARA_Pos;//最新记录
		}
		break;
		case FILE_MODE_RESERVER://保留区
		{
			max_offset = Rom_Record_Offset[1];
			max_length = Rom_Record_Length[1];
			nrf_addr = ROM_BaseAddr.RESERVER_BASE;
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.RESERVER_Pos;
		}
		break;
		case FILE_MODE_USER1://用户区1
		{
			max_offset = Rom_Record_Offset[2];
			max_length = Rom_Record_Length[2];			
			nrf_addr = ROM_BaseAddr.USER1_BASE;
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.USER1_Pos;
		}
		break;
		case FILE_MODE_USER2://用户区2
		{
			max_offset = Rom_Record_Offset[3];
			max_length = Rom_Record_Length[3];		
			nrf_addr = ROM_BaseAddr.USER2_BASE;
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.USER2_Pos;
		}
		break; 
		case FILE_MODE_RUNPARA://运行参数区
		{
			max_offset = 1;
			max_length = PARA_RECORD_LEN;
		}
		break;
		default:
		{
			cmd_state = (FILE_ERR <<8) | FILE_MODE_ERR;
		}
		break;
	}
	//长度和偏移量边界检查
	if(f1_para.length>max_length || (f1_para.offset>=max_offset && f1_para.offset<FILE_OFFSET_RNEW))
	{
		cmd_state = FILE_ERR << 8 | FILE_BODER_ERR;
	}
	//命令错误
	if(cmd_state!=CMD_RUN_SUCCESS)
	{
		p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
		p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
		return FALSE;
	}
	else
	{
		if(f1_para.mode == FILE_MODE_RUNPARA)
		{
			my_memcpy(&p_packet[EXCUTE_STATE_IDX+2],para_record,PARA_RECORD_LEN);
		}
		else
		{
			if(FILE_OFFSET_RNEW == f1_para.offset)//读取最新记录
			{
				#if 0
				if(0 == *ROM_BaseAddr.pROM_Pos)//记录为空
				{
					if(FILE_MODE_PARA == f1_para.mode)//参数区为空，返回运行参数
						my_memcpy(&p_packet[FILE_RDATA_IDX],para_record,f1_para.length);
				}
				else
				{
				#endif
					nrf_addr += (*ROM_BaseAddr.pROM_Pos-1)*max_length;//偏移量*长度
				#if 0
				}
				#endif
			}
			else
			{
				nrf_addr += f1_para.offset*max_length;			
			}
			nrf_nvmc_read_bytes(nrf_addr, &p_packet[FILE_RDATA_IDX],f1_para.length);			
		}

	}
	p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
	p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
	return TRUE;
}
/*********************************************************
@Description:参数检查
@Input:state : 
@Output:
@Return:无
**********************************************************/
uint8_t para_check(uint8_t mode,uint8_t *pdata)
{
	switch(mode)
	{
		case FILE_MODE_PARA://内部参数区
		case FILE_MODE_RUNPARA://运行参数
		{
			if(pdata[0] > PARA_BYTE0_MAX)
			{
				return FALSE;
			}
			if(pdata[1] > PARA_BYTE1_MAX)
			{
				return FALSE;
			}
			if(pdata[2] > PARA_BYTE2_MAX)
			{
				return FALSE;
			}
			if(pdata[3] > PARA_BYTE3_MAX)
			{
				return FALSE;
			}
			if(pdata[4] > PARA_BYTE4_MAX)
			{
				return FALSE;
			}
			if(pdata[5] > PARA_BYTE5_MAX)
			{
				return FALSE;
			}		
		}			
		break;
		case FILE_MODE_RESERVER:
		case FILE_MODE_USER1:
		case FILE_MODE_USER2:
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

/*********************************************************
@Description:文件写入
@Input:state : 
@Output:
@Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令		保留 			模式	保留	保留		  		
数组位置	CMD_IDX		CMD_PARA_IDX	+1		+2		+3		
模式：00：内部参数区 01：保留区 02用户区 03用户区
偏移：0xff写最新记录，当记录满时，擦除所有记录
	  0xfe写最新记录，当记录满时，不擦除所有记录
	  
标签应答
定义		命令代码1字节 	执行状态2字节 	
数组位置	CMD_IDX			EXCUTE_STATE_IDX								
*********************************************************/
uint8_t Write_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint8_t max_length;
	uint16_t max_offset;//最大长度、最大偏移
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令状态
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA:
		{
			max_offset = Rom_Record_Offset[0];		
			max_length = Rom_Record_Length[0];	
			nrf_addr = ROM_BaseAddr.PARA_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.PARA_Pos;
		}
		break;
		case FILE_MODE_RESERVER:
		{
			max_offset = Rom_Record_Offset[1];		
			max_length = Rom_Record_Length[1];
			nrf_addr = ROM_BaseAddr.RESERVER_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.RESERVER_Pos;
		}
		break;
		case FILE_MODE_USER1:
		{
			max_offset = Rom_Record_Offset[2];		
			max_length = Rom_Record_Length[2];			
			nrf_addr = ROM_BaseAddr.USER1_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.USER1_Pos;
		}
		break;
		case FILE_MODE_USER2:
		{
			max_offset = Rom_Record_Offset[3];		
			max_length = Rom_Record_Length[3];		
			nrf_addr = ROM_BaseAddr.USER2_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.USER2_Pos;
		}
		break;
		case FILE_MODE_RUNPARA:
		{
			max_offset = 1;
			max_length = PARA_RECORD_LEN;
		}
		break;
		default:
		{
			cmd_state = FILE_ERR <<8 | FILE_MODE_ERR;
		}
		break;
	}
	//长度边界检查
	if(f1_para.length>max_length )
	{
		cmd_state = FILE_ERR << 8 | FILE_BODER_ERR;
	}
	//偏移检查
	switch(f1_para.offset)
	{
		case FILE_OFFSET_WNEW:break;//写最新记录
		case FILE_OFFSET_WNEW_NERASE:
		{
			if(*ROM_BaseAddr.pROM_Pos >= max_offset)//超出最大偏移量，不擦除
			{
				cmd_state = FILE_ERR << 8|FILE_FULL_ERR;//记录满
			}
		}
		break;
		default://错误
		{
			cmd_state = FILE_ERR << 8 | FILE_WOFFSET_ERR;
		}
		break;
	}
	//参数检查
	if(TRUE != para_check(f1_para.mode,&p_packet[FILE_WDATA_IDX]))
	{
		cmd_state = FILE_ERR << 8|FILE_WDATA_ERR;
	}
	//错误，返回
	if(cmd_state!=CMD_RUN_SUCCESS)
	{
		p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
		p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
		return FALSE;
	}
	else
	{
		if(f1_para.mode == FILE_MODE_RUNPARA)
		{
			my_memcpy(para_record,&p_packet[FILE_WDATA_IDX],PARA_RECORD_LEN);
		}
		else
		{
			if(*ROM_BaseAddr.pROM_Pos >= max_offset)//超出最大偏移量，擦除
			{
				nrf_nvmc_page_erase(nrf_addr);
				*ROM_BaseAddr.pROM_Pos = 0;//更新最新偏移量				
			}
			nrf_addr += *ROM_BaseAddr.pROM_Pos*max_length;
			nrf_nvmc_write_bytes(nrf_addr,&p_packet[FILE_WDATA_IDX],f1_para.length);
			(*ROM_BaseAddr.pROM_Pos)++;
			if(f1_para.mode == FILE_MODE_PARA)//更新参数
			{
				my_memcpy(para_record,&p_packet[FILE_WDATA_IDX],16);
				UpdateRunPara();
			}			
		}

	}
	p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
	p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
	return TRUE;
}


/*********************************************************
@Description:擦除
@Input:state : 
@Output:
@Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令		保留 			模式		保留	保留
数组位置	CMD_IDX		CMD_PARA_IDX	+1			+2		+3		
模式：00：内部参数区 01：保留区 02用户区 03用户区
偏移：0xff写最新记录，当记录满时，擦除所有记录
	  0xfe写最新记录，当记录满时，不擦除所有记录
	  
标签应答
定义		命令代码1字节 	执行状态2字节 	
数组位置	CMD_IDX			EXCUTE_STATE_IDX							
*********************************************************/
uint8_t Erase_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令状态
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA:
		{
			nrf_addr = ROM_BaseAddr.PARA_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.PARA_Pos;
		}
		break;
		case FILE_MODE_RESERVER:
		{
			nrf_addr = ROM_BaseAddr.RESERVER_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.RESERVER_Pos;
		}
		break;
		case FILE_MODE_USER1:	
		{
			nrf_addr = ROM_BaseAddr.USER1_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.USER1_Pos;
		}
		break;
		case FILE_MODE_USER2:	
		{
			nrf_addr = ROM_BaseAddr.USER2_BASE;//基地址
			ROM_BaseAddr.pROM_Pos = &ROM_BaseAddr.USER2_Pos;
		}
		break;
		default:
		{
			cmd_state = FILE_ERR <<8 | FILE_MODE_ERR;
		}
		break;
	}

	//错误，返回
	if(cmd_state!=CMD_RUN_SUCCESS)
	{
		p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
		p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
		return FALSE;
	}
	else
	{
		p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
		p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
		nrf_nvmc_page_erase(nrf_addr);
	}
	return TRUE;
}

/*********************************************************
@Description:读运行参数
@Input:
@Output:
@Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令		  		
数组位置	CMD_IDX			
标签应答
定义		命令代码1字节 	执行状态2字节 	  数据
数组位置	CMD_IDX			EXCUTE_STATE_IDX  +2			
*********************************************************/
#if 0
uint8_t Read_Record(uint8_t *p_packet)
{
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令状态
	p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
	p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
	my_memcpy(&p_packet[EXCUTE_STATE_IDX+2],para_record,PARA_RECORD_LEN);			
	return TRUE;
}

/*********************************************************
@Description:写运行参数
@Input:
@Output:
@Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令		  		
数组位置	CMD_IDX			
标签应答
定义		命令代码1字节 	执行状态2字节 	  数据
数组位置	CMD_IDX			EXCUTE_STATE_IDX  +2			
*********************************************************/

uint8_t Write_Record(uint8_t *p_packet)
{
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令状态
	//参数检查
	if(TRUE != para_check(RUN_RECORD_PARA,&p_packet[FILE_WDATA_IDX]))
	{
		cmd_state = FILE_ERR << 8|FILE_WDATA_ERR;
	}
	//错误，返回
	if(cmd_state!=CMD_RUN_SUCCESS)
	{
		p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
		p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
		return FALSE;
	}
	p_packet[EXCUTE_STATE_IDX] = cmd_state>>8;
	p_packet[EXCUTE_STATE_IDX+1] = cmd_state;
	my_memcpy(para_record,&p_packet[EXCUTE_STATE_IDX+2],PARA_RECORD_LEN);		
	return TRUE;
}
#endif
/************************************************* 
@Description:获取文件操作地址
@Input:无
@Output:无
@Return:无
*************************************************/ 
void System_Addr_Init(void)
{
	ROM_BaseAddr.page_size = NRF_FICR->CODEPAGESIZE;
	ROM_BaseAddr.page_num  = NRF_FICR->CODESIZE - 1;
	
	ROM_BaseAddr.MARK_BASE = ROM_BaseAddr.page_size * ROM_BaseAddr.page_num;
	
	//para area
	ROM_BaseAddr.PARA_BASE = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-1);
	//reserved area
	ROM_BaseAddr.RESERVER_BASE = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-2);
	//user area1
	ROM_BaseAddr.USER1_BASE = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-3);
	//user area2
	ROM_BaseAddr.USER2_BASE = ROM_BaseAddr.page_size * (ROM_BaseAddr.page_num-4);
}
/*
@Description:获取系统运行参数，及获取参数区、保留区、用户区最新参数偏移量
@Input:state : 
@Output:
@Return:无
*/
void SystemParaInit(void)
{
	uint32_t nrf_addr;
	uint8_t base_offset;
	uint8_t flash_temp[11];//temp memory
	GetDeviceID();//获取设备ID
	System_Addr_Init();//获取基地址
	MSG_Addr_Init();//获取消息存储地址
	nrf_addr = ROM_BaseAddr.MARK_BASE;
	nrf_nvmc_read_bytes(nrf_addr,flash_temp,11);
	//最后一个扇区用来打标记，如果空，则清空ROM_BaseAddr.PARA_Pos-ROM5存储区,判断是否是新下载的程序
	if((flash_temp[0]!=nvmc_flash_mark[0])||(flash_temp[1]!=nvmc_flash_mark[1])||(flash_temp[2]!=nvmc_flash_mark[2])
		||(flash_temp[3]!=nvmc_flash_mark[3])||(flash_temp[4]!=nvmc_flash_mark[4])||(flash_temp[5]!=nvmc_flash_mark[5])
		||(flash_temp[6]!=nvmc_flash_mark[6])||(flash_temp[7]!=nvmc_flash_mark[7])||(flash_temp[8]!=nvmc_flash_mark[8])
		||(flash_temp[9]!=nvmc_flash_mark[9])||(flash_temp[10]!=nvmc_flash_mark[10]))
	{			 
		nrf_addr = ROM_BaseAddr.PARA_BASE;		
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = ROM_BaseAddr.RESERVER_BASE;
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = ROM_BaseAddr.USER1_BASE;
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = ROM_BaseAddr.USER2_BASE;
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = ROM_BaseAddr.MARK_BASE;
		nrf_nvmc_page_erase(nrf_addr);
		
		MSG_Erase_ALL();//擦除消息存储区	
		nrf_nvmc_write_bytes(nrf_addr,nvmc_flash_mark,11);
	}
	else
	{
		//打过标记读取上次存储的信息
		//内部参数区
		nrf_addr = ROM_BaseAddr.PARA_BASE;
		ROM_BaseAddr.PARA_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[0],Rom_Record_Length[0]);
		//reserver area
		nrf_addr = ROM_BaseAddr.RESERVER_BASE;
		ROM_BaseAddr.RESERVER_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[1],Rom_Record_Length[1]);
		//user area1
		nrf_addr = ROM_BaseAddr.USER1_BASE;
		ROM_BaseAddr.USER1_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[2],Rom_Record_Length[2]);
		//user area2
		nrf_addr = ROM_BaseAddr.USER2_BASE;
		ROM_BaseAddr.USER2_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[3],Rom_Record_Length[3]);	
		//更新消息参数
		MSG_Find_New();
	}
	if(ROM_BaseAddr.PARA_Pos)//更新过参数
	{
		nrf_addr = ROM_BaseAddr.PARA_BASE;
		base_offset = (ROM_BaseAddr.PARA_Pos - 1)*Rom_Record_Length[0];
		nrf_nvmc_read_bytes(nrf_addr+base_offset,para_record,Rom_Record_Length[0]);
	}		
	else//否则默认参数
	{
		my_memcpy(para_record,para_default,16);
	}
	UpdateRunPara();
}
