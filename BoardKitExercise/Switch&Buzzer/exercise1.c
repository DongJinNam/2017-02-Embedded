#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#define ON 		1
#define OFF		0

volatile int state = ON;
volatile int state2 = ON;
volatile int count = 0x00;
const int hz_arr[8] = {17,43,66,77,97,114,129,137};

ISR(TIMER0_OVF_vect)
{
	// state2 is controlled by PE5.
	if (state2 == ON) {
		if(state == ON)
		{
			PORTB = 0x00;
			state = OFF;
		}
		else
		{
			PORTB = 0x10;
			state = ON;
		}
	}				
	TCNT0 = hz_arr[count];
}

ISR(INT4_vect)
{
	++count;
	if (count >= 0x08) count = 0;
	_delay_ms(400);
}

ISR(INT5_vect)
{	
	if (state2 == ON) {
		state2 = OFF;
	}
	else {
		state2 = ON;
	}
	_delay_ms(400);
}

int main()
{
	DDRB = 0x10;
	PORTB = 0x10;

	DDRE = 0xcf;
	EICRB = 0x02;
	EIMSK = 0x30;
	SREG |= 0x80;

	TCCR0 = 0x03;
	TIMSK = 0x01;
	TCNT0 = hz_arr[count];
	sei();
	while(1);
}
