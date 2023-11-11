/*
 * neo6m_buffer.h
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */

#ifndef NEO6M_BUFFER_H_
#define NEO6M_BUFFER_H_

#include "main.h"


typedef struct {
	uint8_t *buffer;
	uint32_t buff_size;
	uint32_t input_pt;
	uint32_t output_pt;
	uint8_t flags;
}neo6m_handle_t;



typedef enum {
	NEO6M_FLAG_ERROR	= 0x0,
	NEO6M_FLAG_INIT		= 0x1,
	NEO6M_FLAG_MALLOC	= 0x2
}neo6m_flag_e;


typedef enum {
	WRITE_SUCCESS	= 0,
	READ_SUCCESS	= 1,
	NO_FREE_SPACE	= 2,
	NO_UNREAD_DATA	= 3,
	HANDLE_INVALID	= 4
}neo6m_status_e;



void neo6m_buffer_init(void);


neo6m_status_e neo6m_uart_buffer_write(neo6m_handle_t *pH_neo6m, uint8_t ch);

char neo6m_buffer_get_char(void);

uint8_t neo6m_buffer_isempty(void);
uint8_t neo6m_buffer_isfull(void);




#endif /* NEO6M_BUFFER_H_ */
