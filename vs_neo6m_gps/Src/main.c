/*
 *
 * @file           : main.c
 * @author         : vikrant shah
 *
 * In this program:
 * 1. We Enable FPU and we use USART2 for debug purpose.
 * 2. We use USART1 to communicate with NEO6M-GPS module.
 *
 * 3. We create neo6m_driver to enable communication between module and mcu via USART1
 *    we use it to communicate with U-Blox hardware of neo6m module.
 *    we use it to enable and disable nmea sentences, set baudrate, set data rate etc.
 *
 * 4. we create neo6m_buffer to store into and read from circular buffer (char array)
 *    all the data from neo6m module are received as a long string format.
 *    we write new data received from module on a head position(called from neo6m_driver)
 *    and clear out the tail position as we read from buffer (called from nmea_parser_lib).
 *
 * 5. we create nmea_parser_lib to get data and process necessary action from neo6m_buffer
 *    actions like determining which sentence we are getting (GGA,RMC,GLL etc)
 *    presenting time, longitude, latitude, etc in a user readable (usable) format
 *	  these data are stored and accessed through "gps_data_t" structure on nmea_parser_lib.
 *
 *
 */

#include <stdint.h>
#include "main.h"

#include "fpu.h"
#include "uart.h"
#include "timebase.h"

#include "neo6m_driver.h"
#include "neo6m_buffer.h"
#include "nmea_parser_lib.h"


/* This is a handle structure to access the gps data stored in user usable format */
extern gps_data_t _gps_data;



int main(void)
{
	fpu_enable();
	debug_uart_init();

	neo6m_uart_init();
	neo6m_buffer_init();

	ublox_disable_all_nmea(); 			// disable all default nmea sentence
	delay(100);

	ublox_set_message_rate(1000);		// set messaging rate to 1 second
	delay(100);

	ublox_enable_nmea_sentence(GGA);	// enable GGA sentence
	ublox_enable_nmea_sentence(RMC);	// enable RMC sentence


    while(1)
    {
    	gps_nmea_update();

    	delay(1000);

    }

}
