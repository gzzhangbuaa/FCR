/**
 *	@brief 	pdi.h
 *	@author ZhangGuozhen
 *	@brief 	header file for PDI library
**/

#ifndef _PDI_H
#define _PDI_H

#include <mpi.h>

/*----------------------------------------------------------------------
				Defines
----------------------------------------------------------------------*/

/** Standard size of buffer 					      */
#define PDI_BUFS 256
/** Token returned when PDI performs a xxx			      */
#define PDI_DONE 1
/** Token returned if a PDI function succeeds			      */
#define PDI_SCES 0
/** Token returned if a PDI function fails			      */
#define PDI_NSCS -1

/**     respresent the host type     		                     **/
#define PDI_WORKHOST    0
#define PDI_CENTERHOST  1

/**	open coredump or not					     **/
#define PDI_EnableDump	1
#define	PDI_DisableDump	0

/** 	open detect or not					     **/
#define PDI_EnableDete	1
#define	PDI_DisableDete	0

/*---------------------------------------------------------------------------------*/
/**
        @brief MPI messages type for different function
**/
/*---------------------------------------------------------------------------------*/
#define PDI_TYPE_MSGPNF         0       /** notification for message passing notify     */
#define PDI_TYPE_JOBFNF         1       /** notification for job finishing              */
#define PDI_TYPE_TRIGNF		2	/** notification for tirgger detective & dump	*/
#define PDI_TYPE_HWFTST		3	/** notification for failure test		*/

/*---------------------------------------------------------------------------------*/
/**
 * 	@brief type for failure test
**/
/*----------------------------------------------------------------------------------*/
#define PDI_TYPE_SFAIL		0	/** only create software failures for test	*/
#define PDI_TYPE_HFAIL		1	/** to simulate hardware failures for teset	*/	


#ifdef __cplusplus
extern "C" {
#endif

/** @typedef	PDI_execution
 *  @brief	Execution metadata
 *
 *  This type stores all the dynamic metadata related to the current execution
**/

typedef struct PDIT_execution			/** Topology metadata					*/
{
	int 		pid;			/** Process id of this MPI proc				*/
	MPI_Comm	globalComm;		/** Global communicator					*/
	MPI_Comm	nodeInComm;		/** Local node communicator between MPI work procs	*/
	MPI_Comm	daemonComm;		/** Local inter-communicator between work procs and daemon */
	int		nbProc;			/** Total global number of proc 			*/
	int 		nbNodes;		/** Total global number of nodes			*/
	int 		myRank;			/** My rank on the global comm 				*/
	int 		nodeInRank;		/** My rank on the interNode comm 			*/
	int 		nodeSize;		/** Proc number in local node				*/
	int 		nodeID;			/** Node ID in the system				*/
	int 		groupSize;		/** Count of nodes which one group contains		*/
	int 		amIaHead;		/** True if minimal rank in local node 			*/
	int 		headRank;		/** Rank of the head in local node 			*/
	int 		body[PDI_BUFS];		/** List of app. proc. in local node 			*/
	int 		status[PDI_BUFS];	/** Status of app. proc.in local node 			*/
	int		amICenter;		/** am I the center host of not				*/
} PDIT_execution;


typedef struct PDIT_detail			/** detail info about this proc running */
{
	int pid;				/** pid of this MPI proc		*/
	int rank;				/** rank in global comm of this proc	*/
	int codeLine;				/** line number of last message action	*/
	char functionName[PDI_BUFS];		/** function name of last message happened */
} PDIT_detail;


/*------------------------------------------------------------------------------------------
				Global variables
------------------------------------------------------------------------------------------*/

/** MPI communicator that splits the global one into sub-comm in each node.		*/
extern MPI_Comm PDI_COMM_NodeIn;


/*------------------------------------------------------------------------------------------
				PDI public functions
-------------------------------------------------------------------------------------------*/
int PDI_Init();
int PDI_Knock();

int PDI_FailureTest(int nodeID, int type);
/*-----------------------------------------------------------------------------------------*/
/**
	@brief	an interface provided to app level to trigger this detect round
	@param	integer		dumpOrNot	detemine wether to do corecump 
						before report the suspicious
		integer		deteOrNot	detemine wether need to do node status detect
		char*		errStr		description for error
		integer		len		string length of errStr 
**/
/*-----------------------------------------------------------------------------------------*/

int PDI_Trigger(int dumpOrNot, int deteOrNot, char* errStr, int len);
int PDI_Finalize();

#ifdef __cplusplus
}
#endif

#endif /*-----------#ifndef _PDI_H---------*/
