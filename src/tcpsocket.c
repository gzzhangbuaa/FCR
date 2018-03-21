#define _BSD_SOURCE

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include "chinterface.h"
#include "tcpsocket.h"
#define BACKLOG 50

static pthread_mutex_t lock;


void serverTCP()
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
	
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		printf("[serverTCP-001]: errExit signal\n");
		exit(-1);
	}
	
	printf("[serverTCP-001]: set singnal ignoral done!\n");
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	
	if(getaddrinfo(PDI_Daem.hostName, PORT_NUM, &hints, &result) != 0)
	{
		printf("[serverTCP-002]: errExit, getaddrinfo \n");
		exit(-1);
	}
	
	printf("[serverTCP-002]: getaddrinfo done! \n");
	
	optval = 1;
	int count = 0;
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{	count++;
		printf("check result of getaddrinfo count is %d\n", count);
		lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(lfd == -1)
		{
			printf("[serverTCP-003]: socket failed!, next iteration!\n");
			continue;
		}
		printf("[serverTCP-003]: socket done! \n");

		if(setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		{
			printf("[serverTCP-004]: errExit, setsockopt \n");
			exit(-1);
		}
		printf("[serverTCP-004]: setsockopt done! \n");
	
		if(bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			printf("[serverTCP-005]: bind done! and break loop! \n");
			break;
		}
		printf("[serverTCP-005]: bind failed!\n");
		
		close(lfd);
	}
	
	if(rp == NULL)
	{
		printf("[serverTCP-006]: errExit! Could not bind socket to any address \n");
		exit(-1);
	}
	if(listen(lfd, BACKLOG) == -1)
	{
		printf("[serverTCP-007]: errExit! listen failed!\n");
		exit(-1);
	}

	printf("[serverTCP-007]: listen done!\n");

	freeaddrinfo(result);

	for(;;)
	{
		addrlen = sizeof(struct sockaddr_storage);
		cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
		if(cfd == -1)
		{
			printf("[serverTCP-008]: accept error! continue to accept the next connect request! \n");
			continue;
		}
		printf("[serverTCP-008]: accept one coonection done! \n");
		if(getnameinfo((struct sockaddr *) &claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
		{
			snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
		}
		else
		{
			snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
		}
		printf("Connection from %s \n", addrStr);
		
		/* just for test
		int len = 0;
		char* rb = talloc(char, 30);
		len = recv(cfd, rb, 30, 0);
		printf("[@@@@ Received: %s, len is %d @@@@]\n", rb, len);
		free(rb);
		*/
		
		int len = sizeof(PDIT_recvguider);
		PDIT_recvguider* PDI_RecvGuider = talloc(PDIT_recvguider, 1);
		len = recv(cfd, PDI_RecvGuider, len, 0);
		printf("Recv len is %d, msgType is %d, msgSize is %d\n", len, PDI_RecvGuider->msgType, PDI_RecvGuider->msgSize);
		void* msg = NULL;
		if(PDI_RecvGuider->msgSize > 0)
		{
			msg = talloc(char, PDI_RecvGuider->msgSize);
			if(msg == NULL)
			{
				printf("[DEBUG_INFO]:######: msg is NULL\n");
			}
			else
			{
				printf("[DEBUG_INFO]:######: msg is not NULL\n");
			}
			memset(msg, 0, PDI_RecvGuider->msgSize);
			len = recv(cfd, msg, PDI_RecvGuider->msgSize, MSG_WAITALL);
		}
		
		//response a simple answer if coming type is simple request
		if(PDI_RecvGuider->msgType == PDI_TYPE_REQUEST)
		{
			int sb = PDI_ALIVE;
			send(cfd, &sb, sizeof(sb), 0);
		}
	
		//specially when type is status response from some one
		if(PDI_RecvGuider->msgType == PDI_TYPE_DETECT)
		{
			printf("[DETECTING!]:collect local node status and send back to leader!\n");
			PDIT_nodestatus* PDI_NodeStatus = talloc(PDIT_nodestatus, 1);
			len = sizeof(PDIT_nodestatus);
			PDI_NodeStatus->nodeID = PDI_Daem.nodeID;
			PDI_NodeStatus->groupID = PDI_Daem.groupID;
			PDI_NodeStatus->posInGroup = PDI_Daem.posInGroup;
			PDI_NodeStatus->status = PDI_ALIVE;
			strcpy(PDI_NodeStatus->hostName, PDI_Daem.hostName);
			len = send(cfd, PDI_NodeStatus, len, 0);
			printf("send back msg size is %d\n", len);
			free(PDI_NodeStatus);
		}
		
		//log the msg into a file when group leader give back the distributed node status
		if(PDI_RecvGuider->msgType == PDI_TYPE_GLRESP && PDI_Daem.amICenter == PDI_CENTERHOST)
		{
			/*
			printf("Record these distributed node status into a file!\n");
			void* p = malloc(PDI_RecvGuider->msgSize);
			len = recv(cfd, p, PDI_RecvGuider->msgSize, 0);
			printf("[Center]: GLRESP recv msg-1 size is %d\n", len);
			free(p);
			*/			

			//get how many node status should receive
			int rb[2];
			len = sizeof(rb);
			len = recv(cfd, rb, len, 0);
			printf("[Center]: GLRESP recv msg-2 size is %d\n", len);
			int groupID = rb[0];
			int nodeCount = rb[1];
			len = sizeof(PDIT_nodestatus) * nodeCount;
			PDI_NSList = talloc(PDIT_nodestatus, nodeCount);
			len = recv(cfd, PDI_NSList, len, 0);
			printf("[Center]: GLRESP recv msg size is %d\n", len);

			//open a file descriptor for write 
			FILE* stream;
			if((stream = fopen(PDI_NODESTATUS, "a+")) == NULL)
			{
				printf("[PDI-ERROR]: file operation error occured! abnormal exit!\n");
				exit(-1);
			}
			//write a record head
			int size = PDI_BUFS * 4;
			char* buf = talloc(char, size);	
			snprintf(buf, size, "<There are %d nodes in group %d>\n flag: %d represent PDI_ALIVE, %d represent PDI_DEAD\nstatus detail is below:\n", 
					nodeCount, groupID, PDI_ALIVE, PDI_DEAD);	
			len = strlen(buf);	
			fwrite(buf, len, 1, stream);


			if(PDI_NSList != NULL)
			{
				int i = 0;
				for(i = 0; i < nodeCount; i++)
				{
					printf("<nodeID : %d> <groupID : %d> <posInGroup : %d> <hostName : %s> <status : %d>\n", 
						PDI_NSList[i].nodeID, PDI_NSList[i].groupID, PDI_NSList[i].posInGroup,
						PDI_NSList[i].hostName, PDI_NSList[i].status);
					snprintf(buf, size, "<groupID : %d> <nodeID : %d> <posInGroup : %d> <hostName : %s> <status : %d>\n", 
						PDI_NSList[i].groupID, PDI_NSList[i].nodeID, PDI_NSList[i].posInGroup,
						PDI_NSList[i].hostName, PDI_NSList[i].status);
					len = strlen(buf);	
					fwrite(buf, len, 1, stream);	
				}				
			}
			
			fflush(stream);
			fclose(stream);
			if(buf != NULL)
			{
				free(buf);
			}			

			if(PDI_GroupReport != NULL)
			{
				PDI_GroupReport[groupID] = PDI_GOTGR;
			}
			if(PDI_NSList != NULL)
			{
				free(PDI_NSList);
				PDI_NSList = NULL;
			}
		}

		if(close(cfd) == -1)
		{
			printf("[serverTCP-009]: errExit, close current socket failed! \n");
			exit(-1);
		}
		printf("[serverTCP-009]: close current socket done!\n");
		if((PDI_RecvGuider->msgType == PDI_TYPE_ABREPT || PDI_RecvGuider->msgType == PDI_TYPE_JUSTLOG) && PDI_Daem.amICenter == PDI_CENTERHOST)
		{
			//write the suspection event information into a log file
			if(PDI_RecvGuider->msgSize > 0 && msg != NULL)
			{
				PDIT_execstatus* PDI_ExecStatus = (PDIT_execstatus*)msg;
				int size = PDI_BUFS * 4;
				char* nbuf = talloc(char, size);
				//get basic info
				memset(nbuf, 0, size);
				printf("[##### Inside serverTCP #####]size = %d,hostName = %s, str = %s\n", 
					strlen(PDI_ExecStatus->description), PDI_ExecStatus->nodeStatus.hostName, PDI_ExecStatus->description);
				
				//log the suspicious event source	
				if(PDI_ExecStatus->description != NULL)
				{
					printf("{ ******* Before logger *******}\n");
					logger(PDI_SUSPECTEVENT, PDI_ExecStatus->description, PDI_BUFS * 4);
					printf("{ ******* After  logger *******}\n");
				}
				//log the detail of the event
				if(PDI_ExecStatus->nodeStatus.status == PDI_ALIVE)
				{
					sprintf(nbuf, "[Suspicous Event source]:\n"
							"<groupID : %d> <nodeID : %d> <posInGroup : %d> <hostName : %s>"
							"<nodeStatus : %s>\n", PDI_ExecStatus->nodeStatus.groupID, PDI_ExecStatus->nodeStatus.nodeID,
							PDI_ExecStatus->nodeStatus.posInGroup, PDI_ExecStatus->nodeStatus.hostName,
							"ALIVE");
				}
				else if(PDI_ExecStatus->nodeStatus.status == PDI_DEAD)
				{
                               	        sprintf(nbuf, "[Suspicous Event source]:\n"
                                       	                "<groupID : %d> <nodeID : %d> <posInGroup : %d> <hostName : %s>"
                                               	        "<nodeStatus : %s>\n", PDI_ExecStatus->nodeStatus.groupID, PDI_ExecStatus->nodeStatus.nodeID,
                                       	                PDI_ExecStatus->nodeStatus.posInGroup, PDI_ExecStatus->nodeStatus.hostName,
                                               	        "DEAD");
				}
				else
				{
                                        sprintf(nbuf, "[Suspicous Event source]:\n"
                       	                                "<groupID : %d> <nodeID : %d> <posInGroup : %d> <hostName : %s>"
                               	                        "<nodeStatus : %s>\n", PDI_ExecStatus->nodeStatus.groupID, PDI_ExecStatus->nodeStatus.nodeID,
                                       	                PDI_ExecStatus->nodeStatus.posInGroup, PDI_ExecStatus->nodeStatus.hostName,
                                               	        "UNIDENTIFIED");
				}

				//get detail proc status info
				size = PDI_BUFS * PDI_ExecStatus->nbProcs;
				char* pbuf = talloc(char, size);
				memset(pbuf, 0, size);
				sprintf(pbuf, "There are %d MPI work processes in this node:\n", PDI_ExecStatus->nbProcs);
				int i;
				for(i = 0; i < PDI_ExecStatus->nbProcs; i++)
				{
					char pdetail[PDI_BUFS];
					if(PDI_ExecStatus->pmpStatus[i] == PDI_ALIVE)
					{
						sprintf(pdetail, "<pid : %d> <proc-status : %s>\n", PDI_ExecStatus->processId[i],
							"ALIVE : msg passing activity normally!");
					}
					else if(PDI_ExecStatus->pmpStatus[i] == PDI_DEAD)
					{
						if(PDI_ExecStatus->peStatus[i] == PDI_EXIST)
						{
							sprintf(pdetail, "<pid : %d> <proc-status : %s>\n", PDI_ExecStatus->processId[i],
								"Abnormal but still exists! : msg passing activity abnormally!");
						}
						else if(PDI_ExecStatus->peStatus[i] == PDI_ABORT)
						{
							sprintf(pdetail, "<pid : %d> <proc-status : %s>\n", PDI_ExecStatus->processId[i],
								"Exited already! : abort abnormally already!");
						}
						else
						{
							sprintf(pdetail, "<pid : %d> <proc-status : %s>\n", PDI_ExecStatus->processId[i],
								"Abnormal but UNIDENTIFIED");
						}
					}
					else
					{
						sprintf(pdetail, "<pid : %d> <proc-status : %s>\n", PDI_ExecStatus->processId[i],
							"UNIDENTIFIED");
					}
					strcat(pbuf, pdetail);
				}
				//nbuf used for node status info, pbuf used for process status info, tbuf used for total status info
				size = strlen(nbuf) + strlen(pbuf);
				char* tbuf = talloc(char, size);
				memset(tbuf, 0, size);
				strcat(tbuf, nbuf);
				strcat(tbuf, pbuf);
				size = strlen(tbuf);

				printf("[###### before logger : size is %d]\n%s\n", size, tbuf);
				logger(PDI_SUSPECTEVENT, tbuf, size);
				if(nbuf != NULL)
				{
					free(nbuf);
				}
				if(pbuf != NULL)
				{
					free(pbuf);
				}
				if(tbuf != NULL)
				{
					free(tbuf);
				}
			}
			if(PDI_RecvGuider->msgType == PDI_TYPE_ABREPT)
			{
				if(PDI_Daem.isChecking == PDI_Checking)
				{
					printf("[serverTCP-010]: ABREPT-Checking is online, just log the suspect event!\n");
					
				}
				else
				{
					printf("PDI_Daem.isChecking is %d, create a thread!!!!!!!!!!!!!!!!!!!!!\n", PDI_Daem.isChecking);
					//detective();
					PDI_Daem.isChecking = PDI_Checking;
					
					time_t check_start;
					time(&check_start);
					printf("[DEBUG_INFO]: status checking start time:%s @@@@@####\n", ctime(&check_start));

					pthread_t pdetect;
					printf("[DEBUG-INFO]: create a thread for detective!\n");
					pthread_create(&pdetect, NULL, detective, NULL);
					printf("[DEBUG-INFO]: after the detective thread!\n");
				}
			}
			else if(PDI_RecvGuider->msgType == PDI_TYPE_JUSTLOG)
			{	
				printf("[#### Event Trigger #####]: just record the error msg in the suspect log!\n");
			}
			else
			{
				printf("[DEBUG_INFO] : Error of PDI_TYPE_XXX, check it!\n");
			}			
		}
		if(PDI_RecvGuider->msgType == PDI_TYPE_SPECIFY)
		{
			printf("Detect in group !\n");
			PDIT_nodestatus* PDI_NSList = talloc(PDIT_nodestatus, PDI_Daem.realGSize);
			detectInGroup();
			free(PDI_NSList);
		}
		if(PDI_RecvGuider->msgType == PDI_TYPE_FTEST)
		{
			printf("[DEBUG_INFO]: serverTCP thread received stop command!\n");
        		time_t currentTime;
        		time(&currentTime);
        		printf("[DEBUG_INFO] failure test-serverTCP stop current time: %s", ctime(&currentTime));
			while(1)
			{
				sleep(100);
			}
		}
		if(msg != NULL)
		{
			free(msg);
		}
		free(PDI_RecvGuider);
	}
}

int clientTCP(char* hostName, int msgType, int msgSize, char* msg)
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

        if(getaddrinfo(hostName, PORT_NUM, &hints, &result) != 0)
        {
                printf("[clientTCP-001]: errExit! getaddrinfo \n");
                return PDI_NSCS;
        }
	printf("[clientTCP-001]: getaddrinfo done! \n");
        for(rp = result; rp != NULL; rp = rp->ai_next)
        {
                cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                if(cfd == -1)
                {
			printf("[clientTCP-002]: socket failed! continue to try next socket!\n");
                        continue;
                }
		printf("[clientTCP-002]: socket done! \n");
                if(connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)
                {
			printf("[clientTCP-003]: connect done! break the loop.\n");
                        break;
                }
		printf("[clientTCP-003]: connect failed!\n");
                close(cfd);
		printf("[clientTCP-004]: close bad socket descriptor!\n");
        }

        if(rp == NULL)
        {
                printf("[clientTCP-005]: fatal, Could not connect socket to any address\n");
		return PDI_NSCS;
        }
	
	printf("[clientTCP-004]: this area send and receive messages\n");

        /** Set timeout for send and recv operations **/
        struct timeval timeout = {60, 0};
        int ret = setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
	if(ret != 0)
	{
        	printf("[DEBUG_INFO]:   *****ret = %d,error in setsockopt of clientTCP for setting send timeout! \n", ret);
	}
        ret = setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	if(ret != 0)
	{
        	printf("[DEBUG_INFO]:   *****ret = %d,error in setsockopt of clientTCP for setting recv timeout! \n", ret);
	}
	
	/* just for test
	int len = 0;
	char* sb = talloc(char, 30);
	strcpy(sb, "Welcome to my server\n");
	len = send(cfd, sb, 30, 0);
	printf("[@@@@ send result is %d]\n", len);
	free(sb);
	*/
	//send message type first
	int len = sizeof(PDIT_recvguider);
	printf("the sizeof(PDIT_recvguider) is %d\n", len);
	PDIT_recvguider* PDI_RecvGuider = talloc(PDIT_recvguider, 1);

	//just for failure test
	if(msgType == PDI_TYPE_FTEST)
	{
		PDI_RecvGuider->msgType = msgType;
                PDI_RecvGuider->msgSize = msgSize;
                len = send(cfd, PDI_RecvGuider, len, 0);
		printf("[DEBUG_INFO]: failure test phase, tell serverTCP thread to stop! send result is %d\n", len);
		if(len <= 0)
		{
			return PDI_NSCS;
		}
		else
		{
			return PDI_SCES;
		}
	}

	//send a simle test request to dest node
	if(msgType == PDI_TYPE_REQUEST)	
	{
		PDI_RecvGuider->msgType = msgType;
		PDI_RecvGuider->msgSize = msgSize;
		len = send(cfd, PDI_RecvGuider, len, 0);
		if(msgSize != 0 && msg != NULL)
		{
			len = send(cfd, msg, msgSize, 0);
			printf("send msg-0 count is %d\n", len);
	                if(len <= 0)
        	        {
                	        return PDI_NSCS;
               		}

		}
		
		int rb;
		len = recv(cfd, &rb, sizeof(rb), 0);
		if(len <= 0)
		{
			return PDI_NSCS;
		}
		
	}

	//send suspect report to control centre
	if(msgType == PDI_TYPE_ABREPT || msgType == PDI_TYPE_JUSTLOG)
	{
		PDIT_execstatus* PDI_ExecStatus = talloc(PDIT_execstatus, 1);	
		if(PDI_ExecStatus == NULL)
		{
			printf("[DEBUG_INFO]: Error of talloc in TCP client\n");
			exit(-1);
		}	
		if(PDI_GetProcStatus(PDI_ExecStatus, msg) == PDI_NSCS)
		{
			printf("[DEBUG_INFO]: Error in PDI_GetProcStatus\n");
			exit(-1);
		}
		
		PDI_RecvGuider->msgType = msgType;
		//PDI_RecvGuider->msgType = PDI_TYPE_ABREPT;
		PDI_RecvGuider->msgSize = sizeof(PDIT_execstatus);
		len = send(cfd, PDI_RecvGuider, len, 0);
		printf("send msg-1 count is %d\n", len);
		//send concrete message
		if(PDI_RecvGuider->msgSize != 0 && PDI_ExecStatus != NULL)
		{
			len = send(cfd, PDI_ExecStatus, PDI_RecvGuider->msgSize, MSG_WAITALL);	
			printf("send msg-2 count is %d\n", len);
			printf("[#### Inside Client ####]:PDI_ExecStatus->description:%s", PDI_ExecStatus->description);
		}
		free(PDI_ExecStatus);
	}
	
	//send specify command to group leader
	if(msgType == PDI_TYPE_SPECIFY)
	{
		PDI_RecvGuider->msgType = msgType;
		PDI_RecvGuider->msgSize = msgSize;
		len = send(cfd, PDI_RecvGuider, len, 0);
		printf("send msg-1 count is %d\n", len);
		if(msgSize != 0 && msg != NULL)
		{
			len = send(cfd, msg, msgSize, 0);
			printf("send msg-2 count is %d\n", len);
		}
	}
	
	if(msgType == PDI_TYPE_DETECT)
	{
		PDI_RecvGuider->msgType = msgType;
		PDI_RecvGuider->msgSize = msgSize;
		len = send(cfd, PDI_RecvGuider, len, 0);
		printf("send msg-1 count is %d\n", len);
		if(msgSize != 0 && msg != NULL)
		{
			len = send(cfd, msg, msgSize, 0);
			printf("send msg-2 count is %d\n", len);
		}
                if(len <= 0)
                {
                        printf("[DEBUG_INFO]:***** clientTCP send error with PDI_TYPE_DETECT! %d\n", len);
                        return PDI_NSCS;
                }

		//receive the response from group member
		printf("receive the response about node status!\n");
		len = sizeof(PDIT_nodestatus);
		printf("sizeof(PDIT_nodestatus) is %d\n", len);
		PDIT_nodestatus* PDI_NodeStatus = talloc(PDIT_nodestatus, 1);			
		len = recv(cfd, PDI_NodeStatus, len, 0);

                if(len <= 0)
                {
                        printf("[DEBUG_INFO]:***** clientTCP recv error with PDI_TYPE_DETECT! %d\n", len);
                        return PDI_NSCS;
                }


		printf("Recv len is %d, <nodeID : %d> <groupID : %d> <hostName : %s> <posInGroup : %d> <status : %d>\n", 
			len, PDI_NodeStatus->nodeID, PDI_NodeStatus->groupID, PDI_NodeStatus->hostName, 
			PDI_NodeStatus->posInGroup, PDI_NodeStatus->status);

		
		if(PDI_NSList != NULL)
		{
			printf("[******************************** OK ???? *********************************]\n");
			len = sizeof(PDIT_nodestatus);
			printf("[******PDIT_nodestatus******]: len is %d, posInGroup is %d\n", len, PDI_NodeStatus->posInGroup);
			memcpy(PDI_NSList + (PDI_NodeStatus->posInGroup), PDI_NodeStatus, len);
			printf("[******************************** OK !!!! *********************************]\n");
		}
		
		free(PDI_NodeStatus);
	}
	if(msgType == PDI_TYPE_GLRESP)
	{
		printf("Return all node status in this group to control center!\n");
		PDI_RecvGuider->msgType = msgType;
		PDI_RecvGuider->msgSize = msgSize;
		len = send(cfd, PDI_RecvGuider, len, 0);
		printf("send msg-1 count is %d\n", len);
		if(msgSize != 0 && msg != NULL)
		{
			len = send(cfd, msg, msgSize, 0);
			printf("[GLRESP]: send msg-2 count is %d\n", len);
		}
		
		//send PDIT_nodestatus object counts
		int sb[2];
		sb[0] = PDI_Daem.groupID;
		sb[1] = PDI_Daem.realGSize;
		len = sizeof(sb);
		len = send(cfd, sb, len, 0);
		printf("[GLRESP]: send msg-3 count is %d\n", len);
		//send the list of node status- PDI_NSList
		len = sizeof(PDIT_nodestatus) * PDI_Daem.realGSize;
		len = send(cfd, PDI_NSList, len, 0);
		printf("[GLRESP]: send msg-4 count is %d\n", len);
	}
	
	
	free(PDI_RecvGuider);

        freeaddrinfo(result);
        close(cfd);
	printf("[clientTCP-005]: close the socket descriptor!\n");
	return PDI_SCES;
}


/*----------------------------------------------------------------------------------------------*/
/**
                @brief  update the function detectLauncher_upd()
			add Record to a file
**/
/*----------------------------------------------------------------------------------------------*/
int detectLauncher(char* fileName)
{
	FILE* stream;
	if((stream = fopen(fileName, "a+")) == NULL)
	{
		return PDI_NSCS;
	}

        char* buf = talloc(char, PDI_BUFS * 4);         //1024 to store

	strcpy(buf, "<The process of specifying the group leaders>\n");
	int len = strlen(buf);
	fwrite(buf, len, 1, stream);


        int i, leaderID, flag;
        if(PDI_Daem.leaders == NULL)
        {
		memset(buf, 0, PDI_BUFS * 4);
		strcpy(buf, "The leaders of this MPI program is NULL, please do some check!\n");
		len = strlen(buf);
		fwrite(buf, len, 1, stream);
		fflush(stream);
		fclose(stream);
		PDI_Daem.isChecking = PDI_UnChecking;
		free(buf);
                return PDI_NSCS;
        }

	//incase PDI_GroupReport is hold by last procedure
	if(PDI_GroupReport != NULL)
	{
		free(PDI_GroupReport);
		PDI_GroupReport = NULL;
	}
	//rebuild the PDI_GroupReport space
	PDI_GroupReport = talloc(int, PDI_Daem.groupNums);
	
	for(i = 0; i < PDI_Daem.groupNums; i++)
	{
		PDI_GroupReport[i] = PDI_NGOTGR;
		printf("[@@@@@@@]-PDI_GroupReport[%d]: %d\n", i, PDI_GroupReport[i]);
	}

        char* destHost = talloc(char, PDI_BUFS);
        for(i = 0; i < PDI_Daem.groupNums; i++)
        {
                flag = 0;
                leaderID = PDI_Daem.leaders[i];
                while(!flag)
                {
                        if(leaderID >= (i+1) * PDI_Daem.groupSize || leaderID >= PDI_Daem.nbNodes)
                        {
                                printf("[WARNING]: All nodes in this group had been tested!, all dead! \n");
				memset(buf, 0, PDI_BUFS * 4);
				strcpy(buf, "[WARNING]: All nodes in this group had been tested!, all dead!\n");
				len = strlen(buf);
				fwrite(buf, len, 1, stream);
                                flag++;
                        }
                        else
                        {
                                memset(destHost, 0, PDI_BUFS);
                                strncpy(destHost, PDI_Daem.nameList + leaderID * PDI_BUFS, PDI_BUFS);
                                int result = clientTCP(destHost, PDI_TYPE_SPECIFY, 0, NULL);
				//printf("[WARNING @@@@@@] : result is %d \n", result);
                                if(result == PDI_SCES)
                                {
                                        printf("<Group %d><node %d><hostName %s> as leader is alive!\n", i, leaderID, destHost);
					memset(buf, 0, PDI_BUFS * 4);	
					sprintf(buf, "<Group %d><node %d><hostName %s> as leader is alive!\n", i, leaderID, destHost);
					len = strlen(buf);	
					fwrite(buf, len, 1, stream);			

                                        PDI_Daem.leaders[i] = leaderID;
                                        flag++;
                                }
                                else
                                {
                                        printf("<Group %d><node %d><hostName %s> is dead! can't as a leader\n", i, leaderID, destHost);
					memset(buf, 0, PDI_BUFS * 4);
					sprintf(buf, "<Group %d><node %d><hostName %s> is dead! can't as a leader\n", i, leaderID, destHost);
					len = strlen(buf);
					fwrite(buf, len, 1, stream);

                                        leaderID++;
                                }
                        }
                }
        }
	fflush(stream);
	fclose(stream);
	free(buf);
        free(destHost);
	
	//this variable is reset by checkFinish function when cc got every group's answer in one turn
	//PDI_Daem.isChecking = PDI_UnChecking;
	
}



void* detective()
{
	
	pthread_detach(pthread_self());

	time_t startT;
	time(&startT);
	printf("[DETECTIVE-START] : %s @@@@####!\n", ctime(&startT));
	
	detectLauncher(PDI_DLAUNCHER);
	int gotCount = 0;
	while(gotCount != PDI_Daem.groupNums)
	{
		checkFinish(&gotCount);
		if(gotCount != PDI_Daem.groupNums)
		{
			sleep(10);
		}
	}
	
	printf("[DETECTIVE] : This turn node status detection was done!\n");
	//detective is done in this turn, reset the status flag about checking
	time_t endT;
	time(&endT);
	printf("[DENUG_INFO] this turn status checking end! current time is : %s @@@@####!\n", ctime(&endT));	

	PDI_Daem.isChecking = PDI_UnChecking;
	//release the memory used to store flags of group leader response
	free(PDI_GroupReport);
	PDI_GroupReport = NULL;

	pthread_exit(0);
}






/*--------------------------------------------------------------------------------------------------------*/
/**
		@brief		contrl center check wether it have received all the status response
				from all group leaders or not, if not, it should detect the leader
				which haven't give back the response is alive or dead, if dead, it 
				should specify a new leader and re-execute the detect action inside 
				this group. Worest situation is all nodes are dead in one group. sign
				the status of receving of this group PDI_EXTINCT.
		@param		gotCount	Now how many groups status cc have got
**/
/*--------------------------------------------------------------------------------------------------------*/

int checkFinish(int* gotCount)
{
	if(PDI_GroupReport == NULL)
	{
		return PDI_NSCS;
	}
	int i, flag, leaderID;
	char* destHost = talloc(char, PDI_BUFS);
	*gotCount = 0;
	for(i = 0; i < PDI_Daem.groupNums; i++)
	{
		printf("*************PDI_GroupReport[%d] is %d***********\n", i, PDI_GroupReport[i]);

		if(PDI_GroupReport[i] == PDI_GOTGR)
		{
			(*gotCount)++;
			continue;
		}
		else if(PDI_GroupReport[i] == PDI_EXTINCT)
		{
			//logger!
			char buf[PDI_BUFS];
			if(i == 0)
			{
				sprintf(buf, "<groupID : 0> all nodes were dead! except <hostName : %s> as control center!\n",
					PDI_Daem.ccName);
			}
			else
			{
				sprintf(buf, "<groupID : %d> all nodes were dead!\n", i);
			}
			int len = strlen(buf);
			logger(PDI_NODESTATUS, buf, len);

			(*gotCount)++;
			continue;
		}
		else if(PDI_GroupReport[i] == PDI_NGOTGR)
		{	
			flag = 0;
			leaderID = PDI_Daem.leaders[i];
			strncpy(destHost, PDI_Daem.nameList + leaderID * PDI_BUFS, PDI_BUFS);
			int result = clientTCP(destHost, PDI_TYPE_REQUEST, 0, NULL);	

			printf("[*******result is %d***********]\n", result);
			if(result == PDI_SCES)
			{
				printf("[checkFinish] : <groupID : %d> current group leader <nodeID : %d> <hostName : %s> is alive!\n",
					i, leaderID, destHost);
				continue;
			}		
			else if(result == PDI_NSCS)
			{
				printf("[checkFinish] : <groupID : %d> current group leader <nodeID : %d> <hostName : %s> is dead!\n",
					i, leaderID, destHost);
				printf("[checkFinish] : Now specify a new group leader!\n");
				while(!flag)
				{
					leaderID++;
					if(leaderID >= (i+1) * PDI_Daem.groupSize || leaderID >= PDI_Daem.nbNodes)
					{
						printf("[checkFinish] : There is no additional node left can be as a leader in this group!\n");
						flag++;
						PDI_GroupReport[i] = PDI_EXTINCT;
					}
					else
					{
						strncpy(destHost, PDI_Daem.nameList + leaderID * PDI_BUFS, PDI_BUFS);
						result = clientTCP(destHost, PDI_TYPE_SPECIFY, 0, NULL);
						if(result == PDI_SCES)
						{
							printf("[detectFinish] : Specify a new leader! Succeed!\n");
							flag++;
						}
						else
						{
							printf("[detectFinish] : Specify action failed, move to the next!\n");
						}
						PDI_Daem.leaders[i] = leaderID;
					}
					
				}				

			}
		}
		else
		{
			printf("[ERROR!!!!]: No such status exists in group report status!\n");
			return PDI_NSCS;
		}
	}
	return PDI_SCES;
}




/*----------------------------------------------------------------------------------------------*/
/**
		@brief	update the function detectLauncher_org()
**/
/*----------------------------------------------------------------------------------------------*/
int detectLauncher_upd()
{
	int i, leaderID, flag;
	if(PDI_Daem.leaders == NULL)
	{
		return PDI_NSCS;
	}
	char* destHost = talloc(char, PDI_BUFS);
	for(i = 0; i < PDI_Daem.groupNums; i++)
	{
		flag = 0;
		leaderID = PDI_Daem.leaders[i];
		while(!flag)
		{
			if(leaderID >= (i+1) * PDI_Daem.groupSize || leaderID >= PDI_Daem.nbNodes)
			{
				printf("[WARNING]: All nodes in this group had been tested!, all dead! \n");
				flag++;
			}
			else
			{
				memset(destHost, 0, PDI_BUFS);
				strncpy(destHost, PDI_Daem.nameList + leaderID * PDI_BUFS, PDI_BUFS);
				int result = clientTCP(destHost, PDI_TYPE_SPECIFY, 0, NULL);
				if(result == PDI_SCES)
				{
					printf("<Group %d><node %d><hostName %s> as leader is alive!\n", i, leaderID, destHost);
					PDI_Daem.leaders[i] = leaderID;
					flag++;
				}
				else
				{
					printf("<Group %d><node %d><hostName %s> is dead! can't as a leader\n", i, leaderID, destHost);
					leaderID++;
				}
			}
		}
	}
	free(destHost);
}



/*-----------------------------------------------------------------------------------------------*/
/**
		@brief	control center received suspect event alarm and trigger the detective
			try to designated the group leader and record it
			the first function we built
**/
/*-----------------------------------------------------------------------------------------------*/
int detectLauncher_org()
{
	int i, leaderID;
	if(PDI_Daem.leaders == NULL)
	{
		return PDI_NSCS;
	}

	char* destHost = talloc(char, PDI_BUFS);
	
	for(i = 0; i < PDI_Daem.groupNums; i++)
	{
		memset(destHost, 0, PDI_BUFS);
		leaderID = PDI_Daem.leaders[i];
		strncpy(destHost, PDI_Daem.nameList + leaderID * PDI_BUFS, PDI_BUFS);
		printf("destHost is %s \n", destHost);
		int result = clientTCP(destHost, PDI_TYPE_SPECIFY, 0, NULL);
		if(result == PDI_SCES)
		{
			printf("reslut is %d alive! \n", result);
			continue;
		}
		else
		{
			printf("result is %d dead! \n", result);
			int flag = 0;	
			while(!flag)
			{
				leaderID++;
				if(leaderID >= (i+1) * PDI_Daem.groupSize || leaderID >= PDI_Daem.nbNodes)
				{
					printf("[WARNING]: All nodes in this group had been tested!, all dead!\n");
					flag++;
				}
				else
				{
					strncpy(destHost, PDI_Daem.nameList + leaderID * PDI_BUFS, PDI_BUFS);
					printf("destHost is %s \n", destHost);
					result = clientTCP(destHost, PDI_TYPE_SPECIFY, 0, NULL);
					if(result == PDI_SCES)
					{
						printf("<Group %d><node %d> as leader is alive!\n", i, leaderID);
						PDI_Daem.leaders[i] = leaderID;
						flag++;
					}
					else
					{
						printf("<Group %d><node %d> can't as leader, it's already dead!\n", i, leaderID);
					}
				}
			}
		}
	}
	free(destHost);
}

int detectInGroup()
{
	int i, posInGroup;
	PDI_Daem.amIGroupLeader = PDI_GROUPLEADER;
	posInGroup = PDI_Daem.nodeID % PDI_Daem.groupSize;
	char* destHost = talloc(char, PDI_BUFS);

	PDI_NSList = talloc(PDIT_nodestatus, PDI_Daem.realGSize);
	
	if(PDI_NSList == NULL)
	{
		printf("[DIG:] PDI_NSList is NULL\n");
		return PDI_NSCS;
	}	

	for(i = 0; i < PDI_Daem.realGSize; i++)
	{
		memset(destHost, 0, PDI_BUFS);
		strncpy(destHost, PDI_Daem.groupMembers + i * PDI_BUFS, PDI_BUFS);
		printf("the host I willing to detect is %s\n", destHost);

		if(posInGroup == i)
		{
			printf("host: %s, I am the group leader, I will check myself!\n", destHost);
			PDIT_nodestatus* PDI_NodeStatus = talloc(PDIT_nodestatus, 1);	
			PDI_NodeStatus->nodeID = PDI_Daem.nodeID;
			PDI_NodeStatus->groupID = PDI_Daem.groupID;
			PDI_NodeStatus->posInGroup = PDI_Daem.posInGroup;
			PDI_NodeStatus->status = PDI_ALIVE;
			strcpy(PDI_NodeStatus->hostName, PDI_Daem.hostName);	
			memcpy(PDI_NSList + i, PDI_NodeStatus, sizeof(PDIT_nodestatus));	
			free(PDI_NodeStatus);
			continue;
		}

		int result = clientTCP(destHost, PDI_TYPE_DETECT, 0, NULL);
		if(result == PDI_NSCS)
		{
			printf("[DetectInGroup]: host: %s is dead! \n", destHost);
			PDIT_nodestatus* PDI_NodeStatus = talloc(PDIT_nodestatus, 1);
			PDI_NodeStatus->nodeID = PDI_Daem.groupID * PDI_Daem.groupSize + i;
			PDI_NodeStatus->groupID = PDI_Daem.groupID;
			PDI_NodeStatus->posInGroup = i;
			//strcpy(PDI_NodeStatus->hostName, &PDI_Daem.groupMembers[i]);
			strcpy(PDI_NodeStatus->hostName, destHost);
			PDI_NodeStatus->status = PDI_DEAD;
			memcpy(PDI_NSList + i, PDI_NodeStatus, sizeof(PDIT_nodestatus));
			free(PDI_NodeStatus);
		}
		else
		{
			printf("[DetectInGroup]: host: %s is alive! \n", destHost);
		}
	}
	
	//just for test
	
	for(i = 0; i < PDI_Daem.realGSize; i++)
	{
		printf("[DIG showing]: <nodeID : %d> <groupID : %d> <posInGroup : %d> <hostName : %s> <status : %d>\n", 
			PDI_NSList[i].nodeID, PDI_NSList[i].groupID, PDI_NSList[i].posInGroup,
			PDI_NSList[i].hostName, PDI_NSList[i].status);
	}	
		
	free(destHost);

	//send the detect result to control center
	int len = sizeof(PDIT_nodestatus) * PDI_Daem.realGSize;
	printf("[**********before call clientTCP*********, len is %d]\n", len);
	int result = clientTCP(PDI_Daem.ccName, PDI_TYPE_GLRESP, 0, NULL); 
	printf("[**********after call clientTCP**********, result is %d]\n", result);
	
	free(PDI_NSList);
	return PDI_SCES;
}


// just a control flow for understanding
int actionFlow(int msgType)
{
	if(msgType == PDI_TYPE_ABREPT)
	{
		if(PDI_Daem.amICenter != PDI_CENTERHOST)
		{
			return PDI_NSCS;
		}
		if(PDI_Daem.isChecking == PDI_UnChecking)
		{
			PDI_Daem.isChecking = PDI_Checking;
			printf("[Action-Flow]: Control center send command to  group leader to lanch detective!\n");
			detectLauncher("./DLauncher.log");
			PDI_Daem.isChecking = PDI_UnChecking;
			return PDI_SCES;
		}
		else
		{
			printf("[Action-Flow]: Just log the suspect event info into a file \n");	
			return PDI_SCES;		
		}
	}
	else if(msgType == PDI_TYPE_SPECIFY)
	{
		PDI_Daem.amIGroupLeader = PDI_GROUPLEADER;
		printf("[Action-Flow]: Group leader execute detect inside the group! \n");
		detectInGroup();
	}
	else if(msgType == PDI_TYPE_DETECT)	
	{
		printf("[Action-Flow]: Collect all MPI Processes on this node and send to the leader!\n");
	}
	else if(msgType == PDI_TYPE_SRESP)	
	{
		printf("[Action-Flow]: get all members status info & send it to center control!\n");
	}
	else if(msgType == PDI_TYPE_GLRESP)
	{
		printf("[Action-Flow]: Record the status answer!\n");		
	}
	else
	{
		printf("[Action-Flow]: Message type can't be recongize!\n");
	}		
}


