#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define PDI_NSCS 0
#define PDI_SCES 1

int logger(char* fileName, char* buf, int len);

int main()
{
	//logger("./123", "abcd\n", 5);
	//logger("./123", "efgh\n", 5);
	logger2("./222", "abcd\n", 5);
	logger2("./222", "efgh\n", 5);
}

int logger(char* fileName, char* buf, int len)
{
	int outputFd, openFlags;
	mode_t filePerms;
	openFlags = O_CREAT | O_RDWR | O_APPEND;
	filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; 
	outputFd = open(fileName, openFlags, filePerms);
	if(outputFd == -1)
	{
		printf("[WARNING]: Open file %s error, please do some check!\n", fileName);
		return PDI_NSCS;
	}
	int result = write(outputFd, buf, len);
	if(result < 0)
	{
		printf("[WARNING]: Log write error, please do some check!\n");
		close(outputFd);
		return PDI_NSCS;
	}
	else
	{
		close(outputFd);
		return PDI_SCES;
	}
}

int logger2(char* fileName, char* buf, int len)
{
	FILE* stream;
	if((stream = fopen(fileName, "a+")) == NULL)
	{
		return PDI_NSCS;
	}
	int result = fwrite(buf, len, 1, stream);
	printf("fwrite counts is %d \n", result);
	fflush(stream);
	fclose(stream);
}
