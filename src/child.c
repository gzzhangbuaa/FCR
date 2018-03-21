#include <mpi.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include "chinterface.h"



int main(int argc, char* argv[])
{
	int i;
	char hostName[PDI_BUFS];
	
	int provided, flag;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
		
	gethostname(hostName, PDI_BUFS);
	strcpy(PDI_Daem.hostName, hostName);
	printf("[@@@@ DEBUG_INFO @@@@]: MPI_Init_thread done! hostName is %s\n", PDI_Daem.hostName);

	//Get the interComm between daemon and MPI work processes
        MPI_Comm parent;
        MPI_Comm_get_parent(&parent);
        PDI_Daem.parent = parent;

        //receive the hostname list comes from remote group rank0 in local node
        MPI_Request rreq1, rreq2, rreq3;
        MPI_Status sstatus1, sstatus2, sstatus3;
	sstatus1.MPI_ERROR = 0;
	sstatus2.MPI_ERROR = 0;
	int tag1 = 1, tag2 = 2, tag3 = 3;  //should be the same with sender
        int rb[4];
	/*
        MPI_Irecv(rb, 4, MPI_INT, 0, tag1, PDI_Daem.parent, &rreq1);
	MPI_Wait(&rreq1, &sstatus1);
	*/
	MPI_Recv(rb, 4, MPI_INT, 0, tag1, PDI_Daem.parent, &sstatus1);	
	//printf("[Daemon]@@@@: sstatus1 is %x, source is %d, tag is %d\n", sstatus1.MPI_ERROR, sstatus1.MPI_SOURCE, sstatus1.MPI_TAG);

	PDI_Daem.nodeID = rb[0];	
	PDI_Daem.nbNodes = rb[1];
	PDI_Daem.groupSize = rb[2];
	PDI_Daem.amICenter = rb[3];

	int arraySize = PDI_Daem.nbNodes * PDI_BUFS;
	PDI_Daem.nameList = talloc(char, arraySize);	
	/*
	MPI_Irecv(PDI_Daem.nameList, arraySize, MPI_CHAR, 0, tag2, PDI_Daem.parent, &rreq2);
	MPI_Wait(&rreq2, &sstatus2);
	*/
	MPI_Recv(PDI_Daem.nameList, arraySize, MPI_CHAR, 0, tag2, PDI_Daem.parent, &sstatus2);	
	//printf("[Daemon]@@@@: sstatus2 is %x, source is %d, tag is %d\n", sstatus2.MPI_ERROR, sstatus2.MPI_SOURCE, sstatus2.MPI_TAG);
	
	/*
	printf("[Daemon] : I have got nodeID is %d, nbNodes is %d, groupSize is %d, amICenter is %d\n", 
		PDI_Daem.nodeID, PDI_Daem.nbNodes, PDI_Daem.groupSize, PDI_Daem.amICenter);
	*/
	strncpy(hostName, PDI_Daem.nameList + PDI_Daem.nodeID * PDI_BUFS, PDI_BUFS);
	/*
	printf("[Daemon] : PDI_Daem.hostName is %s, the hostName getting from nameList is %s\n", 
		PDI_Daem.hostName, hostName);
	*/

	/*
	MPI_Irecv(PDI_Daem.appName, PDI_BUFS, MPI_CHAR, 0, tag3, PDI_Daem.parent, &rreq3);
	MPI_Wait(&rreq3, &sstatus3);
	*/
	MPI_Recv(PDI_Daem.appName, PDI_BUFS, MPI_CHAR, 0, tag3, PDI_Daem.parent, &sstatus3);
	//printf("[Daemon]@@@@: sstatus3 is %x, source is %d, tag is %d\n", sstatus3.MPI_ERROR, sstatus3.MPI_SOURCE, sstatus3.MPI_TAG);
	

	//printf("[Daemon]: received appName is %s\n", PDI_Daem.appName);

	//printf("[######## sstatus1 = %d, sstatus2 = %d, sstatus3 = %d #####]\n", sstatus1.MPI_ERROR, sstatus2.MPI_ERROR, sstatus3.MPI_ERROR);

	/* just for test
	for(i = 0; i < PDI_Daem.nbNodes; i++)
	{
		strncpy(hostName, PDI_Daem.nameList + (i * PDI_BUFS), PDI_BUFS);
		printf("Host_%d is : %s\n", i, hostName);
	}	
	*/
	
	//Get control center hostname 
	strncpy(PDI_Daem.ccName, PDI_Daem.nameList, PDI_BUFS);	
	//printf("I got the control center hostName is %s\n", PDI_Daem.ccName);
		
	//Get group which local host locating on
	PDI_Daem.groupID = PDI_Daem.nodeID / PDI_Daem.groupSize;
	int relativePos = PDI_Daem.nodeID % PDI_Daem.groupSize;
	PDI_Daem.posInGroup = relativePos;
	PDI_Daem.realGSize = 0;
	if((PDI_Daem.groupID + 1) * PDI_Daem.groupSize <= PDI_Daem.nbNodes)
	{
		PDI_Daem.realGSize = PDI_Daem.groupSize;
	}
	else
	{
		PDI_Daem.realGSize = PDI_Daem.nbNodes % PDI_Daem.groupSize;
	}
	arraySize = PDI_Daem.realGSize * PDI_BUFS;
	PDI_Daem.groupMembers = talloc(char, arraySize);
	memcpy(PDI_Daem.groupMembers, PDI_Daem.nameList + PDI_Daem.groupID * PDI_Daem.groupSize * PDI_BUFS, 
		PDI_Daem.realGSize * PDI_BUFS);
	
	PDI_Daem.leaders = NULL;        		
	if(PDI_Daem.amICenter != PDI_CENTERHOST)
	{
		free(PDI_Daem.nameList);
	}

	for(i = 0; i < PDI_Daem.realGSize; i++)
	{
		strncpy(hostName, PDI_Daem.groupMembers + i * PDI_BUFS, PDI_BUFS);
		//printf("Host_%d : %s \n", i, hostName);
	}

	PDI_InitLArray();

	//Get the number of work processes in this node 
 	int nodeSize;
	MPI_Comm_remote_size(parent, &nodeSize);
	PDI_Daem.nodeSize = nodeSize;
	//printf("Local Work group size is %d\n", nodeSize);	
	
	//Init the value of all kinds of flags and counts
	PDI_Daem.eventModel = PDI_PROCMODEL;
	//PDI_Daem.eventModel = PDI_NODEMODEL;

	//Init the value of dump switch
	//PDI_Daem.dumpSwitch = PDI_EnableDump;
	PDI_Daem.dumpSwitch = PDI_DisableDump;
	for(i = 0; i < nodeSize; i++)
	{
		PDI_Daem.preFlag[i] = 0;
		PDI_Daem.curFlag[i] = 0;
		PDI_Daem.realCount[i] = 0;
		PDI_Daem.body[i] = -1;
		PDI_Daem.procStatus[i] = PDI_ALIVE;
	}

        //Recv pid from local leader rank0 in parent communicator
        PDI_RecvPid(&PDI_Daem);

	//Init the position nodeSize status flag
	int nsPosition = PDI_Daem.nodeSize;
	if(PDI_Daem.eventModel == PDI_NODEMODEL)
	{
		PDI_Daem.preFlag[nsPosition] = 0;
		PDI_Daem.curFlag[nsPosition] = 0;
	}

	PDI_Daem.isChecking = PDI_UnChecking;

	//Set Timer  when timeout to check the process status
	PDI_AddTimer();
	
	//
	pthread_t thread;
	printf("[DEBUG_INFO]: before pthread_create: hostname:%s\n", PDI_Daem.hostName);
	pthread_create(&thread, NULL, PDI_WaitDetective, NULL);
	printf("[DEBUG_INFO]: after pthread_create: hostname:%s\n", PDI_Daem.hostName);
	PDI_WaitKnocking();
	MPI_Finalize();
	return 0;
}

/*------------------------------------------------------------------------------------------------*/
/**
	@brief		always be ready for receive the knocking and do the right precedure
			executed in the main thread.
	@param
	@return
**/
/*------------------------------------------------------------------------------------------------*/
void PDI_WaitKnocking()
{
	//set a flag for normal exit the loop
	int flag = 0;
	//Listen to the knocking
	int rb[3];
	MPI_Status rstatus;
	while(flag != PDI_Daem.nodeSize)
	{
		MPI_Recv(rb, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, PDI_Daem.parent, &rstatus);
		int srank = rstatus.MPI_SOURCE;

		//record  the rank of every MPI work process in original MPI_COMM_WORLD
		/*
		if(PDI_Daem.body[srank] == -1)
		{
			PDI_Daem.body[srank] = rb[0];
		}
		*/
		
		//just for test
		/*
		printf("I have received a message and rank is %d in original MPI_COMM_WORLD, pid is %d and srank is %d in interComm\n", 
			rb[1], rb[2], srank);
		*/

		int msgType = rb[0];
		if(msgType == PDI_TYPE_JOBFNF)
		{
			flag++;
			continue;	
		}
		else if(msgType == PDI_TYPE_MSGPNF)
		{
			if(PDI_Daem.eventModel == PDI_PROCMODEL)                //update ticking for proc model
                	{
                        	PDI_Daem.curFlag[srank]++;
                	}
                	else if(PDI_Daem.eventModel == PDI_NODEMODEL)   //update ticking for node model
               		{
				PDI_Daem.curFlag[srank]++;		//This step is prepare for status checking
                        	PDI_Daem.curFlag[PDI_Daem.nodeSize]++;
                	}
                	else
                	{
                        	//printf("Non identifiable event model, please check it!\n");
                	}
		}
		else if(msgType == PDI_TYPE_TRIGNF)
		{
			int newrb[3], dumpIf, detectIf, errStrLen;
			MPI_Recv(newrb, 3, MPI_INT, srank, MPI_ANY_TAG, PDI_Daem.parent, &rstatus);
			dumpIf = newrb[0];
			detectIf = newrb[1];
			errStrLen = newrb[2];
			int recvSize = errStrLen;
			char* errMsg = talloc(char, recvSize + 1);
			memset(errMsg, 0, recvSize + 1);
			MPI_Recv(errMsg, recvSize, MPI_CHAR, srank, MPI_ANY_TAG, PDI_Daem.parent, &rstatus);
			recvSize = recvSize + PDI_BUFS;
			char* addStr = talloc(char, recvSize);
			memset(addStr, 0, recvSize);
			sprintf(addStr, "Suspicious Event comes from <hostName : %s> <rank : %d> <pid : %d>\n",
				PDI_Daem.hostName, rb[1], rb[2]);
			strcat(addStr, errMsg);
			errStrLen = strlen(addStr);
			EventTrigger(dumpIf, detectIf, errStrLen, addStr);			
			//printf("[###### Recv Trigger Msg #####]:%s", addStr);
			if(errMsg != NULL)
			{
				free(errMsg);
				errMsg = NULL;
			}
			if(addStr != NULL)
			{
				free(addStr);
				addStr = NULL;
			}
		}
		else if(msgType == PDI_TYPE_HWFTST)   //just for failures test
		{
			int rst = clientTCP(PDI_Daem.hostName, PDI_TYPE_FTEST, 0, NULL);
			printf("[DEBUG_INFO]: failure test phase, stop daemon! hostName=%s,clientTCP rst=%d\n", PDI_Daem.hostName, rst);
			PDI_PauseTimer();
			printf("[DEBUG_INFO]: failure test, hostName = %s cancel timer!\n", PDI_Daem.hostName);
			while(1)
			{
				sleep(100);
			}
		}
		else
		{
			//printf("[WARNING]: Something wrong about MPI message type of PDI, Please do some check!\n");
		}
	}
}


/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
void* PDI_WaitDetective()
{
	/*
	while(1)
	{
		printf("Iam waiting for the status detective!\n");
		printf("I got the PDI_Dame.nbNodes is %d\n", PDI_Daem.nbNodes);
		sleep(10);
	}
	*/
	pthread_detach(pthread_self());
	
	serverTCP();
}

/*----------------------------------------------------------------------------------------------*/
/**
		@brief	Init the leaders array for control center in order to do
		the distributed detect by group
**/
/*----------------------------------------------------------------------------------------------*/
void PDI_InitLArray()
{
        if(PDI_Daem.amICenter == PDI_CENTERHOST)
        {
		int i;
                PDI_Daem.groupNums = PDI_Daem.nbNodes / PDI_Daem.groupSize + (PDI_Daem.nbNodes % PDI_Daem.groupSize != 0 ? 1 : 0);
                PDI_Daem.leaders = talloc(int, PDI_Daem.groupNums);
                int leaderID = 0;
                for(i = 0; i < PDI_Daem.groupNums; i++)
                {
                        if(i == 0)
                        {
                                leaderID = 1;
                        }
                        else
                        {
                                leaderID = i * PDI_Daem.groupSize;
                        }
                        PDI_Daem.leaders[i] = leaderID;
                }
		PDI_Daem.isChecking = PDI_UnChecking;
        }
}

/*--------------------------------------------------------------------------------------------*/
/**
		@brief	Receive the pid info of all MPI Processes in the local node
			from leader rank through nodeInComm communicator
**/
/*--------------------------------------------------------------------------------------------*/
int PDI_RecvPid(PDIT_daemon* PDI_Daem)
{	
	//printf("[##### NOTE !!!#####]\n");
	int nbProc = PDI_Daem->nodeSize;
	//printf("[##### nbProc is %d ####]\n", nbProc);	
	MPI_Request rreq;
	MPI_Status rstatus;
	rstatus.MPI_ERROR = MPI_SUCCESS;
	int tag = 123;

	//non-block receive
	/*
	MPI_Irecv(PDI_Daem->body, nbProc, MPI_INT, 0, tag, PDI_Daem->parent, &rreq);
	MPI_Wait(&rreq, &rstatus);
	*/

	//block receive
	MPI_Recv(PDI_Daem->body, nbProc, MPI_INT, 0, tag, PDI_Daem->parent, &rstatus);	

        int i;
        for(i = 0; i < nbProc; i++)
        {
		//printf("[RecvPid: %d] : %d\n", i, PDI_Daem->body[i]);
        }
	
	//printf("[###### rstatus.MPI_ERROR = %d #######]\n", rstatus.MPI_ERROR);
	

	/* just for non-block receive
	if(rstatus.MPI_ERROR != MPI_SUCCESS)
	{
		printf("[MPI RECV ERROR!]\n");
		return PDI_NSCS;
	}
	else
	{
		printf("[MPI RECV SUCESS!]\n");
		
		//just for test
		int i;
		for(i = 0; i < nbProc; i++)
		{
			printf("[RecvPid: %d] : %d\n", i, PDI_Daem->body[i]);
		}

		return PDI_SCES;
	}
	*/
}


/*--------------------------------------------------------------------------------------------*/
/**
	@brief	log by system call
**/
/*--------------------------------------------------------------------------------------------*/
int logger_bsc(char* fileName, char* buf, int len)
{
        int outputFd, openFlags;
        mode_t filePerms;
        openFlags = O_CREAT | O_RDWR | O_APPEND;
        filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        outputFd = open(fileName, openFlags, filePerms);
        if(outputFd == -1)
        {
                //printf("[WARNING]: Open file %s error, please do some check!\n", fileName);
                return PDI_NSCS;
        }
        int result = write(outputFd, buf, len);
        if(result < 0)
        {
                //printf("[WARNING]: Log write error, please do some check!\n");
                close(outputFd);
                return PDI_NSCS;
        }
        else
        {
                close(outputFd);
                return PDI_SCES;
        }
}

/*---------------------------------------------------------------------*/
/**
	log by library call
**/
/*---------------------------------------------------------------------*/
int logger(char* fileName, char* buf, int len)
{
        FILE* stream;
        if((stream = fopen(fileName, "a+")) == NULL)
        {
                return PDI_NSCS;
        }
        int result = fwrite(buf, len, 1, stream);
        //printf("fwrite counts is %d \n", result);
        fflush(stream);
        fclose(stream);
	stream = NULL;
}


