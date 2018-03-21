/**
 *	@file	daemon.c
 *	@author ZhangGuozhen
 *	@brief	functions of local daemon process
**/

#include "interface.h"

/** Running information about local MPI Proc message passing		*/
static PDIT_detail PDI_Detail;



int PDI_CreateDaemon(char* fileName, PDIT_execution* PDI_Exec)
{
	MPI_Comm splitWorld = PDI_Exec->nodeInComm;
	MPI_Comm* interComm = &PDI_Exec->daemonComm;
        char hostName[PDI_BUFS];
        gethostname(hostName, PDI_BUFS);
        int errs = 0;
        MPI_Info hostinfo;
        MPI_Info_create(&hostinfo);
        MPI_Info_set(hostinfo, "host", hostName);
        MPI_Comm_spawn(fileName, MPI_ARGV_NULL, 1, hostinfo, 0, splitWorld, interComm, &errs);
}

