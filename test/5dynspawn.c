#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pdi.h"

int main(int argc, char* argv[])
{
	
	MPI_Init(&argc, &argv);
	PDI_Init();
	MPI_Barrier(MPI_COMM_WORLD);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	PDI_Knock();	
	//PDI_Finalize();
	int i = 0;
	int dumpOrNot = PDI_DisableDump;
	int deteOrNot = PDI_DisableDete;
	char errMsg[PDI_BUFS];
	strncpy(errMsg, "hello default msg!\n", PDI_BUFS);	
	int len = strlen(errMsg);
	
	//test for event trigger
	if(rank == 5)
	{
		PDI_Trigger(dumpOrNot, deteOrNot, errMsg, len);
	}

	//test for msg passing notify

	/*
	while(1)
	{	i++;
		PDI_Knock();
		sleep(20);
		if(i % 5 == 0 && rank == 5)
		{
			printf("[#### Trigger START ####]!\n");
			char str[PDI_BUFS];
			strcpy(str, "HelloWorld!\n");
			int len = strlen(str);
			PDI_Trigger(PDI_DisableDump, PDI_EnableDete, str, len);
			printf("[#### Trigger END,len:%d, msg:[%s] ####]!\n", len, str);
			
		}
	}
	*/


	while(i == 20)
	{
		i = i++;
		sleep(30);
	}

	MPI_Finalize();
	return 0;
}

