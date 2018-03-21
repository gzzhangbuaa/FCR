#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
		
	gethostname(hostName, BUFFERSIZE);	
	
	/*
	int universe_size, flag, *universe_sizep;

	MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &universe_sizep, &flag);	
	MPI_Attr_get(MPI_COMM_SELF, MPI_UNIVERSE_SIZE, &universe_sizep, &flag);	
	if(!flag)
	{
		printf("This MPI does not support UNIVERSE_SIZE.\n ");
	}
	else
	{
		universe_size = *universe_sizep;
		printf("universe_size is %d\n", universe_size);
	}
	*/

	
	int pid = getpid();
	printf("[I am rank %d as parent, size is %d hostName is %s pid is %d...before]!!!\n", rank, size, hostName, pid);
	MPI_Info hostinfo;
	MPI_Info_create(&hostinfo);
	MPI_Info_set(hostinfo, "host", hostName);
	//MPI_Info_set(hostinfo, "wdir", "./");
	
	
	//MPI_Comm_spawn("./child", NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm, &errs);
	MPI_Comm_spawn("./child", NULL, 1, hostinfo, rank, MPI_COMM_WORLD, &newcomm, &errs);
	
	int ssize, srank;
	MPI_Comm_size(newcomm, &ssize);
	MPI_Comm_rank(newcomm, &srank);
	printf("my rank is %d , now I am parent and the interComm size is %d, srank is %d\n", rank, ssize, srank);
		

	/*	
	if(rank == 1)
	{	
		printf("[I am rank %d as parent, size is %d hostName is %s...before]!!!\n", rank, size, hostName);
		MPI_Comm_spawn("./child", NULL, 4, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm, &errs);
		printf("[I am rank %d as parent, size is %d hostName is %s...after]!!!\n", rank, size, hostName);
	
		int ssize, srank;
		MPI_Comm_size(newcomm, &ssize);
		MPI_Comm_rank(newcomm, &srank);
		printf("[NewComm size is %d, rank of parent in interComm is %d]@@@@@@@@@\n", ssize, srank);
	}
	*/
	
	/*
	if(rank == 1)
	{
		printf("rank is %d,now I spawn a new proc!!!!!!\n",rank);
		MPI_Comm_spawn("./child", NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &newcomm, &errs);
	}
	else
	{

		MPI_Comm_spawn("./child", NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &newcomm, &errs);
	}
	*/

	getchar();	

	//printf("HostName: %s, ProcSize: %d, MyRank: %d\n", hostName, size, rank);
	MPI_Finalize();
	return 0;
}
