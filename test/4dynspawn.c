#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFERSIZE	100
#define PDI_BUFS	256

#define talloc(type, num) (type *)malloc(sizeof(type) * (num))

typedef struct PDIT_detail                      /** detail info about this proc running */
{
        int pid;                                /** pid of this MPI proc                */
        int rank;                               /** rank in global comm of this proc    */
        int codeLine;                           /** line number of last message action  */
        char functionName[PDI_BUFS];            /** function name of last message happened */
} PDIT_detail;

static PDIT_detail pd ;
void PDI_TestThreadSupport();

int PDI_GetNodeCount(MPI_Comm nodeInComm, MPI_Comm globalComm);

int PDI_BuildNodeList(int nodeCount);

void PDI_GetDetail(int rank, PDIT_detail* pd);

int PDI_Knock(MPI_Comm nodeInComm, int rank, int pid);

int PDI_CreateDaemon(char* fileName, MPI_Comm splitWorld, MPI_Comm* interComm);

int main(int argc, char* argv[])
{
	int i;
	int size;
	int rank;
	char hostName[BUFFERSIZE];
	int nameLen;
	int pid;	

	MPI_Comm newcomm;
	int errs = 0;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	//PDI_GetDetail(rank, &pd);
	/*
	if(rank == 0)
	{	
		PDI_TestThreadSupport();
	}
	*/	

	char* allHostName = talloc(char, BUFFERSIZE*size);
	gethostname(hostName, BUFFERSIZE);
	MPI_Allgather(hostName, BUFFERSIZE, MPI_CHAR, allHostName, BUFFERSIZE, MPI_CHAR, MPI_COMM_WORLD);
	

	//keep processes in the same node to a split comm
	int color, key;
	int count = 0;
	MPI_Comm myWorld, splitWorld;
	for(i = 0; i < size; i++)
	{
		if(strcmp(hostName, allHostName + BUFFERSIZE*i) == 0)
		{
			if(count == 0)
			{
				color = i;
			}
			if(i == rank)
			{
				key = count;
				break;
			}
			count++;
		}
	}
	MPI_Comm_dup(MPI_COMM_WORLD, &myWorld);
	MPI_Comm_split(myWorld, color, key, &splitWorld);

	int nbNodes = PDI_GetNodeCount(splitWorld, MPI_COMM_WORLD);
	printf("[Get Nodes count is %d]\n", nbNodes);

	PDI_BuildNodeList(nbNodes);
	
	char fileName[PDI_BUFS];
	strcpy(fileName, "./child");
	
	PDI_CreateDaemon(fileName, splitWorld, &newcomm);
	/*
	MPI_Info hostinfo;
	MPI_Info_create(&hostinfo);
	MPI_Info_set(hostinfo, "host", hostName);	
	MPI_Comm_spawn("./child", MPI_ARGV_NULL, 1, hostinfo, 0, splitWorld, &newcomm, &errs);
	*/

	/*
	int ssize, srank; //in split communicator
	MPI_Comm_size(splitWorld, &ssize);
	MPI_Comm_rank(splitWorld, &srank);
	printf("[Commpare!  hostName is %s, Origin:  size is %d, myrank is %d, Split:  size is %d, myrank is %d]\n",
		hostName, size, rank, ssize, srank);
	*/
	//print the allgather result 
	/*
	if(rank == 0)
	{
		int i = 0;
		for(i = 0; i < size; i++)
		{
			strncpy(hostName, allHostName + (i * BUFFERSIZE), BUFFERSIZE -1);
			printf("[rank is %d, hostName is %s]\n", i, hostName);
		}
	}
	*/

	//examine the allgather operation in the sequence
	/*	
	int* allRank;
	allRank = talloc(int, size);
	for(i = 0; i < size; i++)
	{
		allRank[i] = -1;
	}
	MPI_Allgather(&rank, 1, MPI_INT, allRank, 1, MPI_INT, MPI_COMM_WORLD);
	if(rank == 0)
	{
		int j = 0;
		for(j  = 0; j< size; j++)
		{
			printf("position %d rank %d\n", j, allRank[j]);
		}
	}
	free(allRank);
	*/

	MPI_Get_processor_name(hostName, &nameLen);
	
	pid = getpid();
	PDI_Knock(newcomm, rank, pid);

	//MPI_Comm_spawn("./child", MPI_ARGV_NULL, 1, MPI_INFO_NULL, rank, MPI_COMM_WORLD, &newcomm, &errs);
	int trank, tsize;
	MPI_Comm_rank(newcomm, &trank);
	MPI_Comm_size(newcomm, &tsize);
	printf("Iam host %s, rank is %d, but in the newcomm, trank is %d and tsize is %d\n", hostName, rank, trank, tsize);

	getchar();	
	//printf("HostName: %s, ProcSize: %d, MyRank: %d\n", hostName, size, rank);
	MPI_Finalize();
	return 0;
}


int PDI_BuildNodeList(int nodeCount)
{
	int i, found, pos, nbNodes = 0, nbProc;
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nbProc);

	//nameList is the node name list, no repeate
	char *nameList = talloc(char, nodeCount * PDI_BUFS);
	int* nodeList = talloc(int, nbProc);

	//nodeList the i element corresponding rank-i
	//and the value represent it's rank ID
	for(i = 0; i < nbProc ; i++)
	{
		nodeList[i] = -1;
	}
	

	char hname[PDI_BUFS], str[PDI_BUFS], *lhn;
	lhn = talloc(char, PDI_BUFS * nbProc);
	memset(lhn + (rank * PDI_BUFS), 0, PDI_BUFS);
	gethostname(lhn + (rank * PDI_BUFS), PDI_BUFS);
	
	strncpy(hname, lhn + (rank * PDI_BUFS), PDI_BUFS -1);
	MPI_Allgather(hname, PDI_BUFS, MPI_CHAR, lhn, PDI_BUFS, MPI_CHAR, MPI_COMM_WORLD);
	for(i = 0; i < nbProc; i++)
	{
		found = 0;
		pos = 0;
		strncpy(hname, lhn + (i * PDI_BUFS), PDI_BUFS - 1);
		while((pos < nbNodes) && (found == 0))
		{
			if(strncmp(&(nameList[pos * PDI_BUFS]), hname, PDI_BUFS) == 0)
			{
				found = 1;
			}
			else
			{
				pos++;
			}
		}
		if(found)
		{
			nodeList[i] = pos;
		}
		else
		{
			strncpy(&(nameList[pos * PDI_BUFS]), hname, PDI_BUFS - 1);
			nodeList[i] = nbNodes;
			nbNodes++;
		}
	}
	
	for(i = 0; i < nodeCount; i++)
	{
		strncpy(hname, nameList + (i * PDI_BUFS), PDI_BUFS -1);
		printf("The %d element of nameList is %s\n", i, hname);
	}
	for(i = 0; i < nbProc; i++)
	{
		printf("rank %d , host ID is %d\n", i, nodeList[i]);
	}
}

int PDI_GetNodeCount(MPI_Comm nodeInComm, MPI_Comm globalComm)
{
	int sb, rb, bufsize = 1;
	int size, rank;
	MPI_Comm_rank(nodeInComm, &rank);
	MPI_Comm_size(nodeInComm, &size);
	if(rank == 0)
	{
		sb = 1;
	}
	else
	{
		sb = 0;
	}
	MPI_Allreduce(&sb, &rb, bufsize, MPI_INT, MPI_SUM, globalComm);
	printf("Host count is %d \n", rb);
	return rb;
}

void PDI_TestThreadSupport()
{
        int provided, flag;

        //check the value of these different thread level
        printf("MPI_THREAD_SINGLE value is %d\n", MPI_THREAD_SINGLE);
        printf("MPI_THREAD_FUNNELED value is %d\n", MPI_THREAD_FUNNELED);
        printf("MPI_THREAD_SERIALIZED value is %d\n", MPI_THREAD_SERIALIZED);
        printf("MPI_THREAD_MULTIPLE value is %d\n", MPI_THREAD_MULTIPLE);

	MPI_Query_thread(&provided);

        //export which thread level the MPI is supporting
        switch(provided)
        {
                case (MPI_THREAD_SINGLE):
                        printf("The thread level this MPI implementation supports is MPI_THREAD_SINGLE\n");
                        break;
                case (MPI_THREAD_FUNNELED):
                        printf("The thread level this MPI implementation supprots is MPI_THREAD_FUNNELED\n");
                        break;
                case (MPI_THREAD_SERIALIZED):
                        printf("The thread level this MPI implementation supports is MPI_THREAD_SERIALIZED\n");
                        break;
                case (MPI_THREAD_MULTIPLE):
                        printf("The thread level this MPI implementation supprots is  MPI_THREAD_MULTIPLE\n");
                        break;
                default:
                        printf("This MPI implementation can't support any thread\n");
        }

        MPI_Is_thread_main(&flag);
        if(flag)
        {
                printf("provided's value is %d, this is main thread\n", provided);
        }
        else
        {
                printf("provided's value is %d, this is not main thread\n", provided);
        }

}

void PDI_GetDetail(int rank, PDIT_detail* pd)
{
	//PDIT_detail* pd = talloc(PDIT_detail, 1);
	int codeLine = __LINE__;
	int pid = getpid();
	pd->pid = pid;
	pd->codeLine = codeLine;
	strcpy(pd->functionName, __func__);
	pd->rank = rank;
	printf("pd->pid = %d\n pd->rank = %d\n pd->codeLine = %d\n pd->functionName = %s\n", pd->pid,
		pd->rank, pd->codeLine, pd->functionName);
}

int PDI_Knock(MPI_Comm nodeInComm, int rank, int pid)
{
	int sb[2], rb[2], tag = 0;
	MPI_Request sreq;
	MPI_Status sstatus;
	sb[0] = rank;
	sb[1] = pid;
	MPI_Isend(sb, 2, MPI_INT, 0, tag, nodeInComm, &sreq);
	MPI_Wait(&sreq, &sstatus);
	printf("[Send over!]");
	
}


int PDI_CreateDaemon(char* fileName, MPI_Comm splitWorld, MPI_Comm* interComm)
{
	char hostName[PDI_BUFS];
	gethostname(hostName, PDI_BUFS);
	int errs = 0;
        MPI_Info hostinfo;
        MPI_Info_create(&hostinfo);
        MPI_Info_set(hostinfo, "host", hostName);
        MPI_Comm_spawn(fileName, MPI_ARGV_NULL, 1, hostinfo, 0, splitWorld, interComm, &errs);

}






