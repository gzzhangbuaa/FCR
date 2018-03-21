/**
	@file 	timer.c
	@author	ZhangGuozhen
	@brief	local timer and singnal handle for PDI library private functions.
**/
#include "chinterface.h"

static void PDI_SIGHandler(int sig);
static struct itimerval oldtv;
static char* workProcName = NULL;
extern PDIT_daemon PDI_Daem;



/*------------------------------------------------------------------------------*/
/**
	@brief		
	@param
	@return
**/
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
	PDI_SetTimer(0, 0);
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

	PDI_CheckSuspect();
	//printf("This is signal handler function! hostName is %s\n", PDI_Daem.hostName);
	/* just for test
	if(PDI_Daem.nodeID == 0)
	{
		clientTCP("mpi1");
	}
	if(PDI_Daem.nodeID == 1)
	{
		clientTCP("mpi2");
	}
	*/
	//clientTCP(PDI_Daem.ccName, PDI_TYPE_ABREPT, 100, "abcdefg");
}

/*---------------------------------------------------------------------------------------------*/
/**
	@brief	every MPI process have a cooresponding status flag 
		if there is no msg passing in one process for a long time
		which should be considered as a suspect event
		ALIVE represents msg passing normally
		DEAD represents msg passing abnormally
	return 	integer suspect event counts
		if the return value is non-zero, report action should be created.
**/
/*---------------------------------------------------------------------------------------------*/

int PDI_CheckProc()
{
	//printf("[@@@@ PDI_CheckProc] : START\n");
	int i, nbSuspect = 0;
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{	
		if(PDI_Daem.preFlag[i] == PDI_Daem.curFlag[i])
		{
			PDI_Daem.realCount[i]++;		
			if(PDI_Daem.realCount[i] >= PDI_CORDON)
			{
				//printf("It's time to report to the center controller!\n");
				nbSuspect++;
				PDI_Daem.realCount[i] = 0;	//value reset for next turn 
				PDI_Daem.procStatus[i] = PDI_DEAD;
				//printf("[@@@@ PDI_CheckProc-set %d proc DEAD]\n", i);
			}
			else
			{
				//printf("Wait for watching!\n");
			}
		}
		else
		{
			PDI_Daem.preFlag[i] = PDI_Daem.curFlag[i];
			PDI_Daem.realCount[i] = 0;
			PDI_Daem.procStatus[i] = PDI_ALIVE;
		}		
	}
	//printf("[@@@@ PDI_CheckProc] : END\n");
	//printf("There are %d had reached the cordon for trigger!\n", nbSuspect);
	return nbSuspect;
}

/*----------------------------------------------------------------------------------------------*/
/**
	@brief	For node mode that only when there is no msg passing occured for a long time
		suspection report action would be triggered
		by store and update and compare the status flag located in special position
		to make certain that should report or not
	return 	integer	PDI_REPORT represents report the suspection to cc
		PDI_UNREPORT represents do not report the suspection immediately
**/	
/*----------------------------------------------------------------------------------------------*/

int PDI_CheckNode()
{
	//update every proc's status, prepare for PDI_GetNodeStatus 
	PDI_CheckProc();

	/**
		nsPos resprents the position for recording the node status flag
		all work processes once took a communication action
		they will call MPI_Knock to change the curFlag on this position
		0~(size-1) for proc model 
		position size for node model
	**/
	int nsPos = PDI_Daem.nodeSize;
	if(PDI_Daem.curFlag[nsPos] == PDI_Daem.preFlag[nsPos])
	{
		PDI_Daem.realCount[nsPos]++;
		if(PDI_Daem.realCount[nsPos] >= PDI_CORDON)
		{
			//printf("Very long time no Communicate action, report it\n");
			PDI_Daem.realCount[nsPos] = 0;		//value reset for next turn
			return PDI_REPORT;
		}
		else
		{
			//printf("Made a suspect, not sure, wait and see\n");
		}
	}
	else
	{
		PDI_Daem.preFlag[nsPos] = PDI_Daem.curFlag[nsPos];
		PDI_Daem.realCount[nsPos] = 0;
	}
	return PDI_UNREPORT;
}

/*---------------------------------------------------------------------------------------------*/
/**
	@brief	Two check mode: Node & Proc
		choose one of them to implement
		when the check result satisfy with the suspect trigger condition
		create a report action and translate to center control
		No matter Node or Proc Model, they all will maintain every proc's status
		which will be used for report
	return 	void
**/
/*---------------------------------------------------------------------------------------------*/
void PDI_CheckSuspect()
{
	if(PDI_Daem.eventModel == PDI_NODEMODEL)
	{
		if(PDI_CheckNode() == PDI_REPORT)
		{
			//coredump first
			if(PDI_Daem.dumpSwitch == PDI_EnableDump)
			{
				PDI_DumpForNode();
			}
			//printf("Report to central controller. \n");
			clientTCP(PDI_Daem.ccName, PDI_TYPE_ABREPT, 0, NULL);
		}
	}
	else if(PDI_Daem.eventModel == PDI_PROCMODEL)
	{
		if(PDI_CheckProc() != 0)
		{	
			if(PDI_Daem.dumpSwitch == PDI_EnableDump)
			{
				PDI_DumpForNode();
			}
			//printf("Report to central controller. \n");
			clientTCP(PDI_Daem.ccName, PDI_TYPE_ABREPT, 0, NULL);
		}
	}
	else
	{
		//printf("non identifiable event model,please connect adiministator and check it \n");
	}
}




