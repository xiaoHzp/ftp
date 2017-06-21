#include <stdio.h>
#include <stdlib.h>
#include "ftpcommon.h"
#include "ftpcommon.c"

int main(int argc,char*argv[])
{

	ftpuser ftptmp;
	int listenfd,clientfd;
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(21);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) !=0)
	{
		printf("bind error\n");
		return -1;
	}

	listen(listenfd,5);
	clientfd = accept(listenfd,NULL,NULL);
	write(clientfd,"hello\r\n",sizeof("hello\r\n"));
	init(&ftptmp,clientfd);
	char cmd[255];

	setbuf(stdout,NULL);
	while(1)
	{
		read(ftptmp.sockfd,cmd,255);
		parsecmd(cmd,&ftptmp);
	}	
	
//	setbuf(stdout,NULL);
//	printf("%s",getList(argv[1]));
	return 0;
}
