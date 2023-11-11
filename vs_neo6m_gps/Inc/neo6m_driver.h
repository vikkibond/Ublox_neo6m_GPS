/*
 * neo6m_driver.h
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */

#ifndef NEO6M_DRIVER_H_
#define NEO6M_DRIVER_H_

#include "main.h"
#include "uart.h"
#include "neo6m_buffer.h"


/*Pinout:
 * UART Module	: UART1
 * UART Pins	: PA9 = TX, PA10 = RX
 *
 * NEO-6M Pin		:    STM32F4 Pin
 * TX  Pin			:    PA10(RX)
 * RX  Pin			:  	 PA9(TX)
 * GND				:	 GND
 * VCC				: 	 5V
 * */


/****************** NEO6M UART1 and PIN related macros ******************/
#define UART1_GPIOAEN			( 1U << 0 )
#define UART1EN			( 1U << 4 )

#define UART1_CR1_TE			( 1U << 3 )
#define UART1_CR1_RE			( 1U << 2 )
#define UART1_CR1_RXNEIE		( 1U << 5 )
#define UART1_CR1_UE			( 1U << 13 )

#define UART1_SR_TXE			( 1U << 7 )
#define UART1_SR_RXNE			( 1U << 5 )

#define APB2_CLK 				16000000
#define NEO6M_UART_BAUDRATE		9600


void neo6m_uart_init(void);





typedef struct {
	uint8_t uBlox_header[2];
	uint8_t class_id[2];
	uint8_t msg_id[2];
	uint8_t payload_len[2];
	uint8_t en_di;		// enable or disable
	uint8_t *payload;
	uint8_t ck_a;		// check sum a
	uint8_t ck_b;		// check sum b
}ublox_event_t;


typedef enum {
	GGA		= 0,
	GLL		= 1,
	GSA		= 2,
	GSV		= 3,
	RMC		= 4,
	VTG		= 5,
	GRS		= 6,
	GST		= 7,
	ZDA		= 8,
	GBS		= 9,
	DTM		= 10,
	GNS		= 11,
	VLW		= 12,
	GPQ		= 13,
	TXT		= 14,
	GNQ		= 15,
	GLQ		= 16,
	GBQ		= 17
}ublox_sentence_e;



void ublox_disable_all_nmea(void);
void ublox_enable_nmea_sentence(ublox_sentence_e nmea);




#endif /* NEO6M_DRIVER_H_ */
