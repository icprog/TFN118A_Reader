#include "app_var.h"
#include "nrf_nvmc.h"
//�豸ID
uint8_t DeviceID[4] = {0xff,0x00,0x00,0x01};
/********************************************
					�Զ���FLASH�洢��
********************************************/
//MARK
const uint8_t nvmc_flash_mark[11]={0x54,0x46,0x4E,0x31,0x31,0x38,0x41,0x00,0x00,0x00,0x00};//TFN118A
uint32_t nrf_addr;//flash��ַ
uint8_t Rom_Record_Offset[4] = {16,16,32,32};//4��������Ӧ�����ƫ����,�ֱ��Ӧ�����������������û���1���û���2
uint8_t Rom_Record_Length[4] = {16,16,16,16};//ÿ����¼��Ӧ���ֽ���
uint8_t ActiveMode;//���ڷ������־

//�豸����Ĭ�ϲ���
const uint8_t  para_default[PARA_RECORD_LEN] = 
{
16,//�̺�
0x60,//b7~4-���书��0dbm
0xf1,//b7~b4 ��Ϣ��Դ��Ĭ�ϲ�Я����Ϣ b3~b0 ����ģʽ ��Ĭ�ϻģʽ
0x00,//���ʴ���
0x00,//b7-6,��λ���֣�b5-0����������ֵ��5����
0
};
//���¼�������������¼ROM�е�16/32�飨ƫ�������ļ���ֵ ֵ16/32����ʾ��¼������0��ʾδ��¼
//255��������0x3fc00
//254����ROM0_Pos �洢Ƶ������  0x3f800
//253����ROM1_Pos ������ 0x3f400
//252����ROM2_Pos �û���1 0x3f00
//251����ROM3_Pos �û���2 0x3ec00
uint8_t RomMark;//������һ��������
uint8_t	ROM0_Pos;		//�����ڶ��������ڲ�������
uint8_t	ROM1_Pos;		//����������������������
uint8_t	ROM2_Pos;	  	//�������ĸ��������û���1
uint8_t	ROM3_Pos;   	//�������������,�û���2
uint8_t *pROM_Pos;			//��¼ָ��
// uint8_t	* caucpROM[]={&ROM0_Pos,&ROM1_Pos,&ROM2_Pos,&ROM3_Pos};	
//�洢�ڲ����� 
uint8_t para_record[PARA_RECORD_LEN];

//����
uint16_t cmd_state;//����ִ�����


/*
Description:�������¼�¼ROMλ��
Input:state :
Output:��
Return:��
*/
//uint8_t GetValidPara(uint8_t type,)
//{
//	switch(type)
//	{
//		case TYPE_PARA: nrf_addr = 
//	}
//}



/*
Description:�������¼�¼ROMλ��
Input:state : temp_addr�洢����ַ��temp_size�洢���ܼ�¼������temp_byteÿ����¼����
Output:�������¼�¼λ�� 0����ʾ�޼�¼���� ��>1 ��Ӧ�����¼�¼
Return:��
*/
uint8_t Rom_Pos(uint32_t temp_addr,uint8_t temp_size,uint8_t temp_byte)
{
	uint8_t i,j;
//	temp_addr = (uint8_t*)temp_addr;
	//��֤buff�Ƿ�Ϊ�գ��������¼�¼��λ��
	//����ȣ���������,�ҵ��գ��򷵻�romλ�ã�i =  1~~Rom_record_size������0��ʾ������ȫ��	
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
��������ȡ�豸ID
���룺
�����
****************************************/
void GetDeviceID(void)
{
	nrf_nvmc_read_bytes(ID_BEGIN,DeviceID,4);
}


/*
Description:�����ڲ�����
Input:state : 
Output:
Return:��
*/
void UpdateRunPara(void)
{
//	uint16_t unit;
//	uint8_t time;
	//����ģʽ��������� 1:�ģʽ
	ActiveMode = (para_record[TAGP_WORKMODE_IDX] & TAGP_WORKMODE_Msk) >> TAGP_WORKMODE_Pos;
	
	if(ActiveMode)
		radio_pwr((para_record[TAGP_PWR_IDX] & TAGP_PWR_Msk) >> TAGP_PWR_Pos);
	else
	radio_pwr(TAGP_PWR_N4DBM);	
}

/*********************************************************
Description:�ļ���ȡ
Input:state : 
Output:
Return:��
�ڲ��ļ���flash������-��д���·����ļ�����
����		����	���� 	ģʽ		ƫ��  	����	
����λ��	10		11		13			14		16	
ģʽ��01���ڲ������� 02�������� 03�û��� 04�û���
ƫ���� 0xffff��ȡ���²���
��ǩӦ��
����		�������1�ֽ� 	ִ��״̬2�ֽ� 	����
����λ��	10				11				13
					
*********************************************************/
uint8_t Read_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint8_t max_length;
	uint16_t max_offset;//��󳤶ȡ����ƫ��
	uint16_t cmd_state = CMD_RUN_SUCCESS;//����ִ��״̬
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA://�ڲ�������
				max_offset = Rom_Record_Offset[0];
				max_length = Rom_Record_Length[0];	
				nrf_addr = PARA_BASE;//����ַ
				pROM_Pos = &ROM0_Pos;//���¼�¼
			break;
		case FILE_MODE_RESERVER://������
				max_offset = Rom_Record_Offset[1];
				max_length = Rom_Record_Length[1];
				nrf_addr = RESERVER_BASE;
				pROM_Pos = &ROM1_Pos;
			break;
		case FILE_MODE_USER1://�û���1
				max_offset = Rom_Record_Offset[2];
				max_length = Rom_Record_Length[2];			
				nrf_addr = USER1_BASE;
				pROM_Pos = &ROM2_Pos;
			break;
		case FILE_MODE_USER2://�û���2
				max_offset = Rom_Record_Offset[3];
				max_length = Rom_Record_Length[3];		
				nrf_addr = USER2_BASE;
				pROM_Pos = &ROM3_Pos;
			break; 
		default:
			cmd_state = (FILE_ERR <<8) | FILE_MODE_ERR;
			break;
	}
	//���Ⱥ�ƫ�����߽���
	if(f1_para.length>max_length || (f1_para.offset>max_offset && f1_para.offset<FILE_OFFSET_RNEW))
	{
		cmd_state = FILE_ERR << 8 | FILE_BODER_ERR;
	}
	//�������
	if(cmd_state!=CMD_RUN_SUCCESS)
	{
		my_memcpy(&p_packet[EXCUTE_STATE_IDX],&cmd_state,EXCUTE_STATE_LENGTH);
		return FALSE;
	}
	else
	{
		if(FILE_OFFSET_RNEW == f1_para.offset)//��ȡ���¼�¼
		{
			if(0 == *pROM_Pos)//��¼Ϊ��
			{
				if(FILE_MODE_PARA == f1_para.mode)//������Ϊ�գ��������в���
					my_memcpy(&p_packet[FILE_RDATA_IDX],para_record,f1_para.length);
			}
			else
			{
				nrf_addr += (*pROM_Pos-1)*max_length;//ƫ����*����				
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
Description:�������
Input:state : 
Output:
Return:��
**********************************************************/
uint8_t para_check(uint8_t mode,uint8_t *pdata)
{
	switch(mode)
	{
		case FILE_MODE_PARA://�ڲ�������
			if(pdata[TAGP_BRIEFNUM_IDX]>TAGP_BRIEFNUM_MAX_VALUE  
			||(pdata[TAGP_PWR_IDX]>>TAGP_PWR_Pos)>TAGP_PWR_MAX_VALUE 
			||(pdata[TAGP_WORKMODE_IDX]&TAGP_WORKMODE_Msk)>TAGP_WORKMODE_MAX_VALUE)
			{
				return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}


/*********************************************************
Description:�ļ�д��
Input:state : 
Output:
Return:��
�ڲ��ļ���flash������-��д���·����ļ�����
����		����	���� 	ģʽ		ƫ��  	����	����
����λ��	10		11		13			14		16		17
ģʽ��01���ڲ������� 02�������� 03�û��� 04�û���
ƫ�ƣ�0xffffд���¼�¼������¼��ʱ���������м�¼
	  0xfffeд���¼�¼������¼��ʱ�����������м�¼
	  
��ǩӦ��
����		�������1�ֽ� 	ִ��״̬2�ֽ� 	
����λ��	10				11				

					
*********************************************************/
uint8_t Write_Para(File_Typedef f1_para,uint8_t *p_packet)
{
	uint8_t max_length;
	uint16_t max_offset;//��󳤶ȡ����ƫ��
	uint16_t cmd_state = CMD_RUN_SUCCESS;//����״̬
	switch(f1_para.mode)
	{
		case FILE_MODE_PARA:
				max_offset = Rom_Record_Offset[0];		
				max_length = Rom_Record_Length[0];	
				nrf_addr = PARA_BASE;//����ַ
				pROM_Pos = &ROM0_Pos;
			break;
		case FILE_MODE_RESERVER:
				max_offset = Rom_Record_Offset[1];		
				max_length = Rom_Record_Length[1];
				nrf_addr = RESERVER_BASE;//����ַ
				pROM_Pos = &ROM1_Pos;
			break;
		case FILE_MODE_USER1:
				max_offset = Rom_Record_Offset[2];		
				max_length = Rom_Record_Length[2];			
				nrf_addr = USER1_BASE;//����ַ
				pROM_Pos = &ROM2_Pos;
			break;
		case FILE_MODE_USER2:
				max_offset = Rom_Record_Offset[3];		
				max_length = Rom_Record_Length[3];		
				nrf_addr = USER2_BASE;//����ַ
				pROM_Pos = &ROM3_Pos;
			break;
		default:
			cmd_state = FILE_ERR <<8 | FILE_MODE_ERR;
			break;
	}
	//���ȱ߽���
	if(f1_para.length>max_length )
	{
		cmd_state = FILE_ERR << 8 | FILE_BODER_ERR;
	}
	//ƫ�Ƽ��
	switch(f1_para.offset)
	{
		case FILE_OFFSET_WNEW:
			break;
		default://����
			cmd_state = FILE_ERR << 8 | FILE_WOFFSET_ERR;
			break;
	}
	//�������
	if(TRUE != para_check(f1_para.mode,&p_packet[FILE_WDATA_IDX]))
	{
		cmd_state = FILE_ERR << 8|FILE_WDATA_ERR;
	}
	//���󣬷���
	if(cmd_state!=CMD_RUN_SUCCESS)
	{
		my_memcpy(&p_packet[EXCUTE_STATE_IDX],&cmd_state,EXCUTE_STATE_LENGTH);
		return FALSE;
	}
	else
	{
		if(*pROM_Pos >= max_offset)//�������ƫ����������
		{
			nrf_nvmc_page_erase(nrf_addr);
			*pROM_Pos = 0;//��������ƫ����
		}
		nrf_addr += *pROM_Pos*max_length;
		nrf_nvmc_write_bytes(nrf_addr,&p_packet[FILE_WDATA_IDX],f1_para.length);		
	}
	return TRUE;
}

/*
Description:��ȡϵͳ���в���������ȡ�����������������û������²���ƫ����
Input:state : 
Output:
Return:��
*/
void SystemParaInit(void)
{
	uint32_t nrf_addr;
	uint8_t base_offset;
	uint8_t flash_temp[11];//temp memory
	GetDeviceID();//��ȡ�豸ID
	nrf_addr = MARK_BASE;
	nrf_nvmc_read_bytes(nrf_addr,flash_temp,11);
	//���һ�������������ǣ�����գ������ROM0_Pos-ROM5�洢��,�ж��Ƿ��������صĳ���
	if((flash_temp[0]!=nvmc_flash_mark[0])||(flash_temp[1]!=nvmc_flash_mark[1])||(flash_temp[2]!=nvmc_flash_mark[2])
		||(flash_temp[3]!=nvmc_flash_mark[3])||(flash_temp[4]!=nvmc_flash_mark[4])||(flash_temp[5]!=nvmc_flash_mark[5])
		||(flash_temp[6]!=nvmc_flash_mark[6])||(flash_temp[7]!=nvmc_flash_mark[7])||(flash_temp[8]!=nvmc_flash_mark[8])
		||(flash_temp[9]!=nvmc_flash_mark[9])||(flash_temp[10]!=nvmc_flash_mark[10]))
	{			 
		nrf_addr = PARA_BASE;		
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = RESERVER_BASE;
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = USER1_BASE;
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = USER2_BASE;
		nrf_nvmc_page_erase(nrf_addr);

		nrf_addr = MARK_BASE;
		nrf_nvmc_page_erase(nrf_addr);
				
		nrf_nvmc_write_bytes(nrf_addr,nvmc_flash_mark,11);
	}
	else
	{
		//�����Ƕ�ȡ�ϴδ洢����Ϣ
		//�ڲ�������
		nrf_addr = PARA_BASE;
		ROM0_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[0],Rom_Record_Length[0]);
		//reserver area
		nrf_addr = RESERVER_BASE;
		ROM1_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[1],Rom_Record_Length[1]);
		//user area1
		nrf_addr = USER1_BASE;
		ROM2_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[2],Rom_Record_Length[2]);
		//user area2
		nrf_addr = USER2_BASE;
		ROM3_Pos = Rom_Pos(nrf_addr,Rom_Record_Offset[3],Rom_Record_Length[3]);				
	}
	if(ROM0_Pos)//���¹�����
	{
		nrf_addr = PARA_BASE;
		base_offset = (ROM0_Pos - 1)*Rom_Record_Length[0];
		nrf_nvmc_read_bytes(nrf_addr+base_offset,para_record,Rom_Record_Length[0]);
	}		
	else//����Ĭ�ϲ���
	{
		my_memcpy(para_record,para_default,16);
	}
	UpdateRunPara();
}
