#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "is_seqnum.h"

int main(int argc, char* argv[])
{
	char* reqLenStr;
	char seqNumStr[INT_LEN];
	int cfd;
	ssize_t numRead;
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;

	if(getaddrinfo(argv[1], PORT_NUM, &hints, &result) != 0)
	{
		printf("errExit getaddrinfo \n");
		exit(-1);
	}
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{
		cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(cfd == -1)
		{
			continue;
		}
		if(connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)
		{
			break;
		}
		close(cfd);
	}
	
	if(rp == NULL)
	{
		printf("fatal, Could not connect socket to any address\n");
	}
	freeaddrinfo(result);
	exit(EXIT_SUCCESS);
}
