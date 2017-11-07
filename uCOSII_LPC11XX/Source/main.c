#include "LPC11xx.h"                                                    /* LPC11xx����Ĵ���            */
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
  ����ȫ�ֱ���
*********************************************************************************************************/
OS_EVENT *Uart0ReviceMbox;       //���� 0 ��������

static OS_STK stkTaskWDT[128];
static OS_STK stkTaskMOTOR[128];
static OS_STK stkTaskUart0Cmd[128];
static OS_STK stkTaskUart0Revice[128];


//ͨ��ֵ
int8_t newChannel;
int8_t oldChannel;
int8_t   CH_temp;  //MODE 1��ͨ����
int8_t   CH_temp1; //MODE 0��ͨ����
int8_t   old_osw_status = 1;  
int8_t   new_osw_status = 1;
uint8_t  EPROM[EPROM_LEN];/*EPROM[0]:RST_NUM
                    EPROM[1]:WAY
					EPROM[2]:BJN
					EPROM[3]:old_CH
					EPROM[4]:new_CH
					EPROM[6]:addres ��ַ��0-1λ�⿪�ص�ַ
					                      2-4λоƬ��ַ
										  5-7λҵ���̵�ַ
					*/
uint8_t  MODE_FLAG=0;//ģʽ��־λ: 0 ����λ����  1 ���ڿ���
uint8_t  motor_step; //����״̬
uint8_t  u0ReviceBuf[EPROM_LEN*2];   //���ڽ������ݻ�������
uint8_t  u0SendBuf[EPROM_LEN*2];
uint8_t  u0ReviceBuf_temp[2];

//����
//uint16 step[9]={0x080,0x480,0x400,0x600,0x200,0xa00,0x800,0x880,0x080};
//uint16 code step_buf[9]={0xc80,0x480,0x680,0x600,0xe00,0xa00,0xa80,0x880,0xc80};
//uint16 step_buf[9]={0x0e00,0x0a00,0x0a80,0x0880,0x0c80,0x0480,0x0680,0x0600,0x0e00};
//��ȡ
uint16_t step_buf[9]={0x0e,0x0a,0x0b,0x09,0x0d,0x05,0x07,0x06,0x0e};
//��ȡ
//uint16 step_buf[9]={0x01,0x05,0x04,0x06,0x02,0x0a,0x08,0x09,0x01};


/*********************************************************************************************************
  ������
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
** Descriptions:        �û�������ں���
**                      ����˵�����������ǰ���������ϻ�uCOS-II�ٷ���վ�ϻ�ȡuCOS-II V2.52��Դ�룬
**                                ����ӵ�uCOSII�ļ����¡����򽫳����޷��ҵ�uCOSIIԴ����ı��������ʾ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
int main(void)
{
	SystemInit(); //ϵͳ��ʼ��
	
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
** ��������: TaskWDT
** ��������: ι���Ź�����
** �䡡��: ��
** �䡡��: ��
** ˵  ��: 
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
** ��������: TaskMOTOR
** ��������: �������
** �䡡��: ��
** �䡡��: ��
** ˵  ��: 
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
		//��⵽��λ����
	    if((LPC_GPIO2->DATA&(1<<8))==(0<<8)) 
		{	
			if(motor_step!=0)  //ʹ����Ϊ8�ı���
			{
				tt=motor_step;
				reverse(tt);
			}
			while((LPC_GPIO1->DATA&(1<<5))!=(0<<5))  //��翪���Ƿ񱻵�ס
			{
				reverse(8);
			}
			old_osw_status= -1; 	
			OSTimeDly(5);
			forward(EPROM[0]);//��λλ��
			READY_L;
	        ERROR_L;				
			while((LPC_GPIO2->DATA&(1<<8))==(0<<8))  //���ڸ�λ״̬��
			{
			    OSTimeDly(500);
				LPC_GPIO0->DATA ^= (1<<6); //RUN
			}
		}
		
		if(MODE_FLAG == 0)  //MODE 0���ݴ���
		{
		    new_osw_status = (LPC_GPIO2->DATA & 0x0f);
			CH_temp1 = new_osw_status + 1;
			if(new_osw_status != old_osw_status ) //�յ�ͨ���л�����,��δ�յ���λ����
			{
				if((new_osw_status>=0)&&(new_osw_status<EPROM[1]))
				{
					ERROR_L;
					READY_H;//READY״̬Ϊ�ߵ�ƽ,��ʾ���ݴ�����
					if(new_osw_status > old_osw_status)//ͨ����ԭ���Ĵ���ת
					{
						sn = new_osw_status - old_osw_status;
						forward((sn)*(EPROM[2]*2));
						old_osw_status = new_osw_status;
					}
					else if(new_osw_status < old_osw_status)//ͨ����ԭ����С����ת               
					{
						sn = old_osw_status - new_osw_status;
						reverse((sn+1)*(EPROM[2]*2));//��ת��Ҫ����һ����֮������תһ��
						OSTimeDly(5);
						forward((EPROM[2]*2));
						old_osw_status = new_osw_status;
					}
					READY_L;//��ʾ������Ч
				}
				else
				{
					ERROR_H;//����ͨ�������������
				}
			}
		}
		OSTimeDly(1);
	}
}


/*******************************************************************************************************
                            TaskUart0Cmd ����0
********************************************************************************************************/
void TaskUart0Cmd(void *pvData)
{
	uint8_t err;
	uint16_t len;	
	pvData = pvData;	
	while (1) 
	{ 
        OSMboxPend(Uart0ReviceMbox,0, &err);         // �ȴ�������������
		if( (len = Cmd_process( (char*)&u0ReviceBuf ,(char *)u0SendBuf)) > 0 )
		{   
			UART_send( u0SendBuf ,len );
		}
		OSTimeDly(1);
	}
}


/*********************************************************************************************************
** ��������: TaskUart0Revice
** ��������: ��COS-II�����񡣴�UART0�������ݣ���������һ֡���ݺ�ͨ����
**           Ϣ���䴫�͵�TaskStart����
** �䡡��: pdata        ���񸽼Ӳ���(ָ��)
** �䡡��: ��
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
						*cp++ = 0;  //����ĺ��油0
						i++;
					}
					break;
				}
			}
			if(ADDRESS == u0ReviceBuf[6])
			{
				OSMboxAccept(Uart0ReviceMbox);  //��� ����Uart0ReviceMbox
			    OSMboxPost(Uart0ReviceMbox,(void *)u0ReviceBuf);
			}	
		}
	  OSTimeDly(1);
	}
}


/*********************************************************************************************************
  ��ʼ������
*********************************************************************************************************/
void Uart0_init(void)
{
    UART_init(115200); // ��ʼ�����ڣ�������115200	
 	LPC_UART->IER = 0x01;	//ֻ��������жϣ��ر������ж�
 	NVIC_EnableIRQ(UART_IRQn); //���������ж�
}


/*********************************************************************************************************
  GPIOʹ��
*********************************************************************************************************/
void GPIO_init(void)
{
	//SystemInit();
  SysCLK_config(); // ʱ������
	
  //RUN��P0.6����Ϊ���
  LPC_GPIO0->DIR |= (1<<6);
	
  //��ȡ���ݹܽ�P2.0---P2.7(D0--D7)����Ϊ����
	LPC_GPIO2->DIR &= ~(255<<0);
	
  //��λP2.8��RESET)Ϊ����״̬   	
	LPC_GPIO2->DIR &= ~(1<<8);
	
  //״̬�ܽ�P2.9��READY��,P2.10��ERROR������Ϊ���
	LPC_GPIO2->DIR |= (3<<9);
	
  //��P3.0--P3.3������Ϊ��������������
	LPC_GPIO3->DIR |= (15<<0);
	
  //��翪������ж�λ������Ƭ��ס����͵�ƽ������ס����ߵ�ƽ <3.3V>
  //����off(P1.5)Ϊ����
	LPC_GPIO1->DIR &= ~(1<<5);
}


//������ģ���ת
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


//������ģ���ת
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


//EEPROM��ʼ��
void EPROM_init(void)
{
    eepromRead(0x100, EPROM, sizeof(EPROM));
	OSTimeDly(5);
	eepromRead(0x100, EPROM, sizeof(EPROM));
}


//�����ʼ��
void motor_init(uint8_t reset_step)
{
  motor_step=0;
	while((LPC_GPIO1->DATA&(1<<5))!=(0<<5))//��翪���Ƿ񱻵�ס
	{
		reverse(8);
	}
	delay_ms(50);
	old_osw_status = -1;	
	forward(reset_step);//��λλ��
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






