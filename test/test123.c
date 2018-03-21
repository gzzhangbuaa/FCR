#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	char* ptr = "<nameID : hostName>\n";
	int len = strlen(ptr);
	printf("str is %s\n,len is %d\n", ptr, len);
}
