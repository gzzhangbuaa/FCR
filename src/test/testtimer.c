#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static void PDI_SIGHandler(int sig);
static struct itimerval oldtv;

void PDI_SetTimer(int interval, int value);
void PDI_SIGProcess();
void PDI_AddTimer();
void PDI_PauseTimer();

/*------------------------------------------------------------------------------*/
/**
 *         @brief          
 *                 @param
 *                         @return
 *                         **/
/*------------------------------------------------------------------------------*/
void PDI_AddTimer()
{
        //printf("Now add a timer to the local daemon process!\n");
        PDI_SIGProcess();
        PDI_SetTimer(60, 60);
        //printf("hostName is %s\n", PDI_Daem.hostName);
}

void PDI_PauseTimer()
{
	PDI_SetTimer(0,0);
}
void PDI_SIGProcess()
{
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = PDI_SIGHandler;
        if(sigaction(SIGALRM, &sa, NULL) == -1)
        {
                perror("sigaction");
                exit(-1);
        }
}

void PDI_SetTimer(int interval, int value)
{
        struct itimerval itv;
        itv.it_interval.tv_sec = interval;
        itv.it_interval.tv_usec = 0;
        itv.it_value.tv_sec = value;
        itv.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &itv, &oldtv);
}


static void PDI_SIGHandler(int sig)
{
	printf("timer clock tick!\n");
}

void main()
{
	PDI_AddTimer();
	int i = 0;
	while(i != 40)
	{
		printf("i = %d\n", i);
		sleep(10);
		if( i == 20)
		{
			PDI_PauseTimer();
			printf("Pause timer,i = 20\n");
		}
		i++;
	}
}
