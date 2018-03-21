#include "mpi.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	MPI_Comm intercomm, newcomm;	
	int rank, size, errs = 0;
	char port_name_out[MPI_MAX_PORT_NAME];
	char serv_name[256];	
	strcpy(serv_name, "MyTest");

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
			
	MPI_Open_port(MPI_INFO_NULL, port_name_out);	
	int merr = MPI_Publish_name(serv_name, MPI_INFO_NULL, port_name_out);
	printf("Publish result is %d\n", merr);

	MPI_Comm_spawn("./client", MPI_ARGV_NULL, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomm, &errs);
	printf("accepting!\n");
	merr = MPI_Comm_accept(port_name_out, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm);

	printf("accepted! accept result is %d\n", merr);
 	
	merr = MPI_Comm_disconnect(&newcomm);
	printf("server disconnect ! result is %d\n", merr);
	//sleep(10);	
	
	
	merr = MPI_Unpublish_name(serv_name, MPI_INFO_NULL, port_name_out);
	
	printf("Unpublished! result is %d\n", merr);
	
	while(1){}
	MPI_Finalize();
}
