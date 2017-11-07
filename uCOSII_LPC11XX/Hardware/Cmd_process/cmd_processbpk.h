#ifndef __CMD_PROCESSBPK_H
#define __CMD_PROCESSBPK_H
#include "LPC11xx.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "E2PROM.h"
#include "os_cpu.h"

#define MAX_CHANNEL 8

#define ADDRESS 0x26
//READY,ERROR状态管脚高低电平定义
#define READY_H   LPC_GPIO2->DATA |= (1<<9)
#define ERROR_H   LPC_GPIO2->DATA |= (1<<10)

#define READY_L		LPC_GPIO2->DATA &= ~(1<<9)
#define ERROR_L		LPC_GPIO2->DATA &= ~(1<<10)


typedef enum{
    False = 0,
	True
}FLAG;

extern uint16_t  Cmd_process( char *sprintf_buf,char *ret_buf );
#endif



