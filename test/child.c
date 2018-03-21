#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE 100


int main(int argc, char* argv[])
{
	int i;
	int size;
	int rank;
	char hostName[BUFFERSIZE];
	int nameLen;
	MPI_Comm parent;
	
	int provided, flag;

	//check the value of these different thread level 
	printf("MPI_THREAD_SINGLE value is %d\n", MPI_THREAD_SINGLE);
	printf("MPI_THREAD_FUNNELED value is %d\n", MPI_THREAD_FUNNELED);
	printf("MPI_THREAD_SERIALIZED value is %d\n", MPI_THREAD_SERIALIZED);
	printf("MPI_THREAD_MULTIPLE value is %d\n", MPI_THREAD_MULTIPLE);

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

	//export which thread level the MPI is supporting
	switch(provided)
	{
		case (MPI_THREAD_SINGLE):
			printf("The thread level this MPI implementation supports is MPI_THREAD_SINGLE\n");
			break;
		case (MPI_THREAD_FUNNELED):
			printf("The thread level this MPI implementation supprots is MPI_THREAD_FUNNELED\n");
			break;
		case (MPI_THREAD_SERIALIZED):
			printf("The thread level this MPI implementation supports is MPI_THREAD_SERIALIZED\n");
			break;
		case (MPI_THREAD_MULTIPLE):
			printf("The thread level this MPI implementation supprots is  MPI_THREAD_MULTIPLE\n");
			break;
		default:
			printf("This MPI implementation can't support any thread\n");
	}
	
	MPI_Is_thread_main(&flag);
	if(flag)
	{
		printf("provided's value is %d, this is main thread\n", provided);
	}
	else
	{
		printf("provided's value is %d, this is not main thread\n", provided);
	}	

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//getchar();	
	MPI_Comm_get_parent(&parent);
	int ssize, srank;
	MPI_Comm_size(parent, &ssize);
	MPI_Comm_rank(parent, &srank);
		
	gethostname(hostName, BUFFERSIZE);

	int rb[2];
	MPI_Status rstatus;
	while(1)
	{
		MPI_Recv(rb, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, parent, &rstatus);
		printf("I have receive a message and rank is %d, pid is %d\n", rb[0], rb[1]);
	}

	for(i = 0; i < 100; i++)
	{
		printf("myhost is %s, I am child, my rank in MPI_COMM_WORLD is %d, the size of this MPI_COMM_WORLD is %d interComm size is %d, current proc in interComm rank is %d\n", hostName, rank, size, ssize, srank);
		sleep(5);
	}
	
	MPI_Finalize();
	return 0;
}
