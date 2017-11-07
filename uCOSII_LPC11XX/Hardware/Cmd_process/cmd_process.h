#ifndef __CMD_PROCESS_H_
#define __CMD_PROCESS_H_

#include "LPC11xx.h"

//READY,ERROR状态管脚高低电平定义
#define READY_H   LPC_GPIO2->DATA |= (1<<9)
#define ERROR_H   LPC_GPIO2->DATA |= (1<<10)

#define READY_L		LPC_GPIO2->DATA &= ~(1<<9)
#define ERROR_L		LPC_GPIO2->DATA &= ~(1<<10)


extern uint16_t  Cmd_process( char *sprintf_buf );

#endif 

