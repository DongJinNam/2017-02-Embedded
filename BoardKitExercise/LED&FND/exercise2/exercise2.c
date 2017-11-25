#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL

unsigned char FND_DATA[19]={0x3f, 0x06, 0x5b, 0x4f,
0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e,
0x79, 0x71, 0x80, 0x40, 0x08};

unsigned char fnd_sel[4] = {
	0x01,0x02,0x04,0x08
};

void main() {
	int i = 0;
	unsigned short val = 0x0000;
	unsigned short temp = 0x0000;
	DDRC = 0xff; // FND output port
	DDRG = 0x0f; // FND output port
	
	for (; val < 10000; val++) {
		temp = val;
		for (i = 0; i < 4; i++) {
			if (i == 2)
				PORTC = (FND_DATA[temp % 10] | 0x80);
			else
				PORTC = FND_DATA[temp % 10];
			PORTG = fnd_sel[i];
			_delay_ms(40);
			temp /= 10;
		}			
	}
	// exit 
	PORTC = FND_DATA[0];
	PORTG = 0x01;	
}

