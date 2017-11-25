#include <avr/io.h>
#include <util/delay.h>
#define CDS_VALUE 871

void init_adc();
unsigned short read_adc();
void show_adc(unsigned short value);
void display_FND(unsigned short value);

int main(){
	unsigned short value;
	DDRA = 0xff;
	DDRC = 0xff;
	DDRG = 0x0f;
	init_adc();

	while(1){
		value = read_adc();
		display_FND(value);
		show_adc(value);		
	}
}

void init_adc(){
	ADMUX = 0x20;	/* REFS(1:0) = "00": AREF(+5V) �������� ���
					 * ADLAR = '0': ����Ʈ ������ ����
					 * MUX(4:0) = "00000": ADC0 ���, �ܱ� �Է�
					 */
	ADCSRA = 0X87;	/* ADEN = '1': ADC�� Enable
					 * ADFR = '0': single conversion ���
					 * ADPS(2:0) = "111": ���������Ϸ� 128����
					 */
}

unsigned short read_adc(){
	unsigned char adc_low, adc_high;
	unsigned short value = 0x0000;
	ADCSRA |= 0x40;	/* ADSC = '1': ADC start conversion */
	while((ADCSRA & 0x10) != 0x10);	/* ADC ��ȯ �Ϸ� �˻� */
	adc_low = ADCL;	/* ��ȯ�� low�� high�� �о���� */
	adc_high = ADCH;
	value |= (adc_high << 2); /* 16��Ʈ ������ ���� */
	value |= (adc_low >> 6); /* 16��Ʈ ������ ���� */
	return value;
}

void show_adc(unsigned short value){
	DDRA = 0xff;
	/* ���ذ��� ���� LED on, off */
	if (value >= CDS_VALUE + 4 * 18)
		PORTA = 0x00;
	else if (value >= CDS_VALUE + 3 * 18)
		PORTA = 0x80;
	else if (value >= CDS_VALUE + 2 * 18)
		PORTA = 0xC0;	
	else if (value >= CDS_VALUE + 1 * 18)
		PORTA = 0xE0;	
	else if (value >= CDS_VALUE)
		PORTA = 0xF0;	
	else if (value >= CDS_VALUE - 1 * 18)
		PORTA = 0xF8;	
	else if (value >= CDS_VALUE - 2 * 18)
		PORTA = 0xFC;	
	else if (value >= CDS_VALUE - 3 * 18)
		PORTA = 0xFE;	
	else if (value >= CDS_VALUE - 4 * 18)
		PORTA = 0xFF;	
}

void display_FND(unsigned short value) {
	unsigned char digit[12] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67, 0x40, 0x00 };
	unsigned char fnd_sel[4] = { 0x01, 0x02, 0x04, 0x08 };
	unsigned short num[4];
	int i;

	num[3] = value / 1000;
	value %= 1000;
	num[2] = value / 100;
	value %= 100;
	num[1] = value / 10;
	value %= 10;
	num[0] = value % 10;
			
	for (i = 0; i < 4; i++) {
		PORTC = digit[num[i]];
		PORTG = fnd_sel[i];
		_delay_ms(5);
	}	
}
