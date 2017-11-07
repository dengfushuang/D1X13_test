#include "LPC11xx.h"                                                    /* LPC11xx外设寄存器            */
#include "os_cpu.h"
#include "os_cfg.h"
#include "ucos_ii.h"
#include "uart.h"

#include "stdio.h"
#include "E2PROM.h"
#include <stdlib.h>
#include <string.h>
#include "nxplpc11xx.h"
#include "cmd_processbpk.h"
//#include "wdt.h"

#define EPROM_LEN 10

/*********************************************************************************************************
  定义全局变量
*********************************************************************************************************/
OS_EVENT *Uart0ReviceMbox;       //串口 0 接收邮箱

static OS_STK stkTaskWDT[128];
static OS_STK stkTaskMOTOR[128];
static OS_STK stkTaskUart0Cmd[128];
static OS_STK stkTaskUart0Revice[128];


//通道值
int8_t newChannel;
int8_t oldChannel;
int8_t   CH_temp;  //MODE 1下通道数
int8_t   CH_temp1; //MODE 0下通道数
int8_t   old_osw_status = 1;  
int8_t   new_osw_status = 1;
uint8_t  EPROM[EPROM_LEN];/*EPROM[0]:RST_NUM
                    EPROM[1]:WAY
					EPROM[2]:BJN
					EPROM[3]:old_CH
					EPROM[4]:new_CH
					EPROM[6]:addres 地址：0-1位光开关地址
					                      2-4位芯片地址
										  5-7位业务盘地址
					*/
uint8_t  MODE_FLAG=0;//模式标志位: 0 数据位控制  1 串口控制
uint8_t  motor_step; //步数状态
uint8_t  u0ReviceBuf[EPROM_LEN*2];   //串口接收数据缓存数组
uint8_t  u0SendBuf[EPROM_LEN*2];
uint8_t  u0ReviceBuf_temp[2];

//节拍
//uint16 step[9]={0x080,0x480,0x400,0x600,0x200,0xa00,0x800,0x880,0x080};
//uint16 code step_buf[9]={0xc80,0x480,0x680,0x600,0xe00,0xa00,0xa80,0x880,0xc80};
//uint16 step_buf[9]={0x0e00,0x0a00,0x0a80,0x0880,0x0c80,0x0480,0x0680,0x0600,0x0e00};
//反取
uint16_t step_buf[9]={0x0e,0x0a,0x0b,0x09,0x0d,0x05,0x07,0x06,0x0e};
//正取
//uint16 step_buf[9]={0x01,0x05,0x04,0x06,0x02,0x0a,0x08,0x09,0x01};


/*********************************************************************************************************
  任务定义
*********************************************************************************************************/
void TaskWDT(void *pvData);
void TaskMOTOR(void *pvData);
void TaskUart0Cmd(void *pvData);
void TaskUart0Revice(void *pvData);
void Uart0_init(void);
void GPIO_init(void);
void EPROM_init(void);
void reverse(uint8_t ff);
void forward(uint8_t rr);
void motor_init(uint8_t reset_step);
void delay_ms(uint32_t kk);


/*********************************************************************************************************
** Function name:       main
** Descriptions:        用户程序入口函数
**                      特殊说明：编译代码前请在网络上或uCOS-II官方网站上获取uCOS-II V2.52的源码，
**                                并添加到uCOSII文件夹下。否则将出现无法找到uCOSII源代码的编译错误提示；
** input parameters:    无
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
int main(void)
{
	SystemInit(); //系统初始化
	
	OSInit();
	OSTaskCreate(TaskWDT,         (void *)0, &stkTaskWDT[sizeof(stkTaskWDT) / 4 - 1],                 1);
	OSTaskCreate(TaskUart0Cmd,    (void *)0, &stkTaskUart0Cmd[sizeof(stkTaskUart0Cmd) / 4 - 1],       3);
	OSTaskCreate(TaskMOTOR,       (void *)0, &stkTaskMOTOR[sizeof(stkTaskMOTOR) / 4 - 1],             4);
	OSTaskCreate(TaskUart0Revice, (void *)0, &stkTaskUart0Revice[sizeof(stkTaskUart0Revice) / 4 - 1], 5);

	Uart0ReviceMbox = OSMboxCreate(NULL);
	if(Uart0ReviceMbox == NULL)
	{
		while (1);
	}
	OSStart();
	return 0;
}


/*********************************************************************************************************
** 函数名称: TaskWDT
** 功能描述: 喂看门狗任务
** 输　入: 无
** 输　出: 无
** 说  明: 
********************************************************************************************************/
void TaskWDT(void *pvData)
{
	pvData = pvData;

	UART_init(115200);
    GPIO_init();
	EPROM_init();
		
	while(1)
	{
		LPC_GPIO0->DATA ^= (1<<6); 
		OSTimeDly(500);
	}
}


/*********************************************************************************************************
** 函数名称: TaskMOTOR
** 功能描述: 电机任务
** 输　入: 无
** 输　出: 无
** 说  明: 
********************************************************************************************************/
void TaskMOTOR(void *pvData)
{
	uint8_t tt;
	int8_t sn;
	
	EPROM[0] = 34;//RST_NUM
	EPROM[1] = 13;//WAY
	EPROM[2] = 8; //BJN
	
    pvData = pvData;
	motor_init(EPROM[0]);
	MODE_FLAG = 0;
	while(1)
	{
		//检测到复位命令
	    if((LPC_GPIO2->DATA&(1<<8))==(0<<8)) 
		{	
			if(motor_step!=0)  //使步数为8的倍数
			{
				tt=motor_step;
				reverse(tt);
			}
			while((LPC_GPIO1->DATA&(1<<5))!=(0<<5))  //光电开关是否被挡住
			{
				reverse(8);
			}
			old_osw_status= -1; 	
			OSTimeDly(5);
			forward(EPROM[0]);//复位位置
			READY_L;
	        ERROR_L;				
			while((LPC_GPIO2->DATA&(1<<8))==(0<<8))  //处于复位状态下
			{
			    OSTimeDly(500);
				LPC_GPIO0->DATA ^= (1<<6); //RUN
			}
		}
		
		if(MODE_FLAG == 0)  //MODE 0数据处理
		{
		    new_osw_status = (LPC_GPIO2->DATA & 0x0f);
			CH_temp1 = new_osw_status + 1;
			if(new_osw_status != old_osw_status ) //收到通道切换命令,且未收到复位命令
			{
				if((new_osw_status>=0)&&(new_osw_status<EPROM[1]))
				{
					ERROR_L;
					READY_H;//READY状态为高电平,表示数据处理中
					if(new_osw_status > old_osw_status)//通道比原来的大，正转
					{
						sn = new_osw_status - old_osw_status;
						forward((sn)*(EPROM[2]*2));
						old_osw_status = new_osw_status;
					}
					else if(new_osw_status < old_osw_status)//通道比原来的小，反转               
					{
						sn = old_osw_status - new_osw_status;
						reverse((sn+1)*(EPROM[2]*2));//反转需要多走一步，之后再正转一步
						OSTimeDly(5);
						forward((EPROM[2]*2));
						old_osw_status = new_osw_status;
					}
					READY_L;//表示数据有效
				}
				else
				{
					ERROR_H;//大于通道数，数据溢出
				}
			}
		}
		OSTimeDly(1);
	}
}


/*******************************************************************************************************
                            TaskUart0Cmd 任务0
********************************************************************************************************/
void TaskUart0Cmd(void *pvData)
{
	uint8_t err;
	uint16_t len;	
	pvData = pvData;	
	while (1) 
	{ 
        OSMboxPend(Uart0ReviceMbox,0, &err);         // 等待接收邮箱数据
		if( (len = Cmd_process( (char*)&u0ReviceBuf ,(char *)u0SendBuf)) > 0 )
		{   
			UART_send( u0SendBuf ,len );
		}
		OSTimeDly(1);
	}
}


/*********************************************************************************************************
** 函数名称: TaskUart0Revice
** 功能描述: μCOS-II的任务。从UART0接收数据，当接收完一帧数据后通过消
**           息邮箱传送到TaskStart任务。
** 输　入: pdata        任务附加参数(指针)
** 输　出: 无
********************************************************************************************************/
void TaskUart0Revice(void *pvData)
{
	uint8_t *cp;
	uint8_t i,temp;

	while(1)
	{
		cp = u0ReviceBuf;
		*cp = UART_recive();
		if(*cp == '<')
		{
			cp++;
			for(i = 0; i < EPROM_LEN ; i++)
			{
				temp = UART_recive();
				*cp++ = temp;
				if(temp == '>')
				{
					while(i < (EPROM_LEN*2))
					{
						*cp++ = 0;  //空余的后面补0
						i++;
					}
					break;
				}
			}
			if(ADDRESS == u0ReviceBuf[6])
			{
				OSMboxAccept(Uart0ReviceMbox);  //清空 邮箱Uart0ReviceMbox
			    OSMboxPost(Uart0ReviceMbox,(void *)u0ReviceBuf);
			}	
		}
	  OSTimeDly(1);
	}
}


/*********************************************************************************************************
  初始化定义
*********************************************************************************************************/
void Uart0_init(void)
{
    UART_init(115200); // 初始化串口，波特率115200	
 	LPC_UART->IER = 0x01;	//只允许接收中断，关闭其他中断
 	NVIC_EnableIRQ(UART_IRQn); //开启串口中断
}


/*********************************************************************************************************
  GPIO使能
*********************************************************************************************************/
void GPIO_init(void)
{
	//SystemInit();
  SysCLK_config(); // 时钟配置
	
  //RUN灯P0.6设置为输出
  LPC_GPIO0->DIR |= (1<<6);
	
  //读取数据管脚P2.0---P2.7(D0--D7)设置为输入
	LPC_GPIO2->DIR &= ~(255<<0);
	
  //复位P2.8（RESET)为输入状态   	
	LPC_GPIO2->DIR &= ~(1<<8);
	
  //状态管脚P2.9（READY）,P2.10（ERROR）设置为输出
	LPC_GPIO2->DIR |= (3<<9);
	
  //把P3.0--P3.3脚设置为输出，电机驱动脚
	LPC_GPIO3->DIR |= (15<<0);
	
  //光电开关输出判断位，被弹片挡住输出低电平，不挡住输出高电平 <3.3V>
  //设置off(P1.5)为输入
	LPC_GPIO1->DIR &= ~(1<<5);
}


//二相八拍，反转
void reverse(uint8_t ff)
{
	for(;ff>0;ff--)
	{
		if(motor_step==0)motor_step=8;
		motor_step--;
		LPC_GPIO3->DATA  = step_buf[motor_step] | (LPC_GPIO3->DATA  & 0xf0);	
		OSTimeDly(2);
	}
}


//二相八拍，正转
void forward(uint8_t rr)
{
	for(;rr>0;rr--)
	{
		if(motor_step==8) motor_step=0;
		motor_step++;
		LPC_GPIO3->DATA  = step_buf[motor_step] | (LPC_GPIO3->DATA  & 0xf0);
		OSTimeDly(2);	
	}
}


//EEPROM初始化
void EPROM_init(void)
{
    eepromRead(0x100, EPROM, sizeof(EPROM));
	OSTimeDly(5);
	eepromRead(0x100, EPROM, sizeof(EPROM));
}


//电机初始化
void motor_init(uint8_t reset_step)
{
  motor_step=0;
	while((LPC_GPIO1->DATA&(1<<5))!=(0<<5))//光电开关是否被挡住
	{
		reverse(8);
	}
	delay_ms(50);
	old_osw_status = -1;	
	forward(reset_step);//复位位置
	READY_L;
	ERROR_L;
	CH_temp = 1;
}


void delay_ms(uint32_t kk)
{
  uint32_t i;
	while(kk != 0)
	{
		for(i=0;i<5500;i++);
		kk--;
	}
}

/*********************************************************************************************************
  End Of File
*********************************************************************************************************/






