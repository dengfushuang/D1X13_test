/*****************************************************************************
 *   timer32.c:  32-bit Timer C file for NXP LPC1xxx Family Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2008.08.20  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "LPC11xx.h"
#include "timer32.h"
#include "gpio.h"

volatile uint32_t timer32_0_counter = 0;
volatile uint32_t timer32_1_counter = 0;
volatile uint32_t timer32_0_capture = 0;
volatile uint32_t timer32_1_capture = 9;

/*****************************************************************************
** Function name:		delay32Ms
**
** Descriptions:		Start the timer delay in milo seconds
**						until elapsed
**
** parameters:			timer number, Delay value in milo second			 
** 						
** Returned value:		None
** 
*****************************************************************************/
void delay32Ms(uint8_t timer_num, uint32_t delayInMs)
{
  if (timer_num == 0)
  {
    /* setup timer #0 for delay */
    LPC_TMR32B0->TCR = 0x02;		/* reset timer */
    LPC_TMR32B0->PR  = 0x00;		/* set prescaler to zero */
    LPC_TMR32B0->MR0 = delayInMs * (SystemAHBFrequency / 1000);
    LPC_TMR32B0->IR  = 0xff;		/* reset all interrrupts */
    LPC_TMR32B0->MCR = 0x04;		/* stop timer on match */
    LPC_TMR32B0->TCR = 0x01;		/* start timer */
  
    /* wait until delay time has elapsed */
    while (LPC_TMR32B0->TCR & 0x01);
  }
  else if (timer_num == 1)
  {
    /* setup timer #1 for delay */
    LPC_TMR32B1->TCR = 0x02;		/* reset timer */
    LPC_TMR32B1->PR  = 0x00;		/* set prescaler to zero */
    LPC_TMR32B1->MR0 = delayInMs * (SystemAHBFrequency / 1000);
    LPC_TMR32B1->IR  = 0xff;		/* reset all interrrupts */
    LPC_TMR32B1->MCR = 0x04;		/* stop timer on match */
    LPC_TMR32B1->TCR = 0x01;		/* start timer */
  
    /* wait until delay time has elapsed */
    while (LPC_TMR32B1->TCR & 0x01);
  }
  return;
}

/******************************************************************************
** Function name:		TIMER32_0_IRQHandler
**
** Descriptions:		Timer/Counter 0 interrupt handler
**						executes each 10ms @ 60 MHz CPU Clock
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void TIMER32_0_IRQHandler(void)
{
  if ( LPC_TMR32B0->IR & 0x01 )
  {  
	LPC_TMR32B0->IR = 1;				/* clear interrupt flag */
	LEDToggle( 1 , 11 );
	LEDToggle( 1 , 10 );
	LEDToggle( 1 , 9 );
	//LEDToggle( 1 , 8 );
	//LEDToggle( 2 , 2 );
	//LEDToggle( 2 , 3 );
  }
  if ( LPC_TMR32B0->IR & (0x1<<4) )
  {  
	LPC_TMR32B0->IR = 0x1<<4;			/* clear interrupt flag */
	timer32_0_capture++;
  }
  return;
}

/******************************************************************************
** Function name:		TIMER32_1_IRQHandler
**
** Descriptions:		Timer/Counter 1 interrupt handler
**						executes each 10ms @ 60 MHz CPU Clock
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void TIMER32_1_IRQHandler(void)
{
  if ( LPC_TMR32B1->IR & 0x01 )
  {    
	LPC_TMR32B1->IR = 1;			/* clear interrupt flag */
	timer32_1_counter++;
  }
  if ( LPC_TMR32B1->IR & (0x1<<4) )
  {  
	LPC_TMR32B1->IR = 0x1<<4;			/* clear interrupt flag */
	timer32_1_capture++;
  }
  return;
}

/******************************************************************************
** Function name:		enable_timer
**
** Descriptions:		Enable timer
**
** parameters:			timer number: 0 or 1
** Returned value:		None
** 
******************************************************************************/
void enable_timer32(uint8_t timer_num)
{
  if ( timer_num == 0 )
  {
    LPC_TMR32B0->TCR = 1;
  }
  else
  {
    LPC_TMR32B1->TCR = 1;
  }
  return;
}

/******************************************************************************
** Function name:		disable_timer
**
** Descriptions:		Disable timer
**
** parameters:			timer number: 0 or 1
** Returned value:		None
** 
******************************************************************************/
void disable_timer32(uint8_t timer_num)
{
  if ( timer_num == 0 )
  {
    LPC_TMR32B0->TCR = 0;
  }
  else
  {
    LPC_TMR32B1->TCR = 0;
  }
  return;
}

/******************************************************************************
** Function name:		reset_timer
**
** Descriptions:		Reset timer
**
** parameters:			timer number: 0 or 1
** Returned value:		None
** 
******************************************************************************/
void reset_timer32(uint8_t timer_num)
{
  uint32_t regVal;

  if ( timer_num == 0 )
  {
    regVal = LPC_TMR32B0->TCR;
    regVal |= 0x02;
    LPC_TMR32B0->TCR = regVal;
  }
  else
  {
    regVal = LPC_TMR32B1->TCR;
    regVal |= 0x02;
    LPC_TMR32B1->TCR = regVal;
  }
  return;
}

/******************************************************************************
** Function name:		init_timer
**
** Descriptions:		Initialize timer, set timer interval, reset timer,
**						install timer interrupt handler
**
** parameters:			timer number and timer interval
** Returned value:		None
** 
******************************************************************************/
void init_timer32(uint8_t timer_num, uint32_t TimerInterval) 
{
  if ( timer_num == 0 )
  {
    /* Some of the I/O pins need to be clearfully planned if
    you use below module because JTAG and TIMER CAP/MAT pins are muxed. */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<9);
    LPC_IOCON->PIO1_5 &= ~0x07;	/*  Timer0_32 I/O config */
    LPC_IOCON->PIO1_5 |= 0x02;	/* Timer0_32 CAP0 */
    LPC_IOCON->PIO1_6 &= ~0x07;
    LPC_IOCON->PIO1_6 |= 0x02;	/* Timer0_32 MAT0 */
    LPC_IOCON->PIO1_7 &= ~0x07;
    LPC_IOCON->PIO1_7 |= 0x02;	/* Timer0_32 MAT1 */
    LPC_IOCON->PIO0_1 &= ~0x07;	
    LPC_IOCON->PIO0_1 |= 0x02;	/* Timer0_32 MAT2 */
#ifdef __JTAG_DISABLED
    LPC_IOCON->JTAG_TDI_PIO0_11 &= ~0x07;	
    LPC_IOCON->JTAG_TDI_PIO0_11 |= 0x03;	/* Timer0_32 MAT3 */
#endif

    timer32_0_counter = 0;
    timer32_0_capture = 0;
    LPC_TMR32B0->MR0 = TimerInterval;
#if TIMER_MATCH
	LPC_TMR32B0->EMR &= ~(0xFF<<4);
	LPC_TMR32B0->EMR |= ((0x3<<4)|(0x3<<6)|(0x3<<8)|(0x3<<10));	/* MR0/1/2/3 Toggle */
#else
	/* Capture 0 on rising edge, interrupt enable. */
	LPC_TMR32B0->CCR = (0x1<<0)|(0x1<<2);
#endif
    LPC_TMR32B0->MCR = 3;			/* Interrupt and Reset on MR0 */

    /* Enable the TIMER0 Interrupt */
    NVIC_EnableIRQ(TIMER_32_0_IRQn);
  }
  else if ( timer_num == 1 )
  {
    /* Some of the I/O pins need to be clearfully planned if
    you use below module because JTAG and TIMER CAP/MAT pins are muxed. */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<10);
#ifdef __JTAG_DISABLED
    LPC_IOCON->JTAG_TMS_PIO1_0  &= ~0x07;	/*  Timer1_32 I/O config */
    LPC_IOCON->JTAG_TMS_PIO1_0  |= 0x03;	/* Timer1_32 CAP0 */
    LPC_IOCON->JTAG_TDO_PIO1_1  &= ~0x07;	
    LPC_IOCON->JTAG_TDO_PIO1_1  |= 0x03;	/* Timer1_32 MAT0 */
    LPC_IOCON->JTAG_nTRST_PIO1_2 &= ~0x07;
    LPC_IOCON->JTAG_nTRST_PIO1_2 |= 0x03;	/* Timer1_32 MAT1 */
    LPC_IOCON->ARM_SWDIO_PIO1_3  &= ~0x07;
    LPC_IOCON->ARM_SWDIO_PIO1_3  |= 0x03;	/* Timer1_32 MAT2 */
#endif
    LPC_IOCON->PIO1_4 &= ~0x07;
    LPC_IOCON->PIO1_4 |= 0x02;		/* Timer0_32 MAT3 */

    timer32_1_counter = 0;
    timer32_1_capture = 0;
    LPC_TMR32B1->MR0 = TimerInterval;
#if TIMER_MATCH
	LPC_TMR32B1->EMR &= ~(0xFF<<4);
	LPC_TMR32B1->EMR |= ((0x3<<4)|(0x3<<6)|(0x3<<8)|(0x3<<10));	/* MR0/1/2 Toggle */
#else
	/* Capture 0 on rising edge, interrupt enable. */
	LPC_TMR32B1->CCR = (0x1<<0)|(0x1<<2);
#endif
    LPC_TMR32B1->MCR = 3;			/* Interrupt and Reset on MR0 */

    /* Enable the TIMER1 Interrupt */
    NVIC_EnableIRQ(TIMER_32_1_IRQn);
  }
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
