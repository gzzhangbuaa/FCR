#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include "chinterface.h"


//get pid of the work process in this node by appication name
void getPidByName(pid_t *pid, char* task_name, int dumpSwitch)
{
        printf("task_name is %s\n", task_name);
        DIR* dir;
        struct dirent* ptr;
        FILE* fp;
        char filepath[PDI_BUFS];
        char cur_task_name[PDI_BUFS];
        char buf[PDI_BUFS];

        // Open a log file to record the processId when gcore these processes
        FILE* stream;

	if(dumpSwitch == PDI_EnableDump)
	{
		if((stream = fopen(PDI_DUMPABOUT, "a+")) == NULL)
        	{
                	printf("Open file gcoreabout.txt ERROR.\n");
                	return;
        	}
	}

        // Get processId through comparing the processName with that in proc
        dir = opendir("/proc");
        if(NULL != dir)
        {
		int counter = 0;
                while((ptr = readdir(dir)) != NULL)
                {
                        if((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                                continue;
                        if(DT_DIR != ptr->d_type)
                                continue;
                        sprintf(filepath, "/proc/%s/status", ptr->d_name);
                        fp = fopen(filepath, "r");
                        if(NULL != fp)
                        {
                                if(fgets(buf, PDI_BUFS, fp) == NULL)
                                {
                                        fclose(fp);
                                        continue;
                                }
                                sscanf(buf, "%*s %s", cur_task_name);
                                //printf("task_name:%s,cur_task_name:%s\n", task_name, cur_task_name);
                                if(!strcmp(task_name, cur_task_name))
                                {
                                        printf("Compre ing:");
                                        printf("PID:%s\n",ptr->d_name);
                                        sscanf(ptr->d_name, "%d", pid + counter);
					printf("[DEBUG_INFO]: get a pid and translate into an int num %d !!!!\n", pid[counter]);
					counter++;
					
                                        // Write current process information into the local gcoreabout.txt file
					if(dumpSwitch == PDI_EnableDump)
					{
                                        	char pidBuff[PDI_BUFS];
                                        	sprintf(pidBuff, "[gcore] <groupID : %d> <nodeID : %d> <posInGroup : %d> <hostName : %s> <ProcessName : %s> <ProcessId : %s>\n", 
							PDI_Daem.groupID, PDI_Daem.nodeID, PDI_Daem.posInGroup, PDI_Daem.hostName, task_name, ptr->d_name);
                                        	int len = strlen(pidBuff);
                                       		int res = fwrite(pidBuff, len, 1, stream);
                                        	printf("write len is %d \n", res);

                                        	//generate a coredump file for this pid process
                                        	char cmd[PDI_BUFS];
                                        	sprintf(cmd, "gcore -o %score_%s %s", PDI_DUMPPATH, PDI_Daem.hostName, ptr->d_name);
                                        	system(cmd);
                                        	printf("[pid :%s done]\n", ptr->d_name);
					}
                                }
                                fclose(fp);
                        }
                }
                closedir(dir);
        }

        // Close the fd opened previous
	if(dumpSwitch == PDI_EnableDump)
	{
        	fflush(stream);
		fclose(stream);
	}
}

/*---------------------------------------------------------------------------------------------------*/
/**
	@brief	make coredumps for all work processes on this node
	@param	program when the application running
**/
/*---------------------------------------------------------------------------------------------------*/
void takeCoreDump(char* taskName)
{
	//pid_t array was used to maintain the processId that found to be still exist
	int i;
	pid_t* pid = talloc(pid_t, PDI_Daem.nodeSize);
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{
		pid[i] = -1;  //set default value
	}
	getPidByName(pid, taskName, PDI_EnableDump);
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{
		printf("[Found Exist Pid]: %d @@@@@@@########\n", pid[i]);
	}
	free(pid);
}

/*---------------------------------------------------------------------------------------------------*/
/**
	@brief	make a coredump by a single process id
**/
/*---------------------------------------------------------------------------------------------------*/
void PDI_DumpForProc(int pid)
{
	char cmd[PDI_BUFS];
	sprintf(cmd, "gcore -o %score_%s %d", PDI_DUMPPATH, PDI_Daem.hostName, pid);
	system(cmd);
}


/*--------*------------------------------------------------------------------------------------------*/
/**
        @brief
**/
/*---------------------------------------------------------------------------------------------------*/
void PDI_DumpForNode()
{
        takeCoreDump(PDI_Daem.appName);
}




int PDI_GetProcStatus(PDIT_execstatus* PDI_ExecStatus, char* descStr)
{
	int i;
        PDI_ExecStatus->nodeStatus.nodeID = PDI_Daem.nodeID;
        PDI_ExecStatus->nodeStatus.groupID = PDI_Daem.groupID;
        PDI_ExecStatus->nodeStatus.posInGroup = PDI_Daem.posInGroup;
        strncpy(PDI_ExecStatus->nodeStatus.hostName, PDI_Daem.hostName, PDI_BUFS);
        PDI_ExecStatus->nodeStatus.status = PDI_ALIVE;

        PDI_ExecStatus->nbProcs = PDI_Daem.nodeSize;
	
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{
		printf("[DEBUG_DATA] : processId[%d] : %d\n", i, PDI_Daem.body[i]);
	}	

        memcpy(PDI_ExecStatus->processId, PDI_Daem.body, PDI_BUFS * sizeof(int));
	memcpy(PDI_ExecStatus->pmpStatus, PDI_Daem.procStatus, PDI_BUFS * sizeof(int));

	pid_t* pid = talloc(pid_t, PDI_Daem.nodeSize);
	if(pid == NULL)
	{
		return PDI_NSCS;
	}
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{
		pid[i] = -1;
	}
	getPidByName(pid, PDI_Daem.appName,PDI_DisableDump);
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{
		printf("[Found Exist Pid]: %d @@@@@@######\n", pid[i]);
	}

	//Detemine which proc still exists and which has exit abnormally
	for(i = 0; i < PDI_Daem.nodeSize; i++)
	{
		int j, flag = PDI_NSCS;
		for(j = 0; j < PDI_Daem.nodeSize; j++)
		{
			if(PDI_ExecStatus->processId[i] == pid[j])
			{
				flag = PDI_SCES;
				break;
			}
		}
		if(flag == PDI_SCES)
		{
			PDI_ExecStatus->peStatus[i] = PDI_EXIST;
		}
		else
		{
			PDI_ExecStatus->peStatus[i] = PDI_ABORT;
		}
		printf("[###Get_ProcStatus i= %d, pmp=%d, pe=%d]\n", i, PDI_ExecStatus->pmpStatus[i], PDI_ExecStatus->peStatus[i]);
	}
	if(pid != NULL)
	{
		free(pid);
	}
	
	int limitSize = PDI_BUFS * 4;
	if(descStr != NULL)
	{
		printf("[#### descStr ####]: %s", descStr);
		int realSize = strlen(descStr);
		if(limitSize <= realSize)
		{
			strncpy(PDI_ExecStatus->description, descStr, limitSize);	
		}
		else
		{
			strncpy(PDI_ExecStatus->description, descStr, realSize);
		}
		printf("[#### PDI_ExecStatus->description ####]:%s", PDI_ExecStatus->description);
	}
	else
	{
		strncpy(PDI_ExecStatus->description, "No more description for the error event!\n", limitSize);
	}
	return PDI_SCES;
}





