#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv)
{
	int errs = 0;
	char port_name[MPI_MAX_PORT_NAME], port_name_out[MPI_MAX_PORT_NAME];
	char serv_name[256];
	int merr, mclass;
	char errmsg[MPI_MAX_ERROR_STRING];
	char outstr[1024];
	char sb[1024];
	char rb[1024];
	int msglen;
	MPI_Status stat;
	int rank, newrk, flag;
	MPI_Comm newcomm;

	if(argc != 2)
	{
		fprintf(stdout, "Usage: %s control_code, [server_name] control_code=0 means exit \n", argv[0]);
		fflush(stdout);
		return 0;
	}
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	flag = 123;
	if(argc == 3)
	{
		strcpy(serv_name, argv[2]);
	}
	else
	{
		strcpy(serv_name, "MyTest");
	}

	MPI_Barrier(MPI_COMM_WORLD);

	printf("runned here! 1 serv_name is %s\n", serv_name);
	merr= MPI_Lookup_name(serv_name, MPI_INFO_NULL, port_name_out);
	
	printf("runned here! 2 \n");	
	sprintf(outstr, "Grank:%d, trying connecting to server at:::::%s", rank, port_name_out);
	fprintf(stdout, "%s\n", outstr);
	MPI_Comm_connect(port_name_out, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm);
	MPI_Comm_rank(newcomm, &newrk);

	sprintf(outstr, "Grank:%d, Lrank:%d, connected to server", rank, newrk);
	fprintf(stdout, "%s\n", outstr);

	sprintf(outstr, "Grank:%d, Lrank:%d, requesting service....<%s>", rank, newrk, argv[1]);
	fprintf(stdout, "%s\n", outstr);

	sprintf(sb, "Grank:%d, Lrank:%d, require<%s>", rank, newrk, argv[1]);
	msglen = strlen(sb);
	MPI_Send(sb, msglen, MPI_CHAR, 0, flag, newcomm);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Comm_disconnect(&newcomm);
	fprintf(stdout, "client disconnected..........existing....\n");
	MPI_Finalize();
	return 0;
}
