/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           I2CSlave.c
** Last modified Date:  2010-05-07
** Last Version:        V1.0
** Descriptions:        I2C�ӻ������
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

extern uint8_t   eeprom[256];                                           /* ����EEPROM�洢��Ԫ��256�ֽ�  */
extern uint8_t   adrpoint;                                              /* ����EEPROM��д����ָ��       */
extern uint8_t   slarv;                                                 /* �����ӵ�ַ���ձ�־��Ϊ1ʱ��ʾ*/
                                                                        /* �����մӻ���ַ               */

/*********************************************************************************************************
** Function name:       I2C_IRQHandler
** Descriptions:        I2C�жϴ�����
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void I2C_IRQHandler(void) 
{
   uint8_t  sta;
   
   sta = LPC_I2C->STAT;                                                 /* ȡ��I2C״̬��                */
   sta &= 0xF8;
   switch(sta) {
      case  0x60:                                                       /* ���յ�����SLA+W              */
      case  0x68:   
            slarv = 0;
            LPC_I2C->CONSET = 0x04;
            break;
            
      case  0xA8:                                                       /* ���յ�SLA+R�����ѷ������ݲ�  */
                                                                        /* ���յ�ACKλ��                */
      case  0xB0:
      case  0xB8:
            LPC_I2C->DAT = eeprom[adrpoint];
            adrpoint++;
            LPC_I2C->CONSET = 0x04;
            break;
            
      case  0x80:                                                       /* ���յ�����                   */
            if(0==slarv) {
               adrpoint = LPC_I2C->DAT;
               slarv = 1;
            } else {
               eeprom[adrpoint] = LPC_I2C->DAT;
               adrpoint++;
            }
            LPC_I2C->CONSET = 0x04;
            break;
            
      case  0xA0:                                                       /* �ѷ������ݣ������յ���ACK    */
      case  0xC0:                                                       /* ���߽�������������������     */
      default:                                                          /* ����״̬                     */
            LPC_I2C->CONSET = 0x04;
   }
   
   LPC_I2C->CONCLR = 0x28;                                              /* �����I2C��־λ��STA��SI     */
}

/*********************************************************************************************************
** Function name:       I2CSlaveInit
** Descriptions:        ��ʼ��I2C������
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
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


