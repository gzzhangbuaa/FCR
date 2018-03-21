#ifndef _PDI_CHINTERFACE_H
#define	_PDI_CHINTERFACE_H


#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>

/*----------------------------------------------------------------------------------
				Defines
----------------------------------------------------------------------------------*/
/** Malloc macro								  */
#define talloc(type, num)	(type *)malloc(sizeof(type) * (num))


/**	Standard buffer size	**/
#define PDI_BUFS 	256
/**	Cordon for trigger	**/
#define PDI_CORDON 	3	

/**	Process status 		**/
/** 	ALIVE represents frequently msg passing happens	or node can response for a request	**/
/**	DEAD  represents enough long time on msg passing or node can't response for a request 	**/
#define PDI_ALIVE	0
#define	PDI_DEAD	1

/** 	Two kinds under the PDI_DEAD suitation	**/
/**	Process still exists but do not execute further, like a zombie may be casued by dead lock	**/
#define	PDI_EXIST	0
/**	Process exit abnormally may be caused by signal likes kill or other reasons			**/
#define	PDI_ABORT	1



/** Token returned if a PDI function succeeds	*/
#define	PDI_SCES	0
/** Token returned if a PDI function fails	*/
#define PDI_NSCS	-1

/** Open coredump function or not		*/
#define	PDI_EnableDump	1
#define PDI_DisableDump 0

/*---------------------------------------------------------------------------------*/
/**
	@brief	socket messages type for different function
**/
/*---------------------------------------------------------------------------------*/
#define PDI_TYPE_ABREPT		0         /* for abnormal Event report		*/
#define	PDI_TYPE_SPECIFY	1	  /* for specify the group leader	*/
#define PDI_TYPE_DETECT		2	  /* for node status detect		*/
#define PDI_TYPE_SRESP		3    	  /* for node status response		*/
#define PDI_TYPE_GLRESP		4  	  /* for group leader response		*/

#define PDI_TYPE_REQUEST	5	  /* for a simple test			*/
#define PDI_TYPE_JUSTLOG	6	  /* for only log no detection	        */

#define PDI_TYPE_FTEST		7	  /* for failure test to stop serverTCP */
/*---------------------------------------------------------------------------------*/
/**
	@brief MPI messages type for different function
**/
/*---------------------------------------------------------------------------------*/
#define	PDI_TYPE_MSGPNF		0	/** for message passing notify		*/
#define PDI_TYPE_JOBFNF		1	/** for job finishing	notify		*/
#define PDI_TYPE_TRIGNF		2	/** for error event trrigger		*/
#define PDI_TYPE_HWFTST		3       /** for hardware faults test		*/


/**	coredump or not								*/
#define	PDI_EnableDump		1
#define	PDI_DisableDump		0

/**	detect or not								*/
#define	PDI_EnableDete		1
#define	PDI_DisableDete		0

/*---------------------------------------------------------------------------------*/
/**	
	@brief	Single event or whole node as the trigger
	PROCMODEL represents if one process happened timeout
	then immediately report the suspection to central control
	NODEMODEL represents only all the processed on this node
	had happened timeout, then report this suspection to center
**/
/*---------------------------------------------------------------------------------*/
#define	PDI_PROCMODEL 	0
#define PDI_NODEMODEL	1

/**	Report to the central control or not		**/
#define	PDI_REPORT	0
#define PDI_UNREPORT	1

/**	respresent the host type			**/
#define	PDI_WORKHOST	0
#define	PDI_CENTERHOST	1				

/**	am I group leader or not			**/
#define	PDI_GROUPMEMBER	0
#define	PDI_GROUPLEADER	1

/**	control got the node status from group leader report or not	**/
#define	PDI_GOTGR	1
#define	PDI_NGOTGR	0
/**	Nodes in a group are all dead			**/
#define	PDI_EXTINCT	-1				

/**	checking on					**/
#define PDI_Checking	0
#define PDI_UnChecking	1

/*---------------------------------------------------------------------------------*/
/**
	@brief	set log file names
**/
/*---------------------------------------------------------------------------------*/
#define	PDI_ALLHOST		"./allhost.log"
#define	PDI_DLAUNCHER		"./dlauncher.log"
#define	PDI_NODESTATUS		"./nstatus.log"
#define	PDI_SUSPECTEVENT	"./suspect.log"
#define PDI_DUMPABOUT		"./dumpabout.log"
#define PDI_DUMPPATH		"HOME/pp442/BIGDATA/coredumps"

typedef struct PDIT_daemon
{
	MPI_Comm	parent;					/** The inter-comm between parent and child 			**/
	int 		body[PDI_BUFS];				/** The process pid of MPI work process in this local node	**/
	char		hostName[PDI_BUFS];			/** Local host name						**/
	int 		nodeSize;				/** The count of MPI work process in this node			**/
	int 		preFlag[PDI_BUFS];			/** The history status flag of all these local MPI processes	**/
	int		curFlag[PDI_BUFS];			/** The current status flag of all these local MPI processes	**/
	int 		realCount[PDI_BUFS];			/** Tick timing for timeout for each local MPI process		**/
	int 		procStatus[PDI_BUFS];			/** Dead or alive for each local MPI process			**/
	int 		eventModel;				/** Single process or the whole node abnormal to trigger report **/
	int		reportOrNot;				/** Wether to report the suspection or not			**/
	int 		nodeID;					/** Node ID in nameList						**/
	int 		groupSize;				/** The count of nodes which one group contains	MAX		**/
	int 		realGSize;				/** Real count of nodes this group contains 			**/
	int		posInGroup;				/** The position of this node in the group			**/
	int 		groupID;				/** The ID of the group which local host being included in	**/
	int 		nbNodes;				/** Total nodes number in this application			**/
	char*		nameList;				/** All host name in this list no repeat			**/
	char* 		groupMembers;				/** All the host name combined into one group for detective	**/
	int* 		leaders;				/** Node ID of group leaders					**/
	char		ccName[PDI_BUFS];			/** Central control host name responsible for detective		**/
	char		appName[PDI_BUFS];			/** Application name						**/
	int		amICenter;				/** Local host is the center host or not			**/
	int 		amIGroupLeader;				/** Local daemon is the group leader or not			**/
	int		groupNums;				/** How many groups has been divided into			**/
	int		isChecking;				/** Detective is happening					**/
	int 		dumpSwitch;				/** Enable or disable dump when find suspect event		**/
}PDIT_daemon;

typedef struct PDIT_recvguider					/** This struct used to guid what to do next with this info	**/
{
	int		msgType;				/** different value represent different handle flow		**/
	int		msgSize;				/** the message size next will send				**/
}PDIT_recvguider;

typedef struct	PDIT_nodestatus					/** This struct used to translate node status to group leader	**/
{
	int 		nodeID;					/** node ID in the nameList of all nodes			**/
	int 		groupID;				/** The ID of group 						**/
	int		posInGroup;				/** The position of the node in the group			**/
	char		hostName[PDI_BUFS];			/** The host name of the node					**/
	int 		status;					/** Node status alive or dead					**/
}PDIT_nodestatus;


/*-------------------------------------------------------------------------------------------------------------------------------*/
/**
	@brief	when the remote node occured suspecious event and need to report it to the central controller
		some basic and extented infomation should be sent. Not only the pid of all the procs on this
		node, but aslo the procs are dead or alive.
		alive means there is frequently msg passing in this proc
		dead means there is no msg passing in this proc for a long time
		dead includes two kinds of suitation, one is proc still exists but happend block or dead lock etc.
		another is the proc exit abnormally like crash casued by segament fault etc.
		by checking the specific status of procs we can determine the dead lock, block, dead while etc.
		we can find out which MPI processed has abort, use it's coredump file we can do a further analysis.
**/
/*-------------------------------------------------------------------------------------------------------------------------------*/
typedef struct PDIT_execstatus					/** All procs execuation status in this node 			**/
{
	PDIT_nodestatus	nodeStatus;				/** Basic info about local node					**/
	int		nbProcs;				/** Numbers of MPI work processes in this node			**/
	int		processId[PDI_BUFS];			/** Maintain the pid of all procs in this node			**/
	int		pmpStatus[PDI_BUFS];			/** Maintain proc msg passing status				**/
	int		peStatus[PDI_BUFS];			/** Maintain proc exit	status					**/
	char		description[PDI_BUFS * 4];		/** Description of the error event				**/
}PDIT_execstatus;



void PDI_AddTimer();
void PDI_PauseTimer();
void PDI_SIGProcess();
void PDI_SetTimer();
void PDI_WaitKnocking();
void* PDI_WaitDetective();
void PDI_InitLArray();
void PDI_CheckSuspect();
void PDI_DumpForNode();
void PDI_DumpForProc();
int PDI_RecvPid(PDIT_daemon* PDI_daem);

int detectLauncher(char* fileName);

int logger(char* fileName, char* buf, int len);
int logger_bsc(char* fileName, char* buf, int len);

void serverTCP();
int clientTCP(char* hostName, int msgType, int msgSize, char* msg);
int PDI_GetProcStatus(PDIT_execstatus* PDI_ExecStatus, char* descStr);
int EventTrigger(int dumpOrNot, int deteOrNot, int size, char* msg);

void* detective();



PDIT_daemon PDI_Daem;
PDIT_nodestatus* PDI_NSList;     				/** for restore all the group members node status info		**/

int*	PDI_GroupReport;					/** for control center check wether have got all node status	**/

#endif
