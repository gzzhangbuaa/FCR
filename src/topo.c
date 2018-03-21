/**
 *	@brief		topo.c	
 *	@author		ZhangGuozhen
 *	@brief		Topology functions for the PDI library.
**/

#include "interface.h"
#include <stdlib.h>

/*------------------------------------------------------------------------*/
/**
	@brief Split the MPI_COMM_WORLD into some sub-comm by nodes
	@param namelist		The list of node names from all MPI proc.
	@return integer		PDI_SCES if successful
**/
/*------------------------------------------------------------------------*/
int PDI_SplitCommByNode(PDIT_execution* PDI_Exec)
{
	int i;
        int size;
        int rank;
        char hostName[PDI_BUFS];
        int nameLen;

        MPI_Comm globalComm, nodeInComm;
	globalComm = PDI_Exec->globalComm;
        int errs = 0;
	size = PDI_Exec->nbProc;
	rank = PDI_Exec->myRank;
 
        char* allHostName = talloc(char, PDI_BUFS*size);
        gethostname(hostName, PDI_BUFS);
        MPI_Allgather(hostName, PDI_BUFS, MPI_CHAR, allHostName, PDI_BUFS, MPI_CHAR, globalComm);


        //keep processes in the same node to a split comm
        int color, key;
        int count = 0;
        MPI_Comm myWorld; // a copy of MPI_COMM_WORLD
        for(i = 0; i < size; i++)
        {
                if(strcmp(hostName, allHostName + PDI_BUFS*i) == 0)
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
        MPI_Comm_dup(globalComm, &myWorld);
        MPI_Comm_split(myWorld, color, key, &nodeInComm);
	PDI_Exec->nodeInComm = nodeInComm;
}


/*----------------------------------------------------------------------------*/
/**
 	@brief		Get nodes count in this system
 	@param		nodeInComm	the inter-comm in local node
 	@param		globalComm	original MPI_COMM_WORLD
 	@return 	integer		nodes count	
**/
/*----------------------------------------------------------------------------*/
int PDI_GetNodeCount(PDIT_execution* PDI_Exec)
{
        int sb, rb, bufsize = 1;
        int size, rank;
	MPI_Comm nodeInComm = PDI_Exec->nodeInComm;
	MPI_Comm globalComm = PDI_Exec->globalComm;
        MPI_Comm_rank(nodeInComm, &rank);
        MPI_Comm_size(nodeInComm, &size);
	PDI_Exec->nodeInRank = rank;
	PDI_Exec->nodeSize = size;

        if(rank == 0)
        {
                sb = 1;
		PDI_Exec->amIaHead = 1;
        }
        else
        {
                sb = 0;
		PDI_Exec->amIaHead = 0;
        }

        MPI_Allreduce(&sb, &rb, bufsize, MPI_INT, MPI_SUM, globalComm);
        //printf("Host count is %d \n", rb);
	PDI_Exec->nbNodes = rb;
        return rb;
}

/*--------------------------------------------------------------------------*/
/** 
 	@brief 		Test MPI implementation's support for thread
 	@param		
 	@return		void
**/
/*--------------------------------------------------------------------------*/

void PDI_TestThreadSupport()
{
        int provided, flag;

        //check the value of these different thread level
        /*
	printf("MPI_THREAD_SINGLE value is %d\n", MPI_THREAD_SINGLE);
        printf("MPI_THREAD_FUNNELED value is %d\n", MPI_THREAD_FUNNELED);
        printf("MPI_THREAD_SERIALIZED value is %d\n", MPI_THREAD_SERIALIZED);
        printf("MPI_THREAD_MULTIPLE value is %d\n", MPI_THREAD_MULTIPLE);
	*/

        MPI_Query_thread(&provided);

        //export which thread level the MPI is supporting
        switch(provided)
        {
                case (MPI_THREAD_SINGLE):
                        //printf("The thread level this MPI implementation supports is MPI_THREAD_SINGLE\n");
                        break;
                case (MPI_THREAD_FUNNELED):
                        //printf("The thread level this MPI implementation supprots is MPI_THREAD_FUNNELED\n");
                        break;
                case (MPI_THREAD_SERIALIZED):
                        //printf("The thread level this MPI implementation supports is MPI_THREAD_SERIALIZED\n");
                        break;
                case (MPI_THREAD_MULTIPLE):
                        //printf("The thread level this MPI implementation supprots is  MPI_THREAD_MULTIPLE\n");
                        break;
                default:
                        printf("This MPI implementation can't support any thread\n");
        }

        MPI_Is_thread_main(&flag);
        if(flag)
        {
                //printf("provided's value is %d, this is main thread\n", provided);
        }
        else
        {
                //printf("provided's value is %d, this is not main thread\n", provided);
        }

}


/*--------------------------------------------------------------------------*/
/** 
	@brief		Create a node sequence, no repeat
 	@param		nodeCount	Count of nodes in this system
 	@return		integer		PDI_SCES if successful
**/
/*--------------------------------------------------------------------------*/

int PDI_BuildNodeList(PDIT_execution* PDI_Exec)
{
        int i, found, pos, nbNodes = 0, rank, nbProc;
	rank = PDI_Exec->myRank;
	nbProc = PDI_Exec->nbProc;
	int nodeCount = PDI_Exec->nbNodes;
	
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
	
	PDI_Exec->nodeID = nodeList[rank];
	printf("This rank %d is in the nodeID %d\n", rank, PDI_Exec->nodeID);
	
	
	//Show the detail information
        for(i = 0; i < nodeCount; i++)
        {
                strncpy(hname, nameList + (i * PDI_BUFS), PDI_BUFS -1);
                //printf("The %d element of nameList is %s\n", i, hname);
        }
        for(i = 0; i < nbProc; i++)
        {
                //printf("rank %d , host ID is %d\n", i, nodeList[i]);
        }
	return PDI_SCES;
}



/*--------------------------------------------------------------------------*/
/**
        @brief          Create a node sequence, no repeat
        @param          nodeCount       Count of nodes in this system
	@param 		nameList	array of host name, no repeat
	@param 		nodeList	array of every proc corresponding host position in nameList
        @return         integer         PDI_SCES if successful
**/
/*--------------------------------------------------------------------------*/

int PDI_BuildNodeList2(PDIT_execution* PDI_Exec, char* nameList, int* nodeList)
{
        int i, found, pos, nbNodes = 0, rank, nbProc;
        rank = PDI_Exec->myRank;
        nbProc = PDI_Exec->nbProc;
        int nodeCount = PDI_Exec->nbNodes;

        //nameList is the node name list, no repeate
        //char *nameList = talloc(char, nodeCount * PDI_BUFS);
        //int* nodeList = talloc(int, nbProc);

        //nodeList the i element corresponding rank-i
        //and the value represent it's rank ID
        for(i = 0; i < nbProc ; i++)
        {
                nodeList[i] = -1;
        }

	// lhn is the list of hostname of all procs
        char hname[PDI_BUFS], str[PDI_BUFS], *lhn;
        lhn = talloc(char, PDI_BUFS * nbProc);
        memset(lhn + (rank * PDI_BUFS), 0, PDI_BUFS);
        gethostname(lhn + (rank * PDI_BUFS), PDI_BUFS);

        strncpy(hname, lhn + (rank * PDI_BUFS), PDI_BUFS -1);
	
	//show the host info every rank located on 
	//printf("[@@@@ DEBUG_INFO @@@@]: I am rank %d, located on host:%s\n", rank, hname);

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

        PDI_Exec->nodeID = nodeList[rank];
        //printf("This rank %d is in the nodeID %d\n", rank, PDI_Exec->nodeID);


        //Show the detail information
        for(i = 0; i < nodeCount; i++)
        {
                strncpy(hname, nameList + (i * PDI_BUFS), PDI_BUFS -1);
                //printf("The %d element of nameList is %s\n", i, hname);
        }
        for(i = 0; i < nbProc; i++)
        {
                //printf("rank %d , host ID is %d\n", i, nodeList[i]);
        }
        return PDI_SCES;
}

/*----------------------------------------------------------------------------------*/
/**
	@brief	gather the pid of all MPI processes in this local node
		and then send it to the daemon process by rank 0 of
		the original WORLD group
**/
/*----------------------------------------------------------------------------------*/
int PDI_SendPid(PDIT_execution* PDI_Exec)
{
	int pid = PDI_Exec->pid;
	int nbProc = PDI_Exec->nodeSize;
	int* pidArray = talloc(int, nbProc);
	if(pidArray == NULL)
	{
		return PDI_NSCS;
	}
	MPI_Gather(&pid, 1, MPI_INT, pidArray, 1, MPI_INT, 0, PDI_Exec->nodeInComm);
	
	//Just for test
	if(PDI_Exec->nodeInRank == 0)
	{
		int i;
		for(i = 0; i < nbProc; i++)
		{
			//printf("[DEBUG_INFO]: processId[%d] %d\n", i, pidArray[i]);
		}
	}

	if(PDI_Exec->nodeInRank == 0)
	{
		//non-block send
		/*
		MPI_Request sreq;
		MPI_Status sstatus;
		int tag = 123;
		MPI_Isend(pidArray, nbProc, MPI_INT, 0, tag, PDI_Exec->daemonComm, &sreq);
		MPI_Wait(&sreq, &sstatus);
		*/
		//block send
		int tag = 123;
		MPI_Send(pidArray, nbProc, MPI_INT, 0, tag, PDI_Exec->daemonComm);
	}
	free(pidArray);
	return PDI_SCES;
}


/*----------------------------------------------------------------------------------*/
/**
	record the host set
**/
/*----------------------------------------------------------------------------------*/
int PDI_RecordHostSet(PDIT_execution* PDI_Exec, char* fileName, char* nameList)
{
	FILE* stream;
	if((stream = fopen(fileName, "a+")) == NULL)
	{
		return PDI_NSCS;
	}
	//write the head
	char* phead = "<nameID : hostName>\n";
	int len = strlen(phead);
	fwrite(phead, len, 1, stream);
	//write <key-value>
	int i;
	char* buf = talloc(char, PDI_BUFS);
	for(i = 0; i < PDI_Exec->nbNodes; i++)
	{
		sprintf(buf, "<%d : %s>\n", i, nameList + i * PDI_BUFS);
		len = strlen(buf);
		fwrite(buf, len, 1, stream);
	}
	fflush(stream);
	fclose(stream);
}



/*----------------------------------------------------------------------------------*/
/**
	@brief	log by system call
**/
/*----------------------------------------------------------------------------------*/
int PDI_Logger_bsc(char* fileName, char* buf, int len)
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
int PDI_Logger(char* fileName, char* buf, int len)
{
        FILE* stream;
        if((stream = fopen(fileName, "a+")) == NULL)
        {
                return PDI_NSCS;
        }
        int result = fwrite(buf, len, 1, stream);
        printf("fwrite counts is %d \n", result);
        fflush(stream);
        fclose(stream);
}

/*-----------------------------------------------------------------------------------------*/
/**
	@brief	set release of the limit about coredump
		If not set, when the MPI program occured very bad errors
		the process in the remote node may be can't generate a coredump file
		because of the configure changed by MPI or ssh environment
**/
/*-----------------------------------------------------------------------------------------*/
int PDI_SetCoreLimit()
{
	struct rlimit core_limit;
	core_limit.rlim_cur = RLIM_INFINITY;
	core_limit.rlim_max = RLIM_INFINITY;
	if(setrlimit(RLIMIT_CORE, &core_limit) < 0)
	{
		//printf("[ERROR]: setrlimt in PDI_SetCoreLimit\n");
		return PDI_NSCS;
	}
	return PDI_SCES;
}

