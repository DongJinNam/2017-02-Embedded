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
/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                         3       /* Number of identical tasks                          */
/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskUpdateSecondTickStk[TASK_STK_SIZE];				/* Tasks stacks                    */
OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];
OS_STK        TaskStartStk[TASK_STK_SIZE];

OS_EVENT	 *sem;													/* Semaphores                      */
OS_EVENT	 *mutex;												/* Mutex                      */

volatile INT8S	secElapsed	= 0;
INT8U			x			= 0;
INT8U			taskStatus	= 0x07;
/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  Task0(void *pdata);                      /* Function prototypes of tasks                 */
		void  Task1(void *pdata);
		void  Task2(void *pdata);
        void  TaskUpdateSecondTick(void *pdata);       /* Function prototypes of tasks                 */
        void  TaskStart(void *pdata);                  /* Function prototypes of Startup task          */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);
		void  run(INT8U);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
	INT8U	 err;
	
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                 

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */
	
	sem	= OSSemCreate(1);
	mutex = OSMutexCreate(0x03,&err);

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
    int err;

    pdata = pdata;                                         /* Prevent compiler warning                 */
   
    

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

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
    PC_DispStr( 0,  3, "                                      Week 8                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "  < Task running timeline >                                                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                   Elapsed    second(s), now runs: Task                         ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "  Task0  |                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "  Task1  |                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "  Task2  |                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "  -------+---+--+--+--+--+--+--+--+--+--+--+--+--+--+--+                        ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "  Time   |  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14                        ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
	OSTaskCreate(TaskUpdateSecondTick, (void *)0, &TaskUpdateSecondTickStk[TASK_STK_SIZE - 1], 1);
	OSTaskCreate(Task0, (void *)0, &TaskStk[0][TASK_STK_SIZE - 1], 4);
	OSTaskCreate(Task1, (void *)0, &TaskStk[1][TASK_STK_SIZE - 1], 6);
	OSTaskCreate(Task2, (void *)0, &TaskStk[2][TASK_STK_SIZE - 1], 8);
}

/*
*********************************************************************************************************
*                                            TASK RUNNING STATE
*********************************************************************************************************
*/
void run(INT8U taskNum){
	INT8U t;
	INT8U err;
	const INT8U taskColor[] =
	{
		DISP_FGND_WHITE | DISP_BGND_RED,
		DISP_FGND_WHITE | DISP_BGND_BLUE,
		DISP_FGND_WHITE | DISP_BGND_GREEN
	};
	
	PC_DispChar(55, 7, '0' + taskNum, taskColor[taskNum]);
	PC_DispStr(11 + 3 * x++, 8 + taskNum, "   ", taskColor[taskNum]);
	t = secElapsed;
	// task runs but does nothing for a second
	while(t + 1 > secElapsed);
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/
void TaskUpdateSecondTick (void *pdata)
{	
	pdata = pdata;
	
	// runs until three tasks are dead
	while(taskStatus){
		PC_DispChar(27, 7, '0' + secElapsed / 10, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		PC_DispChar(28, 7, '0' + secElapsed % 10, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		secElapsed++;
		OSTimeDlyHMSM(0, 0, 1, 0);
	}
	PC_DispStr(51, 7, "     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	OSTaskDel(OS_PRIO_SELF);
}

void  Task0 (void *pdata)
{
	INT8U err;
	pdata = pdata;
	
	OSTimeDlyHMSM(0, 0, 2, 0);
	run(0);
	
	OSMutexPend(mutex, 0, &err);			
	OSSemPend(sem, 0, &err);
		
	run(0);
	run(0);
	run(0);
		
	OSMutexPost(mutex);	
	OSSemPost(sem);	
	
	taskStatus &= 0x01;
	OSTaskDel(OS_PRIO_SELF);
}

void  Task1 (void *pdata)
{
	INT8U err;
	pdata = pdata;
	
	OSTimeDlyHMSM(0, 0, 4, 0);
	run(1);
	run(1);
	run(1);
	run(1);
	taskStatus &= 0x02;
	OSTaskDel(OS_PRIO_SELF);
}

void  Task2 (void *pdata)
{
	INT8U err;
	pdata = pdata;
	
	run(2);
	OSSemPend(sem, 0, &err);
	OSMutexPend(mutex, 0, &err);
	
	
	run(2);
	run(2);
	run(2);
		
	OSSemPost(sem);
	OSMutexPost(mutex);
	taskStatus &= 0x04;
	OSTaskDel(OS_PRIO_SELF);
}

