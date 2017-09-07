#ifndef _APP_READER_
#define _APP_READER_
typedef enum
{
	Idle,
	List_Tag,
	List_Reader,
	Auto_Reoprt,
	File_Deal,
	Tag_Report,
	Msg_Deal,
	Time_Set,
	Device_Test,
	Alarm_Clr
}Work_Mode_Typedef;

extern void app_process(void);
#endif

