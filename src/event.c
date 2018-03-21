#include "chinterface.h"

int EventTrigger(int dumpOrNot, int deteOrNot, int size, char* msg)
{
	printf("[##### EventTrigger ####]: size= %d %s\n",size, msg);
	//determine the error event would trigger coredump or not
	if(dumpOrNot == PDI_EnableDump)
	{
		PDI_DumpForNode();	
	}
	if(deteOrNot == PDI_EnableDete)
	{
		clientTCP(PDI_Daem.ccName, PDI_TYPE_ABREPT, size, msg);
	}
	else if(deteOrNot == PDI_DisableDete)
	{
		clientTCP(PDI_Daem.ccName, PDI_TYPE_JUSTLOG, size, msg);
	}
	else
	{
		printf("[DEBUG_INFO]: Param Error, deteOrNot in PDI_Trigger!\n");
		return PDI_NSCS;
	}
	return PDI_SCES;
}

