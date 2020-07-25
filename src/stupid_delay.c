#include "stupid_delay.h"
uint64_t __IO delay_val;

void delay_init(){
	if(SysTick_Config(SystemCoreClock / 100000UL)){
		while(1);
	}
}

void delay_ms(uint32_t value){	//unit of value = [ms]
	delay_val = value * 1000;
	while(delay_val != 0);
}

void delay_us(uint32_t value){
	delay_val = value - (value % 10);
	while(delay_val != 0);
}

void SysTick_Handler(void){
	if(delay_val != 0) delay_val -= 10;
}
