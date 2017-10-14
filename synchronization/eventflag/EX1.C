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
#define  N_RANDOM_TASKS                         4       /* Number of identical tasks                   */
#define  N_DECISION_TASKS                       1
/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskDecisionStk[TASK_STK_SIZE];						/* Tasks stacks                    */
OS_STK        TaskRandomStk[N_RANDOM_TASKS][TASK_STK_SIZE];
OS_STK        TaskStartStk[TASK_STK_SIZE];
OS_FLAG_GRP	  *r_grp; // receive event flag	
OS_FLAG_GRP	  *s_grp; // send event flag	
OS_EVENT	  *r_sem; // receive semaphore
OS_EVENT	  *s_sem; // send semaphore

INT8U		  TaskData[N_RANDOM_TASKS];
char		  receive_array[N_RANDOM_TASKS]; 						/* Global variables                */
INT8U		  send_array[N_RANDOM_TASKS]; 
INT8U		  TaskRandomVal[N_RANDOM_TASKS]; // 이게 필요할지는 의문
/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  RandomTask(void *pdata);                 /* Function prototypes of tasks                 */
        void  DecisionTask(void *pdata);               /* Function prototypes of tasks                 */
        void  TaskStart(void *pdata);                  /* Function prototypes of Startup task          */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);
static  void  PrintColor(INT8U); // 이전과 추가된거 (화면에 단순히 색칠하는것)

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
	INT8U	 err;
	
    srand((unsigned int)time(0));
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                 

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

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
    INT8U err;

    pdata = pdata;                                         /* Prevent compiler warning                 */
   
    

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */
	
	// semaphore and event flag create
	r_sem = OSSemCreate(1);	
	s_sem = OSSemCreate(1);	
	r_grp = OSFlagCreate(0x00, &err);
	s_grp = OSFlagCreate(0x00, &err);
		
    TaskStartCreateTasks();                                /* Create all the application tasks         */

    for (;;) {
        TaskStartDisp();                                   /* Update the display                       */

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
    PC_DispStr( 0,  3, "                                      Week 7                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
    INT8U	i;
	
    // Task Create
	for(i = 0; i < N_RANDOM_TASKS; i++){
		TaskData[i] = i;
		OSTaskCreate(RandomTask, (void *)&TaskData[i], &TaskRandomStk[i][TASK_STK_SIZE - 1], i + 1);
	}
	OSTaskCreate(DecisionTask, (void *)0, &TaskDecisionStk[TASK_STK_SIZE - 1], i + 1);
}

/*
*********************************************************************************************************
*                                          Function to paint colors
*********************************************************************************************************
*/
// Color circulation
static const INT8U _COLOR[] = {
	DISP_FGND_WHITE | DISP_BGND_RED,
	DISP_FGND_WHITE | DISP_BGND_BLUE,
	DISP_FGND_WHITE | DISP_BGND_GREEN,
	DISP_FGND_WHITE | DISP_BGND_BROWN
};

// 컬러 인덱스에 해당하는 컬러값을 화면에 출력하는 역할
static void PrintColor(INT8U nColor){
	INT8U x, y;
	for(y = 0; y < 15; y++)
		for(x = 0; x < 80; x++)
			PC_DispChar(x, y + 6, ' ', _COLOR[nColor]);
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

#define MAX_RANDOM_NUMBER 64
void  RandomTask (void *pdata)
{
	INT8U nRndTask;
	INT8U rnd; // 임의 숫자
	INT8U err;
	INT8U temp; // 출력용 변수
	INT8U i; // iterator
	char ch; // 출력 문자
	
	nRndTask = *(INT8U*)pdata;
	
	for(;;){
		
		// Create a random number
		OSSemPend(s_sem, 0, &err);
		send_array[nRndTask] = random(64);
		OSSemPost(s_sem);
		
		// Send Event Flag Post
		OSFlagPost(s_grp, (1 << nRndTask), OS_FLAG_SET, &err);
		// Receive Event Flag Pend
		OSFlagPend(r_grp, (1 << nRndTask), OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME, 0, &err);
		
		temp = send_array[nRndTask];
		// number print
		for (i = 0; i < 2; i++) {
			ch = (char) (temp % 10) + '0';
			PC_DispChar(11-i+(14*nRndTask), 5, ch, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			temp /= 10;				
		}
		ch = receive_array[nRndTask];
		// 'W' or 'L' print
		PC_DispChar(13+(14*nRndTask), 5, ch, _COLOR[nRndTask]);
		// if task received 'W', print background color
		if (ch == 'W') PrintColor(nRndTask);

		OSTimeDlyHMSM(0,0,3,0);
	}
}

void  DecisionTask (void *pdata)
{
	INT8U min = 64;
	INT8U sel = 0;	
	INT8U err;
	INT8U i;
	// avoid compiler warning
	pdata = pdata;
	
	for(;;){
		// send event flag pend
		OSFlagPend(s_grp, 0x0F, OS_FLAG_WAIT_SET_ALL | OS_FLAG_CONSUME, 0, &err);
		
		min = 64;
		sel = 0;
		
		if (min > send_array[0]) {
			min = send_array[0];
			sel = 0;
		}			
		if (min > send_array[1]) {
			min = send_array[1];
			sel = 1;
		}		
		if (min > send_array[2]) {
			min = send_array[2];
			sel = 2;
		}		
		if (min > send_array[3]) {
			min = send_array[3];
			sel = 3;
		}		
		
		// receive array set
		OSSemPend(r_sem,0,&err);		
		for (i = 0; i < N_RANDOM_TASKS; i++) {
			if (i == sel) {
				receive_array[i] = 'W';
			}
			else {
				receive_array[i] = 'L';
			}
		}		
		OSSemPost(r_sem);	
		
		// receive event flag post
		OSFlagPost(r_grp, 0x0F, OS_FLAG_SET, &err);		
		OSTimeDlyHMSM(0,0,3,0);
	}
}
