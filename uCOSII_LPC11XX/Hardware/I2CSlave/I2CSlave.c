/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           I2CSlave.c
** Last modified Date:  2010-05-07
** Last Version:        V1.0
** Descriptions:        I2C从机软件包
**
**--------------------------------------------------------------------------------------------------------
** Created by:          Lanwuqiang
** Created date:        2010-05-07
** Version:             V1.0
** Descriptions:        
**
**--------------------------------------------------------------------------------------------------------
** Modified by:        
** Modified date:      
** Version:            
** Descriptions:       
**
**--------------------------------------------------------------------------------------------------------
** Modified by:        
** Modified date:      
** Version:            
** Descriptions:       
**
** Rechecked by:
*********************************************************************************************************/
#include "I2CSlave.h"

extern uint8_t   eeprom[256];                                           /* 定义EEPROM存储单元，256字节  */
extern uint8_t   adrpoint;                                              /* 定义EEPROM读写操作指针       */
extern uint8_t   slarv;                                                 /* 定义子地址接收标志，为1时表示*/
                                                                        /* 己接收从机地址               */

/*********************************************************************************************************
** Function name:       I2C_IRQHandler
** Descriptions:        I2C中断处理函数
** input parameters:    无
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
void I2C_IRQHandler(void) 
{
   uint8_t  sta;
   
   sta = LPC_I2C->STAT;                                                 /* 取得I2C状态字                */
   sta &= 0xF8;
   switch(sta) {
      case  0x60:                                                       /* 接收到自身SLA+W              */
      case  0x68:   
            slarv = 0;
            LPC_I2C->CONSET = 0x04;
            break;
            
      case  0xA8:                                                       /* 接收到SLA+R，或已发送数据并  */
                                                                        /* 接收到ACK位。                */
      case  0xB0:
      case  0xB8:
            LPC_I2C->DAT = eeprom[adrpoint];
            adrpoint++;
            LPC_I2C->CONSET = 0x04;
            break;
            
      case  0x80:                                                       /* 接收到数据                   */
            if(0==slarv) {
               adrpoint = LPC_I2C->DAT;
               slarv = 1;
            } else {
               eeprom[adrpoint] = LPC_I2C->DAT;
               adrpoint++;
            }
            LPC_I2C->CONSET = 0x04;
            break;
            
      case  0xA0:                                                       /* 已发送数据，并接收到非ACK    */
      case  0xC0:                                                       /* 总线结束，或总线重新启动     */
      default:                                                          /* 其它状态                     */
            LPC_I2C->CONSET = 0x04;
   }
   
   LPC_I2C->CONCLR = 0x28;                                              /* 清除标I2C标志位，STA、SI     */
}

/*********************************************************************************************************
** Function name:       I2CSlaveInit
** Descriptions:        初始化I2C控制器
** input parameters:    无
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
void I2CSlaveInit( void ) 
{

  LPC_SYSCON->PRESETCTRL |= (0x01<<1);

  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<5);
  LPC_IOCON->PIO0_4 &= ~0x3F;                                           /*  I2C I/O config              */
  LPC_IOCON->PIO0_4 |= 0x01;                                            /* I2C SCL                      */
  LPC_IOCON->PIO0_5 &= ~0x3F;   
  LPC_IOCON->PIO0_5 |= 0x01;                                            /* I2C SDA                      */

  /*
   * Clear flags
   */
  LPC_I2C->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC;    

  /*
   * Reset registers
   */
#if FAST_MODE_PLUS
  LPC_IOCON->PIO0_4 |= (0x1<<9);
  LPC_IOCON->PIO0_5 |= (0x1<<9);
  LPC_I2C->SCLL   = I2SCLL_HS_SCLL;
  LPC_I2C->SCLH   = I2SCLH_HS_SCLH;
#else
  LPC_I2C->SCLL   = I2SCLL_SCLL;
  LPC_I2C->SCLH   = I2SCLH_SCLH;
#endif

  LPC_I2C->ADR0 = SLAVE_ADDR;    
  
  /*
   * Enable the I2C Interrupt
   */
   NVIC_EnableIRQ(I2C_IRQn);
   NVIC_SetPriority(I2C_IRQn, 3);
  
  LPC_I2C->CONSET = I2CONSET_I2EN | I2CONSET_SI;
  return;
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/


