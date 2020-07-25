#ifndef _UART_HPP_
#define _UART_HPP_

#include <stm32f1xx_hal_dma.h>
#include <stm32f1xx_hal_gpio.h>
#include <stm32f1xx_hal_rcc.h>
#include <stm32f1xx_hal_uart.h>
#include <stm32f1xx_hal_cortex.h>
#include <math.h>

#include "simple_buffer.h"
#include "esp8266.h"


/**
 * @addtogroup RFID_System libraries
 * @{
 */

/**
 * @defgroup UART
 * @brief UART library.
 * @{
 */

HAL_StatusTypeDef UART_2_init();

/**
 * @brief - Transmit UART2 routine.
 * @{ Due to the buffers on UART2, the transmit happens in time of
 * interrupt by empty transmit register. This function just sets
 * bit in USART control register, to inoke interrupt on empty register.
 */
void UART_2_transmit();

/**
 * @brief sets new speed of UART
 */
void UART_2_ChangeSpeed(unsigned int speed);

extern struct simple_buffer UART2_transmit_buffer;
extern struct simple_buffer UART2_receive_buffer;

#define ready_bit 1 << 0
#define error_bit 1 << 1

#endif
