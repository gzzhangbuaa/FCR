/**
 *	@file	interface.h
 *	@author	ZhangGuozhen
 *	@brief	header file for the PDI library private functions.
**/

#ifndef	_PDI_INTERFACE_H
#define _PDI_INTERFACE_H

#include "pdi.h"

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/resource.h>
/*----------------------------------------------------------------------------
				Defines
-----------------------------------------------------------------------------*/
/** Malloc macro							     */
#define talloc(type, num)	(type *)malloc(sizeof(type) * (num))

/** Set log file names							     */
#define	PDI_ALLHOST	"./allhost.log"



/*----------------------------------------------------------------------------
			PDI private functions
-----------------------------------------------------------------------------*/
int PDI_SplitCommByNode(PDIT_execution* PDI_Exec);
int PDI_GetNodeCount(PDIT_execution* PDI_Exec);
void PDI_TestThreadSupport();
int PDI_BuildNodeList(PDIT_execution* PDI_Exec);
int PDI_BuildNodeList2(PDIT_execution* PDI_Exec, char* nameList, int* nodeList);

int PDI_CreateDaemon(char* fileName, PDIT_execution* PDI_Exec);
int PDI_GetPath(char* appName);
int PDI_SetCoreLimit();

int PDI_SendPid(PDIT_execution* PDI_Exec);

int PDI_RecordHostSet(PDIT_execution* PDI_Exec, char* fileName, char* nameList);
int PDI_Logger_bsc(char* fileName, char* buf, int len);
int PDI_Logger(char* fileName, char* buf, int len);

#endif
