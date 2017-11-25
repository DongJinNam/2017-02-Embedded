#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#define ON 		1
#define OFF		0

volatile int state = ON;
volatile int state2 = OFF;
volatile int hold = OFF;
volatile int count = 0x00;
volatile int start = OFF;
unsigned short val = 0x0000;
unsigned short cur = 0x0000;
const int hz_arr[8] = {17,43,66,77,97,114,129,137};
unsigned char FND_DATA[19]={0x3f, 0x06, 0x5b, 0x4f,
0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e,
0x79, 0x71, 0x80, 0x40, 0x08};

unsigned char fnd_sel[4] = {
	0x01,0x02,0x04,0x08
};

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
	if (state2 == ON) {
		state2 = OFF;		
	}
	else {
		state2 = ON;
	}
	start = ON;

	++count;	
	if (count >= 0x08) count = 0;
	_delay_ms(300);
}

ISR(INT5_vect)
{	
	if (hold == OFF) 
		hold = ON;
	else
		hold = OFF;
	if (state2 == OFF) cur = 0x00;
	_delay_ms(300);
}

int main()
{
	int i = 0;	
	unsigned short temp = 0x0000;
	DDRC = 0xff; // FND output port
	DDRG = 0x0f; // FND output port

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

	while(1) {

		if (start == OFF) continue;

		for (; cur < 10000; cur++) {
			if (hold == OFF) {
				val = cur;		
			}
			temp = val;
			for (i = 0; i < 4; i++) {
				if (i == 2)
					PORTC = (FND_DATA[temp % 10] | 0x80);
				else
					PORTC = FND_DATA[temp % 10];
				PORTG = fnd_sel[i];
				_delay_ms(1);
				temp /= 10;
			}						
		}
	}

}
