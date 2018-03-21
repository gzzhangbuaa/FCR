#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BUFFERSIZE	100

#define talloc(type, num) (type *)malloc(sizeof(type) * (num))

int main(int argc, char* argv[])
{
	int i;
	int size;
	int rank;
	char hostName[BUFFERSIZE];
	int nameLen;

	MPI_Comm newcomm;
	int errs = 0;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	
	//print the allgather result 
	/*
	if(rank == 0)
	{
		int i = 0;
		for(i = 0; i < size; i++)
		{
			strncpy(hostName, allHostName + (i * BUFFERSIZE), BUFFERSIZE -1);
			printf("[rank is %d, hostName is %s]\n", rank, hostName);
		}
	}
	*/

	//examine the allgather operation in the sequence
	/*
	int* allRank;
	allRank = talloc(int, size);
	int i = 0;
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

	printf("my rank is %d\n", rank);
	MPI_Comm_spawn("./child", MPI_ARGV_NULL, 1, MPI_INFO_NULL, rank, MPI_COMM_WORLD, &newcomm, &errs);
	/*
	if(rank % 2 == 0)
	{
		printf("[rank is %d, before spawn!]\n", rank);	
		MPI_Comm_spawn("./child", MPI_ARGV_NULL, 1, MPI_INFO_NULL, rank, MPI_COMM_WORLD, &newcomm, &errs);
		printf("[rank is %d, after spawn!]\n", rank);
	}
	else
	{
		MPI_Comm_spawn("./child", MPI_ARGV_NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &newcomm, &errs);
	}
	*/
	/*		
	if(rank == 0)
	{
		printf("[spawn one...current rank is %d]\n", rank);
		MPI_Comm_spawn("./child", NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm, &errs);

		printf("[spawn two...current rank is %d]\n", rank);
		MPI_Comm_spawn("./child", NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm, &errs);
	}
	*/
	//int trank, tsize;
	//MPI_Comm_rank(newcomm, &trank);
	//MPI_Comm_size(newcomm, &tsize);
	//printf("Iam host %s, rank is %d, but in the newcomm, trank is %d and tsize is %d\n", hostName, rank, trank, tsize);

	getchar();	
	//printf("HostName: %s, ProcSize: %d, MyRank: %d\n", hostName, size, rank);
	MPI_Finalize();
	return 0;
}
