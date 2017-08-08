#include "app_var.h"
#include "nrf_nvmc.h"
#include "app_msg.h"
//设备ID
uint8_t DeviceID[4] = {0x33,0x33,0x33,0x33};
/********************************************
					自定义FLASH存储区
********************************************/
//MARK
const uint8_t nvmc_flash_mark[11]={0x54,0x46,0x4E,0x31,0x31,0x38,0x41,0x00,0x00,0x00,0x00};//TFN118A
uint32_t nrf_addr;//flash地址
uint8_t Rom_Record_Offset[4] = {16,16,32,32};//4个扇区对应的最大偏移量,分别对应参数区、保留区、用户区1、用户区2
uint8_t Rom_Record_Length[4] = {16,16,16,16};//每条记录对应的字节数

Para_Typedef Reader_Para;


#define delay_interval   40  //40ms

//设备运行默认参数
const uint8_t  para_default[PARA_RECORD_LEN] = 
{
16,//短号
0x60,//b7~4-发射功率0dbm,不使能周期发送
};
//以下几个参数用来记录ROM中的16/32块（偏移量）的计数值 值16/32，表示记录个数，0表示未记录
//255扇区打标记0x3fc00
//254扇区ROM0_Pos 存储频道参数  0x3f800
//253扇区ROM1_Pos 保留区 0x3f400
//252扇区ROM2_Pos 用户区1 0x3f00
//251扇区ROM3_Pos 用户区2 0x3ec00
uint8_t RomMark;//倒数第一个区打标机
uint8_t	ROM0_Pos;		//倒数第二个区，内部参数区
uint8_t	ROM1_Pos;		//倒数第三个扇区，保留区
uint8_t	ROM2_Pos;	  	//倒数第四个扇区，用户区1
uint8_t	ROM3_Pos;   	//倒数第五个扇区,用户区2
uint8_t *pROM_Pos;			//记录指针

ROM_BaseAddr_Typedef   ROM_BaseAddr;//ROM基地址定义
// uint8_t	* caucpROM[]={&ROM0_Pos,&ROM1_Pos,&ROM2_Pos,&ROM3_Pos};	
//存储内部参数 
uint8_t para_record[PARA_RECORD_LEN];

//命令
uint16_t cmd_state;//命令执行情况


/*
Description:返回最新记录ROM位置
Input:state :
Output:无
Return:无
*/
//uint8_t GetValidPara(uint8_t type,)
//{
//	switch(type)
//	{
//		case TYPE_PARA: nrf_addr = 
//	}
//}



/*
Description:返回最新记录ROM位置
Input:state : temp_addr存储区地址，temp_size存储区总记录个数，temp_byte每条记录长度
Output:返回最新记录位置 0：表示无记录，空 ；>1 对应的最新记录
Return:无
*/
uint8_t Rom_Pos(uint32_t temp_addr,uint8_t temp_size,uint8_t temp_byte)
{
	uint8_t i,j;
//	temp_addr = (uint8_t*)temp_addr;
	//验证buff是否为空，返回最新记录的位置
	//不相等，继续查找,找到空，则返回rom位置，i =  1~~Rom_record_size。返回0表示配置区全空	
	for(i=0;i<temp_size;i++)
	{
		for(j=0;j<temp_byte;j++)
		{
			if(*(uint8_t*)temp_addr++ != 0xff)
				break;
		}
		if(j>=temp_byte) break;
		i++;
	}
	return i;
}


/****************************************
函数：获取设备ID
输入：
输出：
****************************************/
void GetDeviceID(void)
{
	nrf_nvmc_read_bytes(ID_BEGIN,DeviceID,4);
}


/*
Description:运行内部参数
Input:state : 
Output:
Return:无
*/
void UpdateRunPara(void)
{
//	uint16_t unit;
//	uint8_t time;
	//发射功率
	Reader_Para.tx_pwr = ((para_record[P_PWR_IDX] & P_PWR_Msk) >> P_PWR_Pos);
	radio_pwr(Reader_Para.tx_pwr);
	//周期发送
	Reader_Para.radio_cycle_time = ((para_record[READERP_SENDTIME_IDX] & READERP_SENDTIME_Msk) >> READERP_SENDTIME_Pos)*TIM0_TIME;
	if(Reader_Para.radio_cycle_time)
		Reader_Para.radio_time_cnt_en = 1;
	else
		Reader_Para.radio_time_cnt_en = 0;
}

/*********************************************************
Description:文件读取
Input:state : 
Output:
Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令	保留 	模式		偏移  	长度	
数组位置	10		11		13			14		16	
模式：01：内部参数区 02：保留区 03用户区 04用户区
偏移量 0xffff读取最新参数
标签应答
定义		命令代码1字节 	执行状态2字节 	数据
数组位置	10				11				13
					
*********************************************************/
uint8_t Read_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint8_t max_length;
	uint16_t max_offset;//最大长度、最大偏移
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令执行状态
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA://内部参数区
				max_offset = Rom_Record_Offset[0];
				max_length = Rom_Record_Length[0];	
				nrf_addr = ROM_BaseAddr.PARA_BASE;//基地址
				pROM_Pos = &ROM0_Pos;//最新记录
			break;
		case FILE_MODE_RESERVER://保留区
				max_offset = Rom_Record_Offset[1];
				max_length = Rom_Record_Length[1];
				nrf_addr = ROM_BaseAddr.RESERVER_BASE;
				pROM_Pos = &ROM1_Pos;
			break;
		case FILE_MODE_USER1://用户区1
				max_offset = Rom_Record_Offset[2];
				max_length = Rom_Record_Length[2];			
				nrf_addr = ROM_BaseAddr.USER1_BASE;
				pROM_Pos = &ROM2_Pos;
			break;
		case FILE_MODE_USER2://用户区2
				max_offset = Rom_Record_Offset[3];
				max_length = Rom_Record_Length[3];		
				nrf_addr = ROM_BaseAddr.USER2_BASE;
				pROM_Pos = &ROM3_Pos;
			break; 
		default:
			cmd_state = (FILE_ERR <<8) | FILE_MODE_ERR;
			break;
	}
	//长度和偏移量边界检查
	if(f1_para.length>=max_length || (f1_para.offset>=max_offset && f1_para.offset<FILE_OFFSET_RNEW))
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
		if(FILE_OFFSET_RNEW == f1_para.offset)//读取最新记录
		{
			if(0 == *pROM_Pos)//记录为空
			{
				if(FILE_MODE_PARA == f1_para.mode)//参数区为空，返回运行参数
					my_memcpy(&p_packet[FILE_RDATA_IDX],para_record,f1_para.length);
			}
			else
			{
				nrf_addr += (*pROM_Pos-1)*max_length;//偏移量*长度				
			}
		}
		else
		{
			nrf_addr += f1_para.offset*max_length;			
		}

	}
	nrf_nvmc_read_bytes(nrf_addr, &p_packet[FILE_RDATA_IDX],f1_para.length);
	return TRUE;
}

/*********************************************************
Description:参数检查
Input:state : 
Output:
Return:无
**********************************************************/
uint8_t para_check(uint8_t mode,uint8_t *pdata)
{
	switch(mode)
	{
		case FILE_MODE_PARA://内部参数区
			if(pdata[TAGP_BRIEFNUM_IDX]>TAGP_BRIEFNUM_MAX_VALUE  
			||(pdata[P_PWR_IDX]>>P_PWR_Pos)>P_PWR_MAX_VALUE 
			||(pdata[TAGP_WORKMODE_IDX]&TAGP_WORKMODE_Msk)>TAGP_WORKMODE_MAX_VALUE)
			{
				return FALSE;
			}
			break;
		case FILE_MODE_RESERVER:
		case FILE_MODE_USER1:
		case FILE_MODE_USER2:
			break;
		default:
			cmd_state = FILE_ERR <<8 | FILE_MODE_ERR;
			return TRUE;
	}
	return TRUE;
}


/*********************************************************
Description:文件写入
Input:state : 
Output:
Return:无
内部文件（flash）操作-读写器下发读文件命令
定义		命令	保留 	模式		偏移  	长度	数据
数组位置	10		11		13			14		16		17
模式：01：内部参数区 02：保留区 03用户区 04用户区
偏移：0xffff写最新记录，当记录满时，擦除所有记录
	  0xfffe写最新记录，当记录满时，不擦除所有记录
	  
标签应答
定义		命令代码1字节 	执行状态2字节 	
数组位置	10				11				

					
*********************************************************/
uint8_t Write_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint8_t max_length;
	uint16_t max_offset;//最大长度、最大偏移
	uint16_t cmd_state = CMD_RUN_SUCCESS;//命令状态
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA:
				max_offset = Rom_Record_Offset[0];		
				max_length = Rom_Record_Length[0];	
				nrf_addr = ROM_BaseAddr.PARA_BASE;//基地址
				pROM_Pos = &ROM0_Pos;
			break;
		case FILE_MODE_RESERVER:
				max_offset = Rom_Record_Offset[1];		
				max_length = Rom_Record_Length[1];
				nrf_addr = ROM_BaseAddr.RESERVER_BASE;//基地址
				pROM_Pos = &ROM1_Pos;
			break;
		case FILE_MODE_USER1:
				max_offset = Rom_Record_Offset[2];		
				max_length = Rom_Record_Length[2];			
				nrf_addr = ROM_BaseAddr.USER1_BASE;//基地址
				pROM_Pos = &ROM2_Pos;
			break;
		case FILE_MODE_USER2:
				max_offset = Rom_Record_Offset[3];		
				max_length = Rom_Record_Length[3];		
				nrf_addr = ROM_BaseAddr.USER2_BASE;//基地址
				pROM_Pos = &ROM3_Pos;
			break;
		default:
			cmd_state = FILE_ERR <<8 | FILE_MODE_ERR;
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
		case FILE_OFFSET_WNEW:
			break;
		default://错误
			cmd_state = FILE_ERR << 8 | FILE_WOFFSET_ERR;
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
		if(*pROM_Pos >= max_offset)//超出最大偏移量，擦除
		{
			nrf_nvmc_page_erase(nrf_addr);
			*pROM_Pos = 0;//更新最新偏移量
		}
		nrf_addr += *pROM_Pos*max_length;
		nrf_nvmc_write_bytes(nrf_addr,&p_packet[FILE_WDATA_IDX],f1_para.length);
		(*pROM_Pos)++;
		if(f1_para.mode == FILE_MODE_PARA)//更新参数
		{
			my_memcpy(para_record,&p_packet[FILE_WDATA_IDX],16);
			UpdateRunPara();
		}
	}
	return TRUE;
}


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
	//最后一个扇区用来打标记，如果空，则清空ROM0_Pos-ROM5存储区,判断是否是新下载的程序
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
				
		nrf_nvmc_write_bytes(nrf_addr,nvmc_flash_mark,11);
	}
	else
	{
		//打过标记读取上次存储的信息
		//内部参数区
		nrf_addr = ROM_BaseAddr.PARA_BASE;
		ROM0_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[0],Rom_Record_Length[0]);
		//reserver area
		nrf_addr = ROM_BaseAddr.RESERVER_BASE;
		ROM1_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[1],Rom_Record_Length[1]);
		//user area1
		nrf_addr = ROM_BaseAddr.USER1_BASE;
		ROM2_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[2],Rom_Record_Length[2]);
		//user area2
		nrf_addr = ROM_BaseAddr.USER2_BASE;
		ROM3_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[3],Rom_Record_Length[3]);			

		MSG_Find_New();		
	}
	if(ROM0_Pos)//更新过参数
	{
		nrf_addr = ROM_BaseAddr.PARA_BASE;
		base_offset = (ROM0_Pos - 1)*Rom_Record_Length[0];
		nrf_nvmc_read_bytes(nrf_addr+base_offset,para_record,Rom_Record_Length[0]);
	}		
	else//否则默认参数
	{
		my_memcpy(para_record,para_default,16);
	}
	UpdateRunPara();
}
