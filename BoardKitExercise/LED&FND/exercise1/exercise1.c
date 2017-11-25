#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL

void main() {
	unsigned char value = 0x80;
	unsigned char state = 0x01; // 0x01 : descend, 0x00 : ascend;
	DDRA = 0xff; // LED output port

	for(;;) {
		PORTA = value;
		_delay_ms(1000);

		if (state & 0x01) {
			value >>= 1;
			if (value == 0x00) {
				state &= 0x00;
				value = 0x01;
			}
		}
		else {
			value <<= 1;
			if (value == 0x80) {
				state |= 0x01;
				value = 0x80;
			}
		}
	}	
}

