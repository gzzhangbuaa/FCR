#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef struct
{
	MPI_Comm newcomm;
	int ccount;
	int rank;
	char port_name_out[MPI_MAX_PORT_NAME];
}ARGS;

void* recv_thread(void *ptr)
{
	ARGS* args;
	int rank, size, newrk, msgsz;
	char rb[1024], *rp;
	char outstr[1024];
	int* rtn;
	
	args = (ARGS*)ptr;
	MPI_Status stat, status;
	
	fprintf(stdout, "process:%d, accept a connection on: <%s> for client#%d\n", args->rank, args->port_name_out, args->ccount);

	MPI_Comm_rank(args->newcomm, &newrk);
	sprintf(outstr, "Grank:%d, Lrank%d, receving from client#%d", args->rank, newrk, args->ccount);
	fprintf(stdout, "%s\n", outstr);

	MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, args->newcomm, &stat);
	MPI_Get_count(&stat, MPI_CHAR, &msgsz);

	MPI_Recv(rb, msgsz, MPI_CHAR, stat.MPI_SOURCE, stat.MPI_TAG, args->newcomm, &status);
	sprintf(outstr, "Grank:%d, Lrank%d, received from client#%d<--%s", args->rank, newrk, args->ccount, rb);
	fprintf(stdout, "%s\n", outstr);

	MPI_Barrier(MPI_COMM_WORLD);
	fprintf(stdout, "server barrier with client#%d after receiving passed\n", args->ccount);
	MPI_Comm_disconnect(&(args->newcomm));
	rtn = (int *)malloc(sizeof(int));

	rp = rb;

	while(*rp != '<' && *rp != '\0')
	{
		rp++;
	}	
	if(*rp == '\0')
	{
		goto errrtn;
	}
	rp++;
	*rtn = 0;
	while(*rp != '>' && *rp != '\0')
	{
		if(*rp < '0' || *rp > '9')
		{
			goto errrtn;
		}
		*rtn = (*rtn) * 10 + *rp++ - '0';
	}
	if(*rp == '\0')
	{
		goto errrtn;
	}
	else
	{
		goto normrtn;
	}

	errrtn:
		fprintf(stderr, "Wrong control code, check the client command line\n");
		fflush(stderr);
		*rtn = 1;
	normrtn:
		free(ptr);
		return ((void*)rtn);
}


int main(int argc, char* argv[])
{
	int errs = 0;
	char port_name[MPI_MAX_PORT_NAME], port_name_out[MPI_MAX_PORT_NAME];
	char serv_name[256];
	int merr, mclass;
	char errmsg[MPI_MAX_ERROR_STRING];
	char outstr[1024];
	char sb[1024];
	int msglen, ccount, provided;
	MPI_Status stat;
	int rank, size, newrk, flag, *rtnp, rtn;
	MPI_Comm newcomm;
	ARGS* args;
	pthread_t thread;
	
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if(size != 1)
	{
		fprintf(stderr, "this MPI server program only run with 1 process\n");
		fflush(stderr);
		return 0;
	}
	
	strcpy(port_name, "cn116-ib:8888");
	strcpy(serv_name, "MyTest");

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
	flag = 123;
	newrk = 0;
	sprintf(sb, "Grank:%d, Lrank:%d, require", rank, newrk);
	msglen = strlen(sb);
	if(rank == 0)
	{
		fprintf(stdout, "process:%d, opening port...\n", rank);
		merr = MPI_Open_port(MPI_INFO_NULL, port_name_out);
		fprintf(stdout, "process:%d, port opened, at < %s > \n", rank, port_name_out);
		merr = MPI_Publish_name(serv_name, MPI_INFO_NULL, port_name_out);
		if(merr)
		{
			errs++;
			MPI_Error_string(merr, errmsg, &msglen);
			printf("Error in Publish_name: \"%s\"\n", errmsg);
			fflush(stdout);
		}
		MPI_Barrier(MPI_COMM_WORLD);

		ccount = 0;
		do
		{
			fprintf(stdout, "process:%d, accepting connection on : <%s>\n", rank, port_name_out);
			merr = MPI_Comm_accept(port_name_out, MPI_INFO_NULL, 0, MPI_COMM_SELF, &newcomm);
			ccount++;
			args = (ARGS*)malloc(sizeof(ARGS));
			args->newcomm = newcomm;
			args->ccount = ccount;
			args->rank = rank;
			sprintf(args->port_name_out, "%s", port_name_out);

			pthread_create(&thread, NULL, recv_thread, args);
			pthread_join(thread, (void**)&rtnp);
			rtn = *rtnp;
			fprintf(stdout, "server after disconnect, client#%d, rtn=%d\n", ccount, rtn);
			free(rtnp);
		}while(rtn);
		
		merr = MPI_Unpublish_name(serv_name, MPI_INFO_NULL, port_name_out);
		fprintf(stdout, "server after unpublish name \n");
		if(merr)
		{
			errs++;
			MPI_Error_string(merr, errmsg, &msglen);
			printf("Error in Unpublish name: \"%s\"\n", errmsg);
			fflush(stdout);
		}
	}
	MPI_Finalize();
	return 0;
}





