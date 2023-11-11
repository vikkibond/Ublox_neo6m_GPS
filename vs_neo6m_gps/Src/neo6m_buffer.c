/*
 * neo6m_buffer.c
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */


#include "neo6m_buffer.h"

/******************** NEO6M UART buffer related variables ********************/

#define NEO6M_BUFF_SIZE	200
uint8_t neo6m_buff[NEO6M_BUFF_SIZE];

uint8_t *p_neo6m_buff = neo6m_buff;

neo6m_handle_t h_neo6m;
/*****************************************************************************/


static neo6m_flag_e neo6m_buffer_config(neo6m_handle_t *p_neo6m_handle, uint32_t buffer_size, void *p_buff);

static neo6m_status_e neo6m_uart_buffer_read(neo6m_handle_t *pH_neo6m, char *dest_data);

static uint32_t neo6m_buffer_get_free_space(neo6m_handle_t *p_neo6m_h);
static uint32_t neo6m_buffer_get_unread_data_cnt(neo6m_handle_t *ph_neo6m);



/**************************************************************************************
 *------------------------------------------------------------------------------------
 *						neo6m_buffer related functions:
 *------------------------------------------------------------------------------------
 **************************************************************************************/


void neo6m_buffer_init(void)
{
	neo6m_buffer_config(&h_neo6m, NEO6M_BUFF_SIZE, p_neo6m_buff);
}



static neo6m_flag_e neo6m_buffer_config(neo6m_handle_t *p_neo6m_handle, uint32_t buffer_size, void *p_buff)
{
	memset(p_neo6m_handle, 0, sizeof(neo6m_handle_t));

	p_neo6m_handle->buff_size 	= buffer_size;
	p_neo6m_handle->buffer		= p_buff;


	/* if p_neo6m_handle->buffer is NULL, which mean buffer cannot be allocated,
	 * Therefore we need to allocate dynamic memory */
	if( ! p_neo6m_handle->buffer)
	{
		p_neo6m_handle->buffer = (uint8_t*)malloc(buffer_size * sizeof(uint8_t));

		/* If we still cannot allocate dynamic memory will return ERROR FLAG */
		if( ! p_neo6m_handle->buffer)
		{
			p_neo6m_handle->buff_size = 0;
			return NEO6M_FLAG_ERROR;
		}

		else
		{
			p_neo6m_handle->flags |= NEO6M_FLAG_MALLOC;
		}
	}

	p_neo6m_handle->flags |= NEO6M_FLAG_INIT;

	return p_neo6m_handle->flags;
}



static uint32_t neo6m_buffer_get_free_space(neo6m_handle_t *ph_neo6m)
{
	uint32_t free_space	= 0;
	uint32_t in 		= ph_neo6m->input_pt;
	uint32_t out 		= ph_neo6m->output_pt;

	if(ph_neo6m == NULL)		return 0;

	if(in == out)				free_space = (ph_neo6m->buff_size);
	if(out > in)				free_space = (out - in);
	if(in > out)				free_space = (ph_neo6m->buff_size - (in - out));

	return free_space - 1;
}


/*
 * This function is called by "neo6m_uart_rxne_callback" when RXNE interrupt is triggered
 * this function stores single "char" from "USART1->DR" into neo6m buffer
 */
neo6m_status_e neo6m_uart_buffer_write(neo6m_handle_t *pH_neo6m, uint8_t ch)
{
	uint32_t free_space = neo6m_buffer_get_free_space(&h_neo6m);

	if(pH_neo6m == NULL)
		return HANDLE_INVALID;

	if(free_space == 0)
		return NO_FREE_SPACE;

	if(pH_neo6m->input_pt >= pH_neo6m->buff_size)
		pH_neo6m->input_pt = 0;

	pH_neo6m->buffer[ pH_neo6m->input_pt ] = (uint8_t)ch;
	pH_neo6m->input_pt++;

	if(pH_neo6m->input_pt >= pH_neo6m->buff_size)
		pH_neo6m->input_pt = 0;

	return WRITE_SUCCESS;
}


uint8_t neo6m_buffer_isfull(void)
{
	if(neo6m_buffer_get_unread_data_cnt(&h_neo6m))
		return FALSE;

	else
		return TRUE;
}


static uint32_t neo6m_buffer_get_unread_data_cnt(neo6m_handle_t *ph_neo6m)
{
	uint32_t unread_data_cnt = 0;

	uint32_t in 	= ph_neo6m->input_pt;
	uint32_t out 	= ph_neo6m->output_pt;

	if(ph_neo6m == NULL)		return 0;

	if(out == in)				return unread_data_cnt = 0;
	if(out > in)				unread_data_cnt = (ph_neo6m->buff_size - (out - in));
	if(in > out)				unread_data_cnt = (in - out);

	return unread_data_cnt - 1;
}


/* This function returns TRUE if there is no unread data on a buffer
 * else if there is unread data present, returns FALSE
 */
uint8_t neo6m_buffer_isempty(void)
{
	if(neo6m_buffer_get_unread_data_cnt(&h_neo6m))
		return 0;
	return 1;
}



static neo6m_status_e neo6m_uart_buffer_read(neo6m_handle_t *pH_neo6m, char *dest_data)
{
	uint32_t read_cnt 	= 0;
	uint32_t unread_data_cnt = neo6m_buffer_get_unread_data_cnt(pH_neo6m);

	if( pH_neo6m == NULL )
		return HANDLE_INVALID;

	if(unread_data_cnt == 0)
		return NO_UNREAD_DATA;

	if(pH_neo6m->output_pt >= pH_neo6m->buff_size)
		pH_neo6m->output_pt = 0;


	*dest_data = pH_neo6m->buffer[ pH_neo6m->output_pt++ ];
	read_cnt++;

	if(pH_neo6m->output_pt >= pH_neo6m->buff_size)
		pH_neo6m->output_pt = 0;

	return READ_SUCCESS;
}



char neo6m_buffer_get_char(void)
{
	char ch = 0;
	neo6m_uart_buffer_read(&h_neo6m, &ch);
	return ch;
}


