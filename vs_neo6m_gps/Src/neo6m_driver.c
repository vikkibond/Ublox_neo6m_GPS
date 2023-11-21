/*
 * neo6m_driver.c
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */
#include "neo6m_driver.h"

/*
 * Whenever uart1 rxne interrupt occurs, received char will be stored in this handle
 */
extern neo6m_handle_t h_neo6m;


static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t neo6m_baud);
static void neo6m_uart_pin_config(void);


static void neo6m_usart_write(uint8_t ch);


static void ublox_calculate_checksum( uint8_t *pkt, int len);
static int ublox_send_packet( uint8_t* pkt, int len, int timeout);



/**************************************************************************************
 *------------------------------------------------------------------------------------
 *
 *                	 	USART1 (for neo6m module) configuration:
 *
 *------------------------------------------------------------------------------------
 **************************************************************************************/

void neo6m_uart_init(void)
{
	neo6m_uart_pin_config();

}


static void neo6m_uart_pin_config(void)
{
	RCC->AHB1ENR |= UART1_GPIOAEN;

	GPIOA->MODER &= ~( 0x3U << 18 );
	GPIOA->MODER |=  ( 0x2U << 18 );

	GPIOA->MODER &= ~( 0x3U << 20 );
	GPIOA->MODER |=  ( 0x2U << 20 );

	GPIOA->AFR[1]	&= ~( 0xFU << 4 );
	GPIOA->AFR[1]	|=  ( 0x7U << 4 );

	GPIOA->AFR[1]	&= ~( 0xFU << 8 );
	GPIOA->AFR[1]	|=  ( 0x7U << 8 );


	RCC->APB2ENR |= UART1EN;

	USART1->CR1 &= ~(UART1_CR1_UE);		// disable USART before configuring parameters

	USART1->BRR = compute_uart_bd(APB2_CLK, NEO6M_UART_BAUDRATE);

	USART1->CR1 = ( UART1_CR1_TE | UART1_CR1_RE | UART1_CR1_RXNEIE );


	USART1->CR1 |= UART1_CR1_UE;			// Finally enable USART peripheral

	NVIC_EnableIRQ( USART1_IRQn );
}




static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t neo6m_baud)
{
	return((periph_clk + (neo6m_baud/2U))/neo6m_baud);
}



static void neo6m_usart_write(uint8_t ch)
{
	while( !(USART1->SR & UART1_SR_TXE));
	USART1->DR = (ch & 0xFF);
}


void neo6m_uart_rxne_callback(void)
{
	if( (USART1->SR & UART1_SR_RXNE) && (USART1->CR1 & UART1_CR1_RXNEIE) )
	{
		char ch = (USART1->DR) & 0xFF;
		neo6m_uart_buffer_write(&h_neo6m, ch);

		/* This is just to help us see incoming data(char) from neo6m module onto serial monitor */
		debug_uart_send(ch);
	}
}


void USART1_IRQHandler(void)
{
	neo6m_uart_rxne_callback();
}



/**************************************************************************************
 *------------------------------------------------------------------------------------
 *
 *                	 	UBLOX Command related functions:
 *
 *------------------------------------------------------------------------------------
 **************************************************************************************/


/*
  	  	  	  	  	  	  UBLOX PROPRIETERY PACKET FORMAT

<--1 Byte--><--1 Byte--> <-1 Byte-> <-1 Byte-><---2 Byte--->		     <--2 Byte-->
 ____________________________________________________________________________________
|Sync Char 1|Sync Char 2|Class     |ID        |Length       |Payload    |CK_A | CK_B |
|___________|___________|__________|__________|_____________|___________|_____|______|

|<-- 0xB5-->|<-- 0x62-->|<------RANGE over Which Checksum Is CALC------>|<-checksum->|


Length is Little Endian
i.e. int i = 0x01234567;
will be stored as 67452301

Checksum Formula:
 	 CK_ values are 8-Bit unsigned integers
 	 CK_A = 0, CK_B = 0

 	 For( I = 0; I < N; I++ )
 	 {
     	 CK_A = CK_A + Buffer[I]
     	 CK_B = CK_B + CK_A
 	 }
*/


const char UBLOX_HEADER[] 	= { 0xB5, 0x62 };		// SYNC Char
const char CFG_MSG[]		= { 0x06, 0x01 };
const char CFG_RATE[]		= { 0x06, 0x08 };


/*
 * This is NMEA message class and id that we need to send via packed if we want to
 * configure anything related to particular NMEA sentence type
 */
uint8_t msg_class_id [][2] = {
			{0xF0,	0x00},		// GGA
			{0xF0,	0x01},		// GLL
			{0xF0,	0x02},		// GSA
			{0xF0,	0x03},		// GSV
			{0xF0,	0x04},		// RMC
			{0xF0,	0x05},		// VTG
			{0xF0,	0x06},		// GRS
			{0xF0,	0x07},		// GST
			{0xF0,	0x08},		// ZDA
			{0xF0,	0x09},		// GBS
			{0xF0,	0x0A},		// DTM
			{0xF0,	0x0D},		// GNS
			{0xF0,	0x0F},		// VLW
			{0xF0,	0x40},		// GPQ
			{0xF0,	0x41},		// TXT
			{0xF0,	0x42},		// GNQ
			{0xF0,	0x43},		// GLQ
			{0xF0,	0x44},		// GBQ
};



/*
 * This function will calculate the checksum CK_A and CK_B of the given packet and
 * at the second last index and last index of the packet Array stores the calculated value
 * @param1 : pointer to the packet array
 * @param2 : total length of the packet
 */
static void ublox_calculate_checksum( uint8_t *pkt, int len)
{
	uint8_t temp_ck_a = 0;
	uint8_t temp_ck_b = 0;

	for( int i = 2; i < (len - 2); i++) {
		temp_ck_a += pkt[i];
		temp_ck_b += temp_ck_a;
	}

	pkt[len-2] = (temp_ck_a & 0xFF);
	pkt[len-1] = (temp_ck_b & 0xFF);
}



static int ublox_send_packet( uint8_t* pkt, int len, int timeout)
{
	for( int i = 0; i < len; i++)
	{
		neo6m_usart_write((uint8_t)(pkt[i]));
	}
	return 1;
}


/*
 *  This function sets the rate of data receive from gps module in millisecond
 *  It only takes 16-bit (8bit + 8bit) value in Little Endian format.
 *  Refer to CFG_RATE page.150
 */
void ublox_set_message_rate(uint16_t rate_ms)
{
	/*
	 * If 1000 millisecond = 0x03E8 = 0xb 0011 1110 1000
	 * little endian format  first part  : 	0xE8 	= 	0b 1110 1000
	 * 						 second part : 	0x03 	= 	0b 0000 0011
	 */
	uint8_t rate_ms_0 = rate_ms % 256;			// 1000 % 256 = 232 = 0b 1110 1000
	uint8_t rate_ms_1 = rate_ms / 256;			// 1000 / 256 = 3	= 0b 0000 0011

	uint8_t packet[] =
	{
			UBLOX_HEADER[0],
			UBLOX_HEADER[1],
			CFG_RATE[0],
			CFG_RATE[1],
			0x06,			// length	(little endian)
			0x00,			// length
			rate_ms_0,		// payload 1 - measure rate	(little endian)
			rate_ms_1,		// payload 2 - measure rate
			0x01,			// payload 3 - nav rate		(little endian)
			0x00,			// payload 4 - nav rate
			0x00,			// payload 5 - time reference	(little endian)
			0x00,			// payload 6 - time reference
			0x00,			// CK_A	(temporary check sum first)
			0x00,			// CK_B	(temporary check sum second)
	};

	int packet_len	= sizeof(packet);
	ublox_calculate_checksum(packet, packet_len);

	ublox_send_packet(packet, packet_len, 100);

}



/*
 *  This command will make UBLOX to disable sending of all the NMEA sentence (which class_id we have stored
 * 	on "msg_class_id[]" array) from gps module to our MCU
 */
void ublox_disable_all_nmea(void)
{
	uint8_t packet[] =
	{
			UBLOX_HEADER[0],	// UBLOX packet protocol header (0xB5) (ublox manual p.g 96)
			UBLOX_HEADER[1],	// 								(0x62)
			CFG_MSG[0],			// UBLOX configure class		(ublox manual p.g 117)
			CFG_MSG[1],			// UBLOX configure id
			0X03,				// Length	(3 pay load size)
			0x00,				// Length	(2-bytes size little endian format)
			0x00,				// payload-1 msg_cls_id[0]
			0x00,				// payload-2 msg_cls_id[1]
			0x00,				// payload-3 0 = disable, 1 = enable message
			0x00,				// CK_A	(temporary check sum first)
			0x00,				// CK_b (temporary check sum second)
	};

	uint8_t msg_cls_id_len = (sizeof(msg_class_id)/sizeof(*msg_class_id));	// 18 total no. of msg_cls_id
	uint8_t msg_cls_id_elements = sizeof(*msg_class_id);					// 2 elements on single msg_cls_id

	uint8_t packet_len =  sizeof(packet);
	uint8_t payLoadOffset = 6;							// pay load is in packer[6] (7 and 8)


	for( int i = 0; i < msg_cls_id_len; i++) 			// runs loop 18 times
	{
		for( uint8_t j = 0; j < msg_cls_id_elements; j++)
		{
			packet[payLoadOffset + j] = msg_class_id[i][j];
		}
		packet[packet_len - 2] = 0x00;
		packet[packet_len - 1] = 0x00;

		// This for loop will calculate the check sum
		for( uint8_t k = 0; k < (packet_len - 4); k++)
		{
			packet[packet_len - 2] += packet[ 2+k ];
			packet[packet_len - 1] += packet[ packet_len - 2 ];
		}

		ublox_send_packet(packet, packet_len, 100);
	}
}



/*
 *  This command will enable sending of particular NMEA sentence from GPS module to our MCU
 */
void ublox_enable_nmea_sentence(ublox_sentence_e nmea)
{
	uint8_t payload_len[]		= { 0x03, 0x00 };	// little endian format 0x0003 = 0x0300

	uint8_t cls_id[]			= { msg_class_id[nmea][0],
									msg_class_id[nmea][1]	};

	uint8_t enable_sentence		= 0x01;
	uint8_t ck_a = 0, ck_b = 0;

	uint8_t packet[] =
	{
			UBLOX_HEADER[0], 	UBLOX_HEADER[1],
			CFG_MSG[0],			CFG_MSG[1],
			payload_len[0], 	payload_len[1],
			cls_id[0],			cls_id[1],			enable_sentence,	// msg_cls, msg_id and enable bit sent as  payload
			ck_a,				ck_b
	};

	int packet_len = sizeof(packet);

	ublox_calculate_checksum(packet, packet_len);

	ublox_send_packet(packet, packet_len, 100);
}









