#include "UART.h"

static UART_HandleTypeDef uart_1_handler, uart_2_handler;

struct simple_buffer UART2_transmit_buffer;
struct simple_buffer UART2_receive_buffer;
struct simple_buffer UART1_transmit_buffer;
struct simple_buffer UART1_receive_buffer;

void USART1_IRQHandler(void)
{
	uint8_t data = 0;
	int8_t ret = 0;

	if (USART1->SR & (USART_SR_ORE | USART_SR_RXNE)) {
		data = (uint8_t)USART1->DR;
		buffer_set_byte(&UART1_receive_buffer, data);
	}

	if (USART1->SR & (USART_SR_TC | USART_SR_TXE)) {
		ret = buffer_get_byte(&UART1_transmit_buffer, &data);
		if (!ret)
			USART1->DR = data;
		else
			USART1->CR1 &= ~USART_CR1_TXEIE;
		USART1->SR &= ~USART_SR_TC;
	}
}



void USART2_IRQHandler(void)
{
	uint8_t data = 0;
	int8_t ret = 0;

	if (USART2->SR & (USART_SR_ORE | USART_SR_RXNE)) {
		data = (uint8_t)USART2->DR;
		esp8266_CheckInput(data);
		buffer_set_byte(&UART2_receive_buffer, data);
	}

	if (USART2->SR & (USART_SR_TC | USART_SR_TXE)) {
		ret = buffer_get_byte(&UART2_transmit_buffer, &data);
		if (!ret) {
			USART2->DR = data;
		} else {
			USART2->CR1 &= ~USART_CR1_TXEIE;
		}
		USART2->SR &= ~USART_SR_TC;
	}
}

void UART_2_ChangeSpeed(unsigned int speed)
{
	uint16_t mantissa = 0;
	uint16_t fraction = 0;
	uint16_t value = 0;
	double acc = 24000000.0/(16.0 * (double)speed);
	double fraction_d = (acc - floor(acc));
	mantissa = acc - fraction_d;
	fraction = floor(16.0*fraction_d + 0.5);
	if (fraction >= 16) {
		mantissa += 1;
		fraction -= 16;
	}
	value = (mantissa << 4) | fraction;
	USART2->CR1 &= ~USART_CR1_UE;
	USART2->BRR = value;
	USART2->CR1 |= USART_CR1_UE;
}

HAL_StatusTypeDef UART_1_init()
{
	HAL_StatusTypeDef ret;
	UART_InitTypeDef init;
	init.BaudRate = 9600;
	init.WordLength = UART_WORDLENGTH_8B;
	init.StopBits = UART_STOPBITS_1;
	init.Parity = UART_PARITY_NONE;
	init.Mode = UART_MODE_TX_RX;
	init.HwFlowCtl = UART_HWCONTROL_NONE;
	init.OverSampling = UART_OVERSAMPLING_16;

	uart_1_handler.Init = init;
	uart_1_handler.Instance = USART1;

	memset(&UART1_receive_buffer, 0, sizeof(struct simple_buffer));
	memset(&UART1_transmit_buffer, 0, sizeof(struct simple_buffer));

	USART1->CR1 = 0;
	ret = HAL_UART_Init(&uart_1_handler);
	USART1->CR1 |= USART_CR1_RXNEIE;
	USART1->CR1 |= USART_CR1_TCIE;
	USART1->CR1 |= USART_CR1_TXEIE;
	return ret;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef init_gpio;

	if (huart->Instance == USART1)
		init_gpio.Pin = GPIO_PIN_9;

	else if (huart->Instance == USART2)
		init_gpio.Pin = GPIO_PIN_2;

	init_gpio.Pull = GPIO_NOPULL;
	init_gpio.Mode = GPIO_MODE_AF_PP;
	init_gpio.Speed = GPIO_SPEED_FREQ_HIGH;

	__HAL_RCC_GPIOA_CLK_ENABLE();

	HAL_GPIO_Init(GPIOA, &init_gpio);

	if (huart->Instance == USART1)
		init_gpio.Pin = GPIO_PIN_10;

	else if (huart->Instance == USART2)
		init_gpio.Pin = GPIO_PIN_3;

	init_gpio.Pull = GPIO_PULLDOWN;
	init_gpio.Mode = GPIO_MODE_AF_INPUT;

	HAL_GPIO_Init(GPIOA, &init_gpio);

	if (huart->Instance == USART1) {
		__HAL_RCC_USART1_CLK_ENABLE();
		HAL_NVIC_EnableIRQ(USART1_IRQn);

	} else if (huart->Instance == USART2) {
		__HAL_RCC_USART2_CLK_ENABLE();
		HAL_NVIC_EnableIRQ(USART2_IRQn);
	}
}

HAL_StatusTypeDef UART_2_init()
{
	HAL_StatusTypeDef ret;
	UART_InitTypeDef init;
	init.BaudRate = 115200;
	init.WordLength = UART_WORDLENGTH_8B;
	init.StopBits = UART_STOPBITS_1;
	init.Parity = UART_PARITY_NONE;
	init.Mode = UART_MODE_TX_RX;
	init.HwFlowCtl = UART_HWCONTROL_NONE;
	init.OverSampling = UART_OVERSAMPLING_16;

	uart_2_handler.Init = init;
	uart_2_handler.Instance = USART2;

	memset(&UART2_receive_buffer, 0, sizeof(struct simple_buffer));
	memset(&UART2_transmit_buffer, 0, sizeof(struct simple_buffer));

	USART2->CR1 = 0;
	ret = HAL_UART_Init(&uart_2_handler);
	USART2->CR1 |= USART_CR1_RXNEIE;
	USART2->CR1 |= USART_CR1_TCIE;
	USART2->CR1 |= USART_CR1_TXEIE;
	return ret;
}

void UART_2_transmit(void)
{
	USART2->CR1 |= USART_CR1_TXEIE;
}

void UART_1_transmit(void)
{
	USART1->CR1 |= USART_CR1_TXEIE;
}
