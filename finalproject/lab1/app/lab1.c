#include "includes.h"
#include <avr/io.h>	
#include <avr/interrupt.h>	
#include <util/delay.h>

#define F_CPU	16000000UL	// CPU frequency = 16 Mhz
#define  TASK_STK_SIZE  OS_TASK_DEF_STK_SIZE     
#define CDS_VALUE 871
#define F_CPU 16000000UL // CPU 클록 값 = 16 Mhz
#define F_SCK 40000UL // SCK 클록 값 = 40 Khz
#define ATS75_ADDR 0x98 // 0b10011000, 7비트를 1비트 left shift
#define ATS75_CONFIG_REG 1
#define ATS75_TEMP_REG 0

// Task Stack
OS_STK	TmpTaskStk[TASK_STK_SIZE];
OS_STK	FndTaskStk[TASK_STK_SIZE];
OS_STK	BuzTaskStk[TASK_STK_SIZE];
OS_STK	LedTaskStk[TASK_STK_SIZE];

unsigned char FND_DATA[12] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67, 0x40, 0x00 };
unsigned char fnd_sel[4] = {
	0x01,0x02,0x04,0x08
};

const int hz_arr[8] = { 17,43,66,77,97,114,129,137 };
// Mailboxs, Semaphores, and so on
OS_EVENT *mb_tmp_fnd;
OS_EVENT *mb_fnd_tmp;
OS_EVENT *sem_buz_fnd;
OS_EVENT *sem_fnd_buz;
OS_EVENT *mb_tmp_led;
OS_EVENT *mb_led_tmp;
volatile int state = 0; // Timer 2에서 사용할 value 값.
volatile int state3 = 1; // 0 : 화씨 온도계, 1 : 섭씨 온도계
volatile int state4 = 0; // 0 : led 사용 x, 1 : led 사용 o
// 3 tasks
void TmpTask(void *pdata);
void FndTask(void *pdata);
void BuzTask(void *pdata);
void LedTask(void *pdata);

void write_twi_1byte_nopreset(char reg, char data);
int read_twi_2byte_nopreset(char reg);
void init_adc();
unsigned short read_adc();
void show_adc(unsigned short value);

// timer 2 interrupt
ISR(TIMER2_OVF_vect) {
	if (state > 0) {
		PORTB = 0x00;
		state = 0;
	}
	else {
		PORTB = 0x10;
		state = 1;
	}
	TCNT2 = hz_arr[0];
}

ISR(INT4_vect) {
	state4 = state4 > 0 ? 0 : 1;	
	_delay_ms(100);
}

ISR(INT5_vect) {
	state3 = state3 > 0 ? 0 : 1;
	_delay_ms(100);
}

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
	OSTaskCreate(BuzTask, (void *)0, (void *)&BuzTaskStk[TASK_STK_SIZE - 1], 2);
	OSTaskCreate(LedTask, (void *)0, (void *)&LedTaskStk[TASK_STK_SIZE - 1], 3);
	// mailbox and semaphore create
	mb_tmp_fnd = OSMboxCreate((void *)0);
	mb_fnd_tmp = OSMboxCreate((void *)0);
	sem_buz_fnd = OSSemCreate(0);
	sem_fnd_buz = OSSemCreate(0);
	mb_tmp_led = OSMboxCreate((void *)0);
	mb_led_tmp = OSMboxCreate((void *)0);

	// Buzzer with Setting Interrupt
	DDRB = 0x10;
	PORTB = 0x10;

	DDRE = 0xcf; // PE4 입력, PE5 입력
	EICRB |= 0x0A; // INT4, INT5 -> falling Edge
	EIMSK = 0x30;// INT4, INT5 interrupt enable
	SREG |= 0x80; // Interrupt Enable (==sei())

	TCCR2 = 0x03; // timer 2 prescaler
	TIMSK &= 0xbf; // timer 2 overflow interrupt enable
	TCNT2 = hz_arr[0]; // initialize value
	state3 = 1;
	state4 = 1;
			
	OSStart();
	return 0;
}

void TmpTask(void *pdata) {
	
	INT8U err;
	char ans;
	int value, i;
	int temperature;
	double n_value = 32.0f;
	double a_value = 1.0f;
	double cel = 0.0f;
	unsigned short num[4];
	unsigned int deliver = 0;
	short value_int, value_deci;

	//OSTimeDlyHMSM(0, 0, 0, 100); // 다음 사이클을 위하여 잠시 기다림
	while (1) { // 온도값 읽어 FND 디스플레이		
		// fnd 처리 부분
		temperature = read_twi_2byte_nopreset(ATS75_TEMP_REG);

		// 값 처리 부분(초기화 부분 매우 중요)
		deliver = 0;
		n_value = 32.0f;
		a_value = 1.0f;
		cel = 0.0f;

		value = temperature;		
		if (state3 == 0) {
			// 화씨 온도계인 경우.
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
			num[2] = (unsigned short)(n_value / 10);
			num[1] = (unsigned short)n_value % 10;
			n_value *= 10;
			num[0] = (unsigned short)n_value % 10;
		}
		else {
			// 섭씨 온도계 사용 시.
			if ((value & 0x8000) != 0x8000) // Sign 비트 체크
				num[3] = 11; 
			else {
				num[3] = 10;
				value = (~value) - 1; // 2’s Compliment
			}
			value_int = (short) ((value & 0x7f00) >> 8);
			value_deci = (short) (value & 0x00ff);
			num[2] = value_int / 10; 
			num[1] = value_int % 10;
			num[0] = ((value_deci & 0x80) == 0x80) * 5;
		}

		// 공통 부문
		deliver += num[2] * 1000;
		deliver += num[1] * 100;
		deliver += num[0] * 10;
		if (num[3] > 10) deliver += 1;

		OSMboxPost(mb_tmp_fnd, (void *)&deliver);
		ans = *(char *) OSMboxPend(mb_fnd_tmp, 0, &err);

		OSMboxPost(mb_tmp_led, (void *)&deliver);
		ans = *(char *)OSMboxPend(mb_led_tmp, 0, &err);

	}	
}

void FndTask(void *pdata) {
	unsigned int num[4];
	unsigned int value;
	int i, idx;
	int degree = 0;
	char ans = 'y';
	INT8U err;
	DDRC = 0xff;
	DDRG = 0x0f;

	while (1) {
		value = *(unsigned int *) OSMboxPend(mb_tmp_fnd, 0, &err);
		degree = 0; // compare value

		num[3] = (value % 2) + 10;
		value /= 10;
		num[0] = value % 10;
		value /= 10;
		num[1] = value % 10;
		value /= 10;
		num[2] = value % 10;
		value /= 10;
		if (value > 0) 
			num[3] = value % 10;

		// compare value 계산
		degree += num[1];
		degree += num[2] * 10;
		if (num[3] < 10)
			degree += num[3] * 100;

		for (i = 0; i < 4; i++) {
			PORTC = FND_DATA[num[i]];
			PORTG = fnd_sel[i];
			if (i == 1)
				PORTC |= 0x80;
			OSTimeDlyHMSM(0, 0, 0, 4);
		}
		// Degree가 특정 온도를 넘기는 경우. 화재 경보. (여기서는 테스트용 섭씨 28도 기준)
		if ((degree >= 82 && state3 == 0) || (degree >= 28 && state3 == 1)) {
			OSSemPost(sem_fnd_buz);
			OSSemPend(sem_buz_fnd, 0, &err);
		}
		else {
			TIMSK &= 0xbf;
		}
		OSMboxPost(mb_fnd_tmp, (void *)&ans);
	}
}

void BuzTask(void *pdata) {
	INT8U err;
	while (1) {
		OSSemPend(sem_fnd_buz, 0, &err);
		TIMSK |= 0x40; // timer 2 overflow interrupt enable
		OSSemPost(sem_buz_fnd);		
	}
}

void LedTask(void *pdata) {
	unsigned int num[4];
	unsigned int value;
	int i, idx;
	int degree = 0;
	char ans = 'y';
	INT8U err;
	DDRA = 0xff;
	while (1) {
		value = *(unsigned int *)OSMboxPend(mb_tmp_led, 0, &err);
		degree = 0; // compare value

		num[3] = (value % 2) + 10;
		value /= 10;
		num[0] = value % 10;
		value /= 10;
		num[1] = value % 10;
		value /= 10;
		num[2] = value % 10;
		value /= 10;
		if (value > 0)
			num[3] = value % 10;

		// compare value 계산
		degree += num[1];
		degree += num[2] * 10;
		if (num[3] < 10)
			degree += num[3] * 100;
		// LED 게이지 Index 설정
		if (state3 > 0) // 섭씨
			idx = degree / 4;
		else // 화씨
			idx = (degree - 32) / 8;
		// Degree가 특정 온도를 넘기는 경우. 화재 경보. (여기서는 테스트용 섭씨 28도 기준)		
		if ((degree >= 82 && state3 == 0) || (degree >= 28 && state3 == 1)) {
			PORTA = 0xff;
		}
		else {
			PORTA = 0x00;
			// state4 > 0 인 경우에만 선택적으로 led를 사용하도록 합니다.
			if (state4 > 0) {				
				for (i = 0; i < idx; i++) {
					PORTA |= (1 << (7-i));
				}
			}
		}
		OSMboxPost(mb_led_tmp, (void *)&ans);
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


