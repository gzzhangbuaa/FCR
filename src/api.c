/**
 *	@file		api.c
 *	@author		ZhangGuozhen
 *	@brief		API functions for PDI library.
**/

#include "interface.h"

/** Dynamic information for this execution.			*/
static PDIT_execution PDI_Exec;

/** MPI request for send message				*/
static MPI_Request PDI_Sreq;

/** MPI communicator that split the global one into sub-comm by node	*/
MPI_Comm PDI_COMM_NodeIn;

int PDI_Init()
{
	//set limit about core
	//PDI_SetCoreLimit();	

	int rank, size;
	PDI_Exec.globalComm = MPI_COMM_WORLD;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	PDI_Exec.nbProc = size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	PDI_Exec.myRank = rank;
	PDI_Exec.pid = getpid();
	//printf("Original size is %d , rank is %d\n", PDI_Exec.nbProc, PDI_Exec.myRank);
	
	PDI_SplitCommByNode(&PDI_Exec);
	//printf("split Comm over!\n");
	
	PDI_GetNodeCount(&PDI_Exec);
	if(PDI_Exec.myRank == 0)
	{
		PDI_Exec.amICenter = PDI_CENTERHOST;
		PDI_TestThreadSupport();
	}
	else
	{
		PDI_Exec.amICenter = PDI_WORKHOST;
	}
	
	//PDI_BuildNodeList(&PDI_Exec);
	char* nameList = talloc(char, PDI_Exec.nbNodes * PDI_BUFS);
	int* nodeList = talloc(int, PDI_Exec.nbProc);
	PDI_BuildNodeList2(&PDI_Exec, nameList, nodeList);
	printf("PDI_BuildNodeList Done! \n");

	//spawn a child process as daemon
	char fileName[PDI_BUFS];
	strcpy(fileName, "./child");
	PDI_CreateDaemon(fileName, &PDI_Exec);

        //translate the node and host information to dameon by rank 0 in local communicator
        if(PDI_Exec.nodeInRank == 0)
        {
		//get the groupSize by default
		//PDI_Exec.groupSize = 100;
		PDI_Exec.groupSize = 8;
                //printf("sender: hostname ID is %d, groupsize is %d, amICenter is %d\n", PDI_Exec.nodeID, PDI_Exec.groupSize, PDI_Exec.amICenter);
		int sb[4];
		sb[0] = PDI_Exec.nodeID;
		sb[1] = PDI_Exec.nbNodes;
		sb[2] = PDI_Exec.groupSize;
		sb[3] = PDI_Exec.amICenter;
		
		/*
		int groupID = PDI_Exec.nodeID / PDI_Exec.groupSize;
		int relativePos = POD_Exec.nodeID % PDI_Exec.groupSize;
		int sendCount = 0;
		if((groupID + 1) * PDI_Exec.groupSize < PDI_Exec.nbNodes)
		{
			sendCount = PDI_Exec.groupSize;
		}
		else
		{
			sendCount = PDI_Exec.nbNodes % PDI_Exec.groupSize;
		}
		*/

		//get app name and tranfer it to deamon 
		char* procName = talloc(char, PDI_BUFS);
		if(procName != NULL)
		{
			memset(procName, 0, PDI_BUFS);
		}
		if(PDI_GetPath(procName) == PDI_NSCS)
		{
			free(procName);
			return PDI_NSCS;
		}
		//printf("[@@@@]: procName is %s\n", procName);


		MPI_Request sreq1, sreq2, sreq3;
		MPI_Status sstatus1, sstatus2, sstatus3;
		int tag1 = 1, tag2 = 2, tag3 = 3;			
		/*	
		MPI_Isend(sb, 4, MPI_INT, 0, tag1, PDI_Exec.daemonComm, &sreq1);
		MPI_Isend(nameList, PDI_Exec.nbNodes * PDI_BUFS, MPI_CHAR, 0, tag2, PDI_Exec.daemonComm, &sreq2);
		MPI_Isend(procName, PDI_BUFS, MPI_CHAR, 0, tag3, PDI_Exec.daemonComm, &sreq3);		
		
		MPI_Wait(&sreq1, &sstatus1);
		MPI_Wait(&sreq2, &sstatus2);
		MPI_Wait(&sreq3, &sstatus3);
		*/
		MPI_Send(sb, 4, MPI_INT, 0, tag1, PDI_Exec.daemonComm);
		MPI_Send(nameList, PDI_Exec.nbNodes * PDI_BUFS, MPI_CHAR, 0, tag2, PDI_Exec.daemonComm);
		MPI_Send(procName, PDI_BUFS, MPI_CHAR, 0, tag3, PDI_Exec.daemonComm);
		//printf("[@@@@ SENDER]: sstatus1 = %d, sstatus2 = %d, sstatus3 = %d\n", sstatus1.MPI_ERROR, sstatus2.MPI_ERROR, sstatus3.MPI_ERROR);
		//printf("[@@@###@@@] SENDEROVER!\n");
        }

	//send pid to daemon 
	if(PDI_SendPid(&PDI_Exec) == PDI_NSCS)
	{
		//printf("[ERROR]: bad things occured in PDI_SendPid function\n");
		exit(-1);
	}	
	
	//record all hosts by rank 0 as control center
	if(PDI_Exec.myRank == 0)
	{
		PDI_RecordHostSet(&PDI_Exec, PDI_ALLHOST, nameList);
	}
        free(nameList);
        free(nodeList);

	//add 20170609
	MPI_Barrier(MPI_COMM_WORLD);
	
	printf("[DEBUG_INFO]: MPI_Barrier arriverd here !\n");
}

int PDI_Knock()
{
	int sb[3], tag = 0;
	sb[0] = PDI_TYPE_MSGPNF;	//msg passing notify
	sb[1] = PDI_Exec.myRank;
	sb[2] = PDI_Exec.pid;
	int result = MPI_Isend(sb, 3, MPI_INT, 0, tag, PDI_Exec.daemonComm, &PDI_Sreq);
	
	//MPI_Status sstatus;
	//MPI_Wait(&PDI_Sreq, &sstatus);
	return result;
}

/**	test for hardware malfunctions or software bugs	**/
/**	specify a nodeID to create failure		**/
int PDI_FailureTest(int nodeID, int type)
{
	if(PDI_Exec.nodeID != nodeID )
	{
		return PDI_SCES;
	}
	// use local communicator to synchnorize
	printf("[DEBUG_INFO]: nodeID is %d\n", PDI_Exec.nodeID);
	MPI_Barrier(PDI_Exec.nodeInComm);

        time_t currentTime;
        time(&currentTime);
        printf("[DEBUG_INFO] failure test time to stop all target MPI processes current time:%s", ctime(&currentTime));
	
	if(type == PDI_TYPE_HFAIL)
	{
		if(PDI_Exec.nodeInRank == 0)
		{
			printf("[DEBUG_INFO]:This is a hardware failure test, Send a notice to daemon!\n ");
        		int sb[3], tag = 0;
        		sb[0] = PDI_TYPE_HWFTST;        //msg passing notify
        		sb[1] = PDI_Exec.myRank;
        		sb[2] = PDI_Exec.pid;
        		int result = MPI_Isend(sb, 3, MPI_INT, 0, tag, PDI_Exec.daemonComm, &PDI_Sreq);
		}
	}
	if(type == PDI_TYPE_SFAIL)
	{
		if(PDI_Exec.nodeInRank == 0)
		{
			printf("[DEBUG_INFO]: This is a software failure test!\n");
		}
	}
	//all MPI processes in the target node trapped 
	while(1)
	{
		sleep(100);
	}
}


int PDI_Finalize()
{
	int sb[3], tag = 1;
	sb[0] = PDI_TYPE_JOBFNF;	//job finished notify
	sb[1] = PDI_Exec.myRank;
	sb[2] = PDI_Exec.pid;
	int result = MPI_Isend(sb, 3, MPI_INT, 0, tag, PDI_Exec.daemonComm, &PDI_Sreq);
	MPI_Status sstatus;
	MPI_Wait(&PDI_Sreq, &sstatus); 	
	return result;
}

int PDI_Trigger(int dumpOrNot, int deteOrNot, char* errStr, int len)
{
	//send base information fist
	int sb[3], tag = 2;
	MPI_Status sstatus;
	sb[0] = PDI_TYPE_TRIGNF;
	sb[1] = PDI_Exec.myRank;
	sb[2] = PDI_Exec.pid;
	int result = MPI_Isend(sb, 3, MPI_INT, 0, tag, PDI_Exec.daemonComm, &PDI_Sreq);	

	MPI_Wait(&PDI_Sreq, &sstatus);
	
	//second send configure information
	sb[0] = dumpOrNot;
	sb[1] = deteOrNot;
	sb[2] = len;
	result = MPI_Isend(sb, 3, MPI_INT, 0, tag, PDI_Exec.daemonComm, &PDI_Sreq);
	MPI_Wait(&PDI_Sreq, &sstatus);

	//printf("[@@@@@@@@@@@@@@@@@@@@@@@@ Before Trigger Send @@@@]: %d, %s", len, errStr);		
	//last send error event description
	result = MPI_Isend(errStr, len, MPI_CHAR, 0, tag, PDI_Exec.daemonComm, &PDI_Sreq);
	MPI_Wait(&PDI_Sreq, &sstatus);
}

void PDI_ShowCurrentTime()
{
        time_t currentTime;
        time(&currentTime);
        printf("[DEBUG_INFO] for failure test : %s", ctime(&currentTime));
}




