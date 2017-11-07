/**********************************************************************************************************
*文件名：Cmd_process.c
*创建时间: 2012-10-18
*功能描述: 处理串口和TCP的设置和查询指令
*作   者 ：
*公   司 ：
**********************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cmd_process.h"
#include "E2PROM.h"
#include "UART.h"
#include "OS_CPU.h"
#include "LPC11xx.h"  

extern uint8_t EPROM[9];
extern uint8_t u0ReviceBuf_temp[2];
extern uint8_t MODE_FLAG;
//extern int8_t old_osw_status;  
//extern int8_t new_osw_status;
extern void motor_init(uint8_t reset_step);
extern void forward(uint8_t rr);
extern void reverse(uint8_t rr);
extern void delay_ms(uint32_t kk);
//全局通道临时变量
extern int8_t CH_temp;
extern int8_t CH_temp1;
uint16_t  Cmd_process( char* sprintf_buf )
{
  uint8_t  CH_NUM;
  uint8_t  buf[3];
	uint16_t sprintf_len=0;
//	uint8_t WAY_temp;
//  uint8_t RST_NUM_temp;
//  uint8_t BJN_temp;
  int8_t SN;
	
	if( strstr(&sprintf_buf[0],"<MT")   != NULL ||
			strstr(&sprintf_buf[0],"ROUTE") != NULL ||
			strstr(&sprintf_buf[0],"MODE")  != NULL ||
			strstr(&sprintf_buf[0],"*IDN")  != NULL
	  )
	{
		//MODE ?
		if( strstr((char*)&sprintf_buf[0],"MODE") != NULL )
		{
		  if(sprintf_buf[5] == '?')
			  sprintf_len = sprintf((char *)sprintf_buf,"MODE %1u\r\n",MODE_FLAG);
			else if(sprintf_buf[5] == '1')
			  MODE_FLAG = 1;
			else if(sprintf_buf[5] == '0')
			  MODE_FLAG = 0;
		}
		
	  //<MT_RESET>
		else if( strstr((char*)&sprintf_buf[3],"_RESET>") != NULL && MODE_FLAG == 1)
		{
			motor_init(EPROM[0]);  
			sprintf_len = sprintf((char *)sprintf_buf,"<MT_RESET_OK>\r\n");
		}
	
		//<MT_FORWARD_XX>
		else if( strstr((char*)&sprintf_buf[3],"_FORWARD_") != NULL && sprintf_buf[14]=='>' && MODE_FLAG == 1)
		{
			if( (sprintf_buf[12]>='0' && sprintf_buf[12]<='9') &&
					(sprintf_buf[13]>='0' && sprintf_buf[13]<='9') )
			{
				buf[0] = sprintf_buf[12]-'0';
				buf[1] = sprintf_buf[13]-'0';
				SN = buf[0]*10+buf[1];
				
				CH_temp += SN;
				if(SN >= 0 && SN <= 13)
				{
					if(CH_temp > 13)
					{
						motor_init(EPROM[0]);
					}
					else forward((SN)*(EPROM[2]*2));
				}
				else  goto  send_err; 
				sprintf_len = sprintf((char *)sprintf_buf,"<MT_FORWARDED>\r\n");
			}
			else  goto  send_err; 
		}
	
		//<MT_REVERSE_XX>
		else if( strstr((char*)&sprintf_buf[3],"_REVERSE_") != NULL && sprintf_buf[14]=='>' && MODE_FLAG == 1)
		{
			if( (sprintf_buf[12]>='0' && sprintf_buf[12]<='9') &&
					(sprintf_buf[13]>='0' && sprintf_buf[13]<='9') )
			{
				buf[0] = sprintf_buf[12]-'0';
				buf[1] = sprintf_buf[13]-'0';
				SN = buf[0]*10+buf[1];
				
				CH_temp -= SN;
				if(SN >= 0 && SN <= 13)
				{
					if(CH_temp < 1)
					{
						if(SN == 1)
						{
							forward((12)*(EPROM[2]*2));
							CH_temp = 13;
						}
						else
						{
							motor_init(EPROM[0]);
						}
					}
					else 
					{
						reverse((SN+1)*(EPROM[2]*2));
						delay_ms(5);
						forward(EPROM[2]*2);
					}
				}
				else  goto  send_err;
				sprintf_len = sprintf((char *)sprintf_buf,"<MT_REVERSED>\r\n");
			}
			else  goto  send_err; 
		}
		
		//<MT_CH_XX>
		else if( strstr((char*)&sprintf_buf[3],"_CH_") != NULL && (sprintf_buf[8]=='>' || sprintf_buf[8]==0 || sprintf_buf[9]=='>') && MODE_FLAG == 1)
		{
			if(sprintf_buf[7] >= '?')
			{
				sprintf_len = sprintf((char *)sprintf_buf,"<MT_CH_%02u>\r\n",CH_temp);
			}
			else if( (sprintf_buf[7]>='0' && sprintf_buf[7]<='9') &&
							 (sprintf_buf[8]>='0' && sprintf_buf[8]<='9') )
			{
				buf[0] = sprintf_buf[7]-'0';
				buf[1] = sprintf_buf[8]-'0';
				CH_NUM = buf[0]*10+buf[1];
				if(CH_NUM <= 13 && CH_NUM > 1)
				{
					if(CH_NUM >= CH_temp)
					{
						SN = CH_NUM - CH_temp;
						forward((SN)*(EPROM[2]*2));
						CH_temp = CH_NUM;
					}
					else if(CH_NUM > 1 && CH_NUM < CH_temp)
					{
						SN = CH_temp - CH_NUM;
						reverse((SN+1)*(EPROM[2]*2));
						delay_ms(5);
						forward(EPROM[2]*2);
						CH_temp = CH_NUM;
					}
				}
				else if(CH_NUM == 1) 
				{
					motor_init(EPROM[0]);
				}
				else goto send_err;
				sprintf_len = sprintf((char *)sprintf_buf,"<MT_CH_SETED>\r\n");
			}
			else goto send_err; 
		}

		//ROUTE ? , ROUTE A,XX
		else if( strstr((char*)&sprintf_buf[0],"ROUTE") != NULL)
		{
			if(sprintf_buf[6] == '?')
			{
				if(MODE_FLAG == 0)
				  sprintf_len = sprintf((char *)sprintf_buf,"<A>,<%02u>\r\n",CH_temp1);
				else
				  sprintf_len = sprintf((char *)sprintf_buf,"<A>,<%02u>\r\n",CH_temp);
			}
			else if(strstr((char*)&sprintf_buf[0],"ROUTE A,") != NULL && MODE_FLAG == 1)
			{
				buf[0] = u0ReviceBuf_temp[0]-'0';
				buf[1] = u0ReviceBuf_temp[1]-'0';
				CH_NUM = buf[0]*10+buf[1];
				if(CH_NUM <= 13 && CH_NUM > 1)
				{
					if(CH_NUM >= CH_temp)
					{
						SN = CH_NUM - CH_temp;
						forward((SN)*(EPROM[2]*2));
						CH_temp = CH_NUM;
					}
					else if(CH_NUM < CH_temp)
					{
						SN = CH_temp - CH_NUM;
						reverse((SN+1)*(EPROM[2]*2));
						delay_ms(5);
						forward((EPROM[2]*2));
						CH_temp = CH_NUM;
					}
				}
				else if(CH_NUM == 1) 
				{
					motor_init(EPROM[0]);
				}
			}
		}

		//*IDN?
		else if( strstr((char*)&sprintf_buf[1],"IDN?") != NULL)
		{
			sprintf_len = sprintf((char *)sprintf_buf,"UC Instruments GM81023(2X13) serial number:GI081164001\r\n");
		}
		else  goto  send_err;
	}
	else
	{
		if(MODE_FLAG == 1)
		{
		  send_err:
		    sprintf_len = sprintf((char *)sprintf_buf,"<CMD_ERR>\r\n");
		}
	}
	return sprintf_len;
}

