#include <stm32f1xx.h>
#include <core_cm3.h>
#include <stdlib.h>
#include "stupid_delay.h"
#include "UART.h"
#include "simple_buffer.h"
#include <stdio.h>

#define LED_port GPIOC
#define LED_Blue (1 << 8)
#define GPIO_setBit(PORT, PIN) (PORT->BSRR |= PIN)
#define GPIO_clearBit(PORT, PIN) (PORT->BSRR |= (PIN << 0x10))

static void init_blue_led() {
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	LED_port->CRH |= GPIO_CRH_MODE8_0;
	LED_port->CRH &= ~GPIO_CRH_CNF8;
}

static void configure_clock() {
/*	RCC->CR |= (1 << 16); //External high speed clock enabled;
	while(!(RCC->CR & (1 << 17))); //External high-speed clock ready flag check

	RCC->CFGR |= 1 << 17; //LSB of division factor PREDIV1
	RCC->CFGR |= 1 << 16; //PLL entry clock source => clock from prediv1
	RCC->CFGR |= (0b0100 << 18); //PLL multiplication factor => x6

	RCC->CR |= (1 << 24); //PLL enabled;
	while(!(RCC->CR & (1 << 25))); //PLL clock ready flag

	RCC->CR &= ~(1 << 19); //Clock security system disabled
	RCC->CR &= ~(1 << 18); //External high-speed clock not bypassed

	RCC->CR |= 1 << 0; //Internal high-speed clock enabled
	while(!(RCC->CR & (1 << 1))); //Internal high-speed clock ready flag

	RCC->CFGR &= ~0b11; //select PLL as input
	RCC->CFGR |= (0b10);
	while(!(RCC->CFGR & 0b1000)); //wait until PLL is selected
*/
	SystemCoreClock = 8000000;
}

extern struct simple_buffer UART2_transmit_buffer;

int main(void){
	char buffer[128];
	configure_clock();
	init_blue_led();
	delay_init();
	UART_1_init();
	UART_2_init();
	esp8266_Init();
	uint16_t year;
	uint8_t day, month, hour, minute, second;
	esp8266_GetDate(&day, &month, &year, &hour, &minute, &second);
	while(1){
		GPIO_setBit(LED_port, LED_Blue);
		delay_ms(1000);
		GPIO_clearBit(LED_port, LED_Blue);
		delay_ms(1000);
		sprintf(buffer, "%d/%d/%d\r\n", (int) day, (int) month, (int) year);
		buffer_set_text(&UART1_transmit_buffer, buffer, strlen(buffer));
		sprintf(buffer, "%d:%d:%d\r\n", (int) hour, (int) minute, (int) second);
		buffer_set_text(&UART1_transmit_buffer, buffer, strlen(buffer));
		UART_1_transmit();
	}
	return 0;
}

