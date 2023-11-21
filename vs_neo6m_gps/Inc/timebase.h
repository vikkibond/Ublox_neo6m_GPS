/*
 * timebase.h
 *
 *  Created on: 4 Nov 2023
 *      Author: vikrantshah
 *
 *  SysTick->LOAD	: Reload value register (SYST_RVR) for system clock of 	16MHz
 *  				  			16000000 -> 1 sec
 *  				  			16000 	 -> 1 milli sec
 *
 *  SysTick->VAL	: Current value register (SYST_CVR)
 *  				  Returns current value of counter but writing any value will reset it to 0.
 *  				  we initialize time base by resetting counter to 0.
 *
 *  SysTick->CTRL 	: Control and status register (SYST_CSR)
 *  				  bit 0  = ENABLE
 *  				  bit 1  = TICKINT
 *  				  bit 2  = CLKSOURCE
 *  				  bit 16 = COUNTFLAG
 */

#ifndef TIMEBASE_H_
#define TIMEBASE_H_

#include <stdint.h>
#include "stm32f4xx.h"


uint32_t get_tick(void);

void delay(uint32_t delay);

void timebase_init(void);


#define MAX_DELAY	0xFFFFFFFF


#endif /* TIMEBASE_H_ */
