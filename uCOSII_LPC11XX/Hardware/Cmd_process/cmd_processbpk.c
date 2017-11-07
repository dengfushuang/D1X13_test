#include "cmd_processbpk.h"

extern uint8_t EPROM[9];
extern int8_t oldChannel;
extern int8_t newChannel;
extern void motor_init(uint8_t reset_step);
extern void forward(uint8_t rr);
extern void reverse(uint8_t rr);
extern void delay_ms(uint32_t kk);

uint16_t  Cmd_process( char *sprintf_buf,char *ret_buf )
{
	FLAG AddrFlag = False;
	FLAG CMDFlag = False;
	FLAG ParamFlag = False;
	
	uint8_t address = 0;
	int8_t cmd_id = 0;
	int8_t parameter = 0;
	int8_t  Channeltemp1 = 0;
	uint16_t str_len = 0;
	
	
	READY_H;
	if((strstr(sprintf_buf,"MOTOR")!= NULL) && (sprintf_buf[0] == '<') && (sprintf_buf[9] == '>'))
	{
		
		address = sprintf_buf[6];
		if(ADDRESS == address)
		{
			AddrFlag = True;
			cmd_id = sprintf_buf[7];
		    parameter = sprintf_buf[8];
			if((parameter >= 0) && (parameter < MAX_CHANNEL))
			{
				ParamFlag = True;
			}else
			{
				ParamFlag = False;
			}
			if((cmd_id > 0) &&(cmd_id < 4))
			{
				CMDFlag = True;
			}else
            {
				CMDFlag = False;
			}
			
		}
	}
	if((AddrFlag == True) && (ParamFlag == True) && (CMDFlag == True))
	{
	    switch(cmd_id)
        {
			
/*****************复位指令**********************/			
			case 0x01:{
			     motor_init(EPROM[0]);
				 newChannel = 0;
				
			}break;
			
/*****************正转指令**********************/			
			case 0x02:{
				 Channeltemp1 =parameter + oldChannel;
				 if(Channeltemp1 > MAX_CHANNEL)
				 {
					 newChannel = 0;
					 motor_init(EPROM[0]);
				 }else
				 {
					 newChannel = Channeltemp1;
					 forward((parameter)*(EPROM[2]*2));
				 }
			     
			}break;

/*****************反转指令**********************/
			case 0x03:{
					 Channeltemp1 = oldChannel - parameter;
					 if(Channeltemp1 < 1)
					 {
						 if(parameter == 1)
						 {
							forward((MAX_CHANNEL - 1)*(EPROM[2]*2));
							newChannel = MAX_CHANNEL;
						 }
						 else
						 {
							newChannel = 0;
							motor_init(EPROM[0]);
						 }
					 }else 
					 {
						newChannel = Channeltemp1;
						reverse((parameter +1)*(EPROM[2]*2));
						delay_ms(5);
						forward(EPROM[2]*2);
					 }	     
			}break;
			default:break;
		}
		oldChannel = newChannel;
		EPROM[4] = newChannel;
		OS_ENTER_CRITICAL();
		eepromWriteNByte(EPROM,0x100,sizeof(EPROM));
		OS_EXIT_CRITICAL();
        str_len = sprintf(ret_buf,"%s,%c,%c,%s","<OK_MOTOR",(char)address,(char)parameter,">");		
	}
/******************当指令或参数错误时执行***********************/
/******************当地址错误时不进行任何操作***********************/	
	else
    {
		if(AddrFlag == True)
		{
			if((ParamFlag == False) && (CMDFlag == True))
			{
			     str_len = sprintf(ret_buf,"%s,%c,,%s","<ERROR",0x01,">");	
			}
			else if((ParamFlag == True) && (CMDFlag == False))
			{
				str_len = sprintf(ret_buf,"%s,%c,,%s","<ERROR",0x02,">");
			}else
			{
				str_len = sprintf(ret_buf,"%s,%c,,%s","<ERROR",0x03,">");
			}
		}
	}
	READY_L;
    return str_len;	
}
