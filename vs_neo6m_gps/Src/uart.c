/*
 * uart.c
 *
 *  Created on: Oct 30, 2023
 *      Author: vikki
 */

#include "uart.h"


static void uart_set_baudrate(uint32_t periph_clk,uint32_t baudrate);
static void debug_uart_write(char ch);


//int __io_putchar(int ch)
//{
//	debug_uart_write(ch);
//	return ch;
//}


void debug_uart_send(char ch)
{
	debug_uart_write(ch);
}


void debug_uart_init(void)
{
	/*Enable clock access to GPIOA*/
	RCC->AHB1ENR |= GPIOAEN;

	/*Set the mode of PA2 to alternate function mode*/
	GPIOA->MODER &=~(1U<<4);
	GPIOA->MODER |=(1U<<5);

	/*Set alternate function type to AF7(UART2_TX)*/
	GPIOA->AFR[0] |=(1U<<8);
	GPIOA->AFR[0] |=(1U<<9);
	GPIOA->AFR[0] |=(1U<<10);
	GPIOA->AFR[0] &=~(1U<<11);

	/*Enable clock access to UART2*/
     RCC->APB1ENR |=	UART2EN;

	/*Configure uart baudrate*/
     uart_set_baudrate(APB1_CLK,DBG_UART_BAUDRATE);

	/*Configure transfer direction*/
     USART2->CR1 = CR1_TE;

	/*Enable UART Module*/
     USART2->CR1 |= CR1_UE;
}


static void debug_uart_write(char ch)
{
	/*Make sure transmit data register is empty*/
	while(!(USART2->SR & SR_TXE)){}

	/*Write to transmit data register*/
	USART2->DR =(ch & 0xFF);
}


static uint16_t compute_uart_bd(uint32_t periph_clk,uint32_t baudrate)
{
	return((periph_clk + (baudrate/2U))/baudrate);
}


static void uart_set_baudrate(uint32_t periph_clk,uint32_t baudrate)
{
	USART2->BRR = compute_uart_bd(periph_clk,baudrate);
}


