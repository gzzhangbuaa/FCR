#include "mpi.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	MPI_Comm newcomm;	
	int rank, size;
	char port_name_out[MPI_MAX_PORT_NAME];
	char serv_name[256];	
	strcpy(serv_name, "MyTest");

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	/*
	MPI_Info info;
	MPI_Info_create(&info);
	MPI_Info_set(info, "ompi_global_scope", "true");
	*/

	int merr;
	merr = MPI_Lookup_name(serv_name, MPI_INFO_NULL, port_name_out);
	printf("Looup result is %d \n", merr);
	
	while(1)
	{	
		printf("connecting to the server!\n");	
		merr = MPI_Comm_connect(port_name_out, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm);
		printf("Connect result is %d \n", merr);
		printf("Now disconnect it!!!\n");
		merr = MPI_Comm_disconnect(&newcomm);
		printf("Disconnect result is %d\n", merr);
		sleep(10);
	}
	

	MPI_Finalize();
}
