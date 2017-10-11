/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"
#include <time.h>

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_RND_TASKS                     4       /* Number of identical tasks                          */

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        RndTaskStk[N_RND_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        DecTaskStk[TASK_STK_SIZE];
OS_STK        TaskStartStk[TASK_STK_SIZE];
char          TaskData[N_RND_TASKS];                      /* Parameters to pass to each task               */
OS_EVENT     *RandomSem;
OS_EVENT     *MQ1; // DecisionTask와 Task1 과의 Message Queue
OS_EVENT     *MQ2; // DecisionTask와 Task2 과의 Message Queue
OS_EVENT     *MQ3; // DecisionTask와 Task3 과의 Message Queue
OS_EVENT     *MQ4; // DecisionTask와 Task4 과의 Message Queue
void		 *MQTbl1[N_RND_TASKS]; // MQ1이 참조하는 Message Queue Table
void		 *MQTbl2[N_RND_TASKS]; // MQ2이 참조하는 Message Queue Table
void		 *MQTbl3[N_RND_TASKS]; // MQ3이 참조하는 Message Queue Table
void		 *MQTbl4[N_RND_TASKS]; // MQ4이 참조하는 Message Queue Table

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  DecisionTask(void *data);                       /* Function prototypes of tasks                  */
		void  RandomTask(void *data);
        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
	INT8U i;
	
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */
	
	// Input random seed
	srand(time(NULL));	
	
	// Initiate sem
	RandomSem   = OSSemCreate(1);

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    char       s[100];
    INT16S     key;


    pdata = pdata;                                         /* Prevent compiler warning                 */

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	// (void *)0 = NULL
	// Message Queue Created
	MQ1 = OSQCreate(&MQTbl1[0], N_RND_TASKS);
	MQ2 = OSQCreate(&MQTbl2[0], N_RND_TASKS);
	MQ3 = OSQCreate(&MQTbl3[0], N_RND_TASKS);
	MQ4 = OSQCreate(&MQTbl4[0], N_RND_TASKS);
	
    TaskStartCreateTasks();                                /* Create all the application tasks         */

    for (;;) {
        TaskStartDisp();                                  /* Update the display                       */


        if (PC_GetKey(&key) == TRUE) {                     /* See if key has been pressed              */
            if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                PC_DOSReturn();                            /* Return to DOS                            */
            }
        }

        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait one second                          */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDispInit (void)
{
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    PC_DispStr( 0,  0, "                         uC/OS-II, The Real-Time Kernel                         ", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
    PC_DispStr( 0,  1, "                                Embedded S/W 003                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "                                      Week 5                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "   Task1:   [ ]  Task2:   [ ]  Task3:   [ ]  Task4:   [ ]                       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "#Tasks          :        CPU Usage:     %                                       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "#Task switch/sec:                                                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY + DISP_BLINK);
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           UPDATE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDisp (void)
{
    char   s[80];


    sprintf(s, "%5d", OSTaskCtr);                                  /* Display #tasks running               */
    PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

#if OS_TASK_STAT_EN > 0
    sprintf(s, "%3d", OSCPUUsage);                                 /* Display CPU usage in %               */
    PC_DispStr(36, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#endif

    sprintf(s, "%5d", OSCtxSwCtr);                                 /* Display #context switches per second */
    PC_DispStr(18, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%1d.%02d", OSVersion() / 100, OSVersion() % 100); /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    switch (_8087) {                                               /* Display whether FPU present          */
        case 0:
             PC_DispStr(71, 22, " NO  FPU ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 1:
             PC_DispStr(71, 22, " 8087 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 2:
             PC_DispStr(71, 22, "80287 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 3:
             PC_DispStr(71, 22, "80387 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
	INT8U i;
	// DecisionTask Priority : 1
	OSTaskCreate(DecisionTask, (void *)0, &DecTaskStk[TASK_STK_SIZE - 1], 1);
	// Make 4 Tasks
	for(i = 0; i < N_RND_TASKS; i++) {
		TaskData[i] = i + '0';
		OSTaskCreate(RandomTask, (void *)&TaskData[i], &RndTaskStk[i][TASK_STK_SIZE - 1], 2 + i);
	}
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void  DecisionTask (void *pdata)
{
	// Variable declaration section
	INT8U err, i, sel, res;
	INT8U num[4] = {0,0,0,0};
	INT8U min = 65;
	char ch;
	char win = 'W', lose = 'L';
	
	// For compiler warning
	pdata = pdata;
	
	for(;;){		
		// Message Queue로부터 넘어온 데이터를 받습니다.				
		INT8U *data1 = (INT8U *) OSQPend(MQ1,0,&err);
		INT8U *data2 = (INT8U *) OSQPend(MQ2,0,&err);
		INT8U *data3 = (INT8U *) OSQPend(MQ3,0,&err);
		INT8U *data4 = (INT8U *) OSQPend(MQ4,0,&err);
		
		min = 64; // 전달받는 인자가 0~63까지 이기 때문에
		sel = 0; // index initialize
		
		num[0] = *data1;
		num[1] = *data2;
		num[2] = *data3;
		num[3] = *data4;
									
		if (min > num[0]) {
			min = num[0];
			sel = 0;
		}			
		if (min > num[1]) {
			min = num[1];
			sel = 1;
		}		
		if (min > num[2]) {
			min = num[2];
			sel = 2;
		}		
		if (min > num[3]) {
			min = num[3];
			sel = 3;
		}				
		
		// 가장 작은 숫자를 보낸 Message Queue에만 'W'를 보내고, 나머지 Message Queue에는 'L'을 보냅니다.
		switch(sel) {
		case 0:
			res = OSQPost(MQ1,(void *)&win);
			res = OSQPost(MQ2,(void *)&lose);
			res = OSQPost(MQ3,(void *)&lose);
			res = OSQPost(MQ4,(void *)&lose);
			break;
		case 1:
			res = OSQPost(MQ1,(void *)&lose);
			res = OSQPost(MQ2,(void *)&win);
			res = OSQPost(MQ3,(void *)&lose);
			res = OSQPost(MQ4,(void *)&lose);
			break;
		case 2:
			res = OSQPost(MQ1,(void *)&lose);
			res = OSQPost(MQ2,(void *)&lose);
			res = OSQPost(MQ3,(void *)&win);
			res = OSQPost(MQ4,(void *)&lose);
			break;
		case 3:
			res = OSQPost(MQ1,(void *)&lose);
			res = OSQPost(MQ2,(void *)&lose);
			res = OSQPost(MQ3,(void *)&lose);
			res = OSQPost(MQ4,(void *)&win);
			break;			
		}								
		OSTimeDlyHMSM(0,0,3,0);
	}
}

void RandomTask (void *pdata){
	// Variable declaration section
	INT8U	rnd;
	INT8U	err;
	INT8U	res;
	INT8U   i, temp;
	char	*result;
	char	*win;
	char	*data = (char *)pdata;
	char	ch;
		
	// For compiler warning
	pdata = pdata;	
		
	for(;;){
		// Create a random number
		OSSemPend(RandomSem, 0, &err);
		rnd = random(64);
		OSSemPost(RandomSem);
						
		if (*data == '0') {
			res = OSQPost(MQ1,(void *)&rnd);
			win = (char *)OSQPend(MQ1,0,&err);
			
			temp = rnd;
			// Task1 옆에 숫자 출력
			for (i = 0; i < 2; i++) {
				ch = (char) (temp % 10) + '0';
				PC_DispChar(11-i, 5, ch, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				temp /= 10;
			}
			ch = *win;
			// 'W' or 'L' 출력			
			PC_DispChar(13, 5, ch, DISP_FGND_WHITE + DISP_BGND_RED);
		}
		else if (*data == '1') {
			res = OSQPost(MQ2,(void *)&rnd);
			win = (char *)OSQPend(MQ2,0,&err);			
			temp = rnd;
			// Task2 옆에 숫자 출력
			for (i = 0; i < 2; i++) {
				ch = (char) (temp % 10) + '0';
				PC_DispChar(25-i, 5, ch, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				temp /= 10;
			}
			ch = *win;
			// 'W' or 'L' 출력			
			PC_DispChar(27, 5, ch, DISP_FGND_WHITE + DISP_BGND_CYAN);
		}
		else if (*data == '2') {
			res = OSQPost(MQ3,(void *)&rnd);
			win = (char *)OSQPend(MQ3,0,&err);			
			temp = rnd;
			// Task3 옆에 숫자 출력
			for (i = 0; i < 2; i++) {
				ch = (char) (temp % 10) + '0';
				PC_DispChar(39-i, 5, ch, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				temp /= 10;
			}			
			ch = *win;
			// 'W' or 'L' 출력			
			PC_DispChar(41, 5, ch, DISP_FGND_WHITE + DISP_BGND_BLUE);
		}		
		else if (*data == '3') {
			res = OSQPost(MQ4,(void *)&rnd);
			win = (char *)OSQPend(MQ4,0,&err);			
			temp = rnd;
			// Task4 옆에 숫자 출력
			for (i = 0; i < 2; i++) {
				ch = (char) (temp % 10) + '0';
				PC_DispChar(53-i, 5, ch, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
				temp /= 10;
			}			
			ch = *win;
			// 'W' or 'L' 출력			
			PC_DispChar(55, 5, ch, DISP_FGND_WHITE + DISP_BGND_GREEN);
		}											
		// 'W' 문자를 받은 경우, 화면을 해당 색으로 칠하도록 합니다.		
		if (*data == '0' && *win == 'W') {
			for (i = 6; i < 21; i++) {
				PC_DispStr( 0,  i, "                                                                                ", DISP_FGND_WHITE + DISP_BGND_RED);
			}			
		}
		else if (*data == '1' && *win == 'W') {
			for (i = 6; i < 21; i++) {
				PC_DispStr( 0,  i, "                                                                                ", DISP_FGND_WHITE + DISP_BGND_CYAN);
			}				
		}
		else if (*data == '2' && *win == 'W') {
			for (i = 6; i < 21; i++) {
				PC_DispStr( 0,  i, "                                                                                ", DISP_FGND_WHITE + DISP_BGND_BLUE);
			}				
		}		
		else if (*data == '3' && *win == 'W') {
			for (i = 6; i < 21; i++) {
				PC_DispStr( 0,  i, "                                                                                ", DISP_FGND_WHITE + DISP_BGND_GREEN);
			}							
		}
		OSTimeDlyHMSM(0,0,3,0);				
	}
}