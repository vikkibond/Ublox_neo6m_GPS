/*
 * timebase.c
 *
 *  Created on: 4 Nov 2023
 *      Author: vikrantshah
 */

#include "timebase.h"


#define CTRL_ENABLE		(1U << 0)
#define CTRL_TICKINT	(1U << 1)
#define CTRL_CLCKSRC	(1U << 2)
#define CTRL_COUNTFLAG	(1U << 16)


/* This value will be stored in SysTick reload register to get the desired delay */
#define ONE_SEC_LOAD	 16000000
#define ONE_MSEC_LOAD	 16000


/* if millisecond 60,000 = 60 minutes
 * unsigned 16-bit (total 65536) */
#define TIMER_MAX_DELAY		 0xFFFF


/* This simply means interrupt increases the count by 1 */
#define TICK_FREQ		 1


volatile uint32_t g_curr_tick;
volatile uint32_t g_curr_tick_p;



/*
 * Delay in second or millisecond depending on value we store on SysTick Reload Register
 */
//void delay(uint32_t delay)
//{
//	uint32_t tickstart =  get_tick();
//	uint32_t wait =  delay;
//
//	if(wait < MAX_DELAY)
//	{
//		wait += (uint32_t)TICK_FREQ;
//	}
//
//    while((get_tick() - tickstart) < wait){}
//}

void delay(uint32_t delay)
{
	volatile uint32_t cnt_start = g_curr_tick;
	volatile uint32_t cnt_end   = 0;

	if((cnt_start + delay) > TIMER_MAX_DELAY)
		cnt_end = (cnt_start + delay) - TIMER_MAX_DELAY;
	else
		cnt_end = cnt_start + delay;

	while (cnt_end != g_curr_tick);
}


//uint32_t get_tick(void)
//{
//	__disable_irq();
//	g_curr_tick_p = g_curr_tick;
//	__enable_irq();
//
//	return g_curr_tick_p;
//}


static void tick_increment(void)
{
	if(g_curr_tick > TIMER_MAX_DELAY)
		g_curr_tick = 0;

	g_curr_tick += TICK_FREQ;
}


void timebase_init(void)
{

	/* Disable global interrupts */
	__disable_irq();

    /* Load the timer with number of clock cycles per second */
	SysTick->LOAD	= ONE_MSEC_LOAD - 1;
	//SysTick->LOAD =  ONE_SEC_LOAD - 1;


    /* Clear systick current value register */
	SysTick->VAL = 0;

    /* Select internal (system) clock source */
	SysTick->CTRL = CTRL_CLCKSRC;

    /* Enable interrupt */
	SysTick->CTRL |= CTRL_TICKINT;

	/* Enable systick */
	SysTick->CTRL |=CTRL_ENABLE;


	/* Enable global interrupts */
	__enable_irq();
}



/* Global ISR for SysTick interrupt
 * Every time SysTick generates an interrupt (depending on reload register value)
 * we will count the number of interrupt to implement our delay
 */
void SysTick_Handler(void)
{
	tick_increment();
}
