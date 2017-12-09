#include "includes.h"
#define F_CPU	16000000UL	// CPU frequency = 16 Mhz
#include <avr/io.h>	
#include <avr/interrupt.h>	
#include <util/delay.h>

#define  TASK_STK_SIZE  OS_TASK_DEF_STK_SIZE     
#define CDS_VALUE 871
#define F_CPU 16000000UL // CPU 클록 값 = 16 Mhz
#define F_SCK 40000UL // SCK 클록 값 = 40 Khz
#define ATS75_ADDR 0x98 // 0b10011000, 7비트를 1비트 left shift
#define ATS75_CONFIG_REG 1
#define ATS75_TEMP_REG 0
     
OS_STK	TmpTaskStk[TASK_STK_SIZE];
OS_STK	FndTaskStk[TASK_STK_SIZE];

unsigned char FND_DATA[12] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67, 0x40, 0x00 };

unsigned char fnd_sel[4] = {
	0x01,0x02,0x04,0x08
};

OS_EVENT *mb_tmp_fnd;
OS_EVENT *mb_fnd_tmp;
//OS_EVENT *sem_tmp;
//OS_EVENT *sem_fnd;

void  TmpTask(void *pdata);
void  FndTask(void *pdata);
void write_twi_1byte_nopreset(char reg, char data);
int read_twi_2byte_nopreset(char reg);
void init_adc();
unsigned short read_adc();
void show_adc(unsigned short value);
void display_FND(int value);
void display_second();
int temperature;

int main (void)
{
	OSInit();

	OS_ENTER_CRITICAL();
	TCCR0=0x07;
	TIMSK=_BV(TOIE0);
	TCNT0=256-(CPU_CLOCK_HZ/OS_TICKS_PER_SEC/ 1024);
	OS_EXIT_CRITICAL();	
	
	init_twi_port(); // TWI 및 포트 초기화
	write_twi_1byte_nopreset(ATS75_CONFIG_REG, 0x00); // 9비트, Normal

	OSTaskCreate(TmpTask, (void *)0, (void *)&TmpTaskStk[TASK_STK_SIZE - 1], 0);
	OSTaskCreate(FndTask, (void *)0, (void *)&FndTaskStk[TASK_STK_SIZE - 1], 1);
	mb_tmp_fnd = OSMboxCreate((void *)0);
	mb_fnd_tmp = OSMboxCreate((void *)0);
			
	OSStart();
	return 0;
}

void TmpTask(void *pdata) {
	
	INT8U err;
	char ans;
	int value, i;
	char value_int, value_deci, num[4];
	unsigned int deliver;

	//OSTimeDlyHMSM(0, 0, 0, 100); // 다음 사이클을 위하여 잠시 기다림
	while (1) { // 온도값 읽어 FND 디스플레이		
		// fnd 처리 부분
		temperature = read_twi_2byte_nopreset(ATS75_TEMP_REG);

		// 값 처리 부분
		value = temperature;
		if ((value & 0x8000) != 0x8000) // Sign 비트 체크
			num[3] = 11;
		else {
			num[3] = 10;
			value = (~value) - 1; // 2’s Compliment
		}

		value_int = (char)((value & 0x7f00) >> 8);
		value_deci = (char)(value & 0x00ff);
		num[2] = (value_int / 10) % 10; num[1] = value_int % 10;
		num[0] = ((value_deci & 0x80) == 0x80) * 5;

		deliver |= (unsigned int) (num[3] << 24);
		deliver |= (unsigned int) (num[2] << 16);
		deliver |= (unsigned int) (num[1] << 8);
		deliver |= (unsigned int) (num[0]);

		//deliver = 9;
		
		OSMboxPost(mb_tmp_fnd, (void *)&deliver);
		ans = *(char *) OSMboxPend(mb_fnd_tmp, 0, &err);
		
		//display_FND(temperature);
		//display_second();
	}	
}

void FndTask(void *pdata) {
	unsigned short num[4];
	unsigned int value;
	int i, idx;
	char ans = 'y';
	INT8U err;
	DDRC = 0xff;
	DDRG = 0x0f;

	while (1) {
		value = *(unsigned int *) OSMboxPend(mb_tmp_fnd, 0, &err);
		for (i = 0; i < 4; i++) {
			idx = (value >> (8 * i)) % 0xff;
			PORTC = FND_DATA[idx];
			PORTG = fnd_sel[i];
			if (i == 1)
				PORTC |= 0x80;
			OSTimeDlyHMSM(0, 0, 0, 3);
		}
		OSMboxPost(mb_fnd_tmp, (void *)&ans);
	}
}


void init_twi_port() {
	DDRC = 0xff;
	DDRG = 0xff; // FND 출력 세팅
	PORTD = 3; // For Internal pull-up for SCL & SCK
	SFIOR &= ~(1 << PUD); // PUD = 0 : Pull Up Disable
	TWBR = (F_CPU / F_SCK - 16) / 2; // 공식 참조, bit trans rate 설정
	TWSR = TWSR & 0xfc; // Prescaler 값 = 00 (1배)
}
void write_twi_1byte_nopreset(char reg, char data) {
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);// START 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x08); // START 상태 검사, 이후 모두 상태 검사
	TWDR = ATS75_ADDR | 0; // SLA+W 준비, W=0
	TWCR = (1 << TWINT) | (1 << TWEN); // SLA+W 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x18);
	TWDR = reg; // aTS75 Reg 값 준비
	TWCR = (1 << TWINT) | (1 << TWEN); // aTS75 Reg 값 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x28);
	TWDR = data; // DATA 준비
	TWCR = (1 << TWINT) | (1 << TWEN); // DATA 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x28);
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // STOP 전송
}
int read_twi_2byte_nopreset(char reg) {
	char high_byte, low_byte;
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);// START 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x08); // START 상태 검사, 이후 ACK 및 상태 검사
	TWDR = ATS75_ADDR | 0; // SLA+W 준비, W=0
	TWCR = (1 << TWINT) | (1 << TWEN); // SLA+W 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x18);
	TWDR = reg; // aTS75 Reg 값 준비
	TWCR = (1 << TWINT) | (1 << TWEN); // aTS75 Reg 값 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x28);
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);// RESTART 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x10); // RESTART 상태 검사, 이후 ACK, NO_ACK 상태 검사
	TWDR = ATS75_ADDR | 1; // SLA+R 준비, R=1
	TWCR = (1 << TWINT) | (1 << TWEN); // SLA+R 전송
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x40);
	TWCR = (1 << TWINT) | (1 << TWEN | 1 << TWEA);// 1st DATA 준비
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x50);
	high_byte = TWDR; // 1st DATA 수신
	TWCR = (1 << TWINT) | (1 << TWEN);// 2nd DATA 준비
	while (((TWCR & (1 << TWINT)) == 0x00) || (TWSR & 0xf8) != 0x58);
	low_byte = TWDR; // 2nd DATA 수신
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); // STOP 전송
	return((high_byte << 8) | low_byte); // 수신 DATA 리턴
}

void display_FND(int value) {
	unsigned char digit[12] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67, 0x40, 0x00 };
	unsigned char fnd_sel[4] = { 0x01, 0x02, 0x04, 0x08 };
	unsigned short num[4];
	double n_value = 32.0f;
	double a_value = 1.0f;
	double cel = 0.0f;

	DDRC = 0xff;
	DDRG = 0x0f;
	
	int i;
	if ((value & 0x8000) != 0x8000) {// Sign 비트 체크
		num[3] = 11;
		a_value *= 9.0;
		cel = (double)((value & 0x7f00) >> 8);
		if (((value & 0x00ff) & 0x0080) == 0x0080) {
			cel += 0.5;
		}
		a_value *= cel;
		a_value /= 5.0;
		n_value += a_value;
	}
	else {
		num[3] = 10;
		value = (~value) - 1; // 2’s Compliment
		a_value *= 9.0;
		cel = (double)((value & 0x7f00) >> 8);
		if (((value & 0x00ff) & 0x0080) == 0x0080) {
			cel += 0.5;
		}
		a_value *= cel;
		a_value /= 5.0;
		n_value -= a_value;
	}	
	num[2] = (unsigned short) (n_value / 10) % 10; 
	num[1] = (unsigned short) n_value % 10;
	n_value *= 10;
	num[0] = (unsigned short) n_value % 10;
	for (i = 0; i < 4; i++) {
		PORTC = digit[num[i]];
		PORTG = fnd_sel[i];
		if (i == 1)
			PORTC |= 0x80;
		OSTimeDlyHMSM(0, 0, 0, 2);
	}
}

void display_second() {
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
			OSTimeDlyHMSM(0, 0, 0, 2);
			temp /= 10;
		}
	}
	// exit 
	PORTC = FND_DATA[0];
	PORTG = 0x01;
}

