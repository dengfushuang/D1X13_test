/****************************************Copyright (c)****************************************************
**                            Guangzhou ZHIYUAN electronics Co.,LTD.
**
**                                 http://www.embedtools.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:           I2CSlave.h
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
#include <LPC11xx.h>

#ifndef __I2C_H 
#define __I2C_H

#define INT8U        uint8_t
#define INT16U       uint16_t
#define INT32U       uint32_t

#define FAST_MODE_PLUS  0
 
#define I2CMASTER       0

#define BUFSIZE        16
#define MAX_TIMEOUT    0x00FFFFFF

#define SLAVE_ADDR      0xA2
#define READ_WRITE      0x01

#define RD_BIT          0x01

#define I2C_IDLE            0
#define I2C_STARTED         1
#define I2C_RESTARTED       2
#define I2C_REPEATED_START  3
#define DATA_ACK            4
#define DATA_NACK           5
#define I2C_WR_STARTED      6
#define I2C_RD_STARTED      7

#define I2CONSET_I2EN    0x00000040                                     /* I2C Control Set Register     */
#define I2CONSET_AA      0x00000004
#define I2CONSET_SI      0x00000008
#define I2CONSET_STO     0x00000010
#define I2CONSET_STA     0x00000020

#define I2CONCLR_AAC     0x00000004                                     /* I2C Control clear Register   */
#define I2CONCLR_SIC     0x00000008
#define I2CONCLR_STAC    0x00000020
#define I2CONCLR_I2ENC   0x00000040

#define I2DAT_I2C        0x00000000                                     /* I2C Data Reg                 */
#define I2ADR_I2C        0x00000000                                     /* I2C Slave Address Reg        */
#define I2SCLH_SCLH      0x00000180                                     /* I2C SCL Duty Cycle High Reg  */
#define I2SCLL_SCLL      0x00000180                                     /* I2C SCL Duty Cycle Low Reg   */
#define I2SCLH_HS_SCLH   0x00000020                                     /* Fast Plus I2C SCL Duty Cycle */
                                                                        /* High Reg                     */
#define I2SCLL_HS_SCLL   0x00000020                                     /* Fast Plus I2C SCL Duty Cycle */
                                                                        /* Low Reg                      */


extern volatile uint8_t I2CWrBuffer[BUFSIZE];
extern volatile uint8_t I2CRdBuffer[BUFSIZE];
extern volatile uint32_t I2CSlaveState;
extern volatile uint32_t I2CReadLength, I2CWriteLength;

extern void I2C_IRQHandler( void );
extern void I2CSlaveInit( void );

#endif                                                                  /* end __I2C_H                  */

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

