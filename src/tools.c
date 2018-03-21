/**
 *	@file	tools.c
 *	@author	ZhangGuozhen
 *	@brief	Utility functions for the PDI library
**/

#include "interface.h"


/*--------------------------------------------------------------------------*/
/**
	@brief	Get application name 
	@param	app directionary path (in, out)
	@param	app name	      (in, out)
	@param 	the lenth of the execuation path
**/
/*--------------------------------------------------------------------------*/
size_t GetPath(char* processdir, char* processname, size_t len)
{
        char* path_end;
        if(readlink("/proc/self/exe", processdir, len) <= 0)
        {
                return -1;
        }
	//printf("[##########dir is %s ############]\n", processdir);

        path_end = strrchr(processdir, '/');
        if(path_end == NULL)
        {
                return -1;
        }
        ++path_end;

	//printf("[@@@@@@ Path_end is %s @@@@]\n", path_end);
        strcpy(processname, path_end);
        *path_end = '\0';
        return (size_t)(path_end - processdir);
}

int PDI_GetPath(char* appName)
{
	char* procDir = talloc(char, PDI_BUFS);
	memset(procDir, 0, PDI_BUFS);
	if(GetPath(procDir, appName, PDI_BUFS) < 0)
	{
		//printf("[GetPath]: something wrong in this place, you should check it!\n");
		free(procDir);
		return PDI_NSCS;
	}
	free(procDir);
	return PDI_SCES;
}

