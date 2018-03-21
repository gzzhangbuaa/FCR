#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "is_seqnum.h"
#define BACKLOG 50

int main(int argc, char* argv[])
{
	uint32_t seqNum;
	char reqLenStr[INT_LEN];
	char seqNumStr[INT_LEN];
	struct sockaddr_storage claddr;
	int lfd, cfd, optval, reqLen;
	socklen_t addrlen;
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)
	char addrStr[ADDRSTRLEN];
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];
	if(argc > 1 && strcmp(argv[1], "--help") == 0)
	{
		printf("check --help\n");
	}
	
	// seqNum = (argc > 1) ? getInt(argv[1], 0, "init-seq-num") : 0;
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		printf("errExit signal\n");
		exit(-1);
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	
	//if(getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)

	if(getaddrinfo("mpi1", PORT_NUM, &hints, &result) != 0)
	{
		printf("errExit, getaddrinfo \n");
		exit(-1);
	}
	
	printf("getaddrinfo done! \n");
	
	optval = 1;
	int count = 0;
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{	count++;
		printf("check result of getaddrinfo count is %d\n", count);
		lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(lfd == -1)
		{
			continue;
		}
		if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		{
			printf("errExit, setsockopt");
			exit(-1);
		}
		printf("setsockopt done! \n");
	
		if(bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		printf("bind done! \n");
		
		close(lfd);
	}
	
	if(rp == NULL)
	{
		printf("Could not bind socket to any address \n");
		exit(-1);
	}
	if(listen(lfd, BACKLOG) == -1)
	{
		printf("errExit listen\n");
		exit(-1);
	}

	printf("listen done! \n");

	freeaddrinfo(result);

	for(;;)
	{
		addrlen = sizeof(struct sockaddr_storage);
		cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
		if(cfd == -1)
		{
			printf("accept error \n");
			continue;
		}
		printf("accept done! \n");
		if(getnameinfo((struct sockaddr *) &claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
		{
			snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
		}
		else
		{
			snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
		}
		printf("Connection from %s \n", addrStr);

		if(close(cfd) == -1)
		{
			printf("errMsg exit \n");
			exit(-1);
		}
	}
}
