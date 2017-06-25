#include "ftpcommon.h"
#include "ftpcommon.c"
#include <pthread.h>

void* do_cmd(void*arg)
{
	char cmd[255];
	ftpuser *ftptmp = (ftpuser*)arg;
	pthread_detach(pthread_self());
	while(1)
	{
		if(read(ftptmp->sockfd,cmd,255) == 0)
			break;
		parsecmd(cmd,ftptmp);
	}
	pthread_exit(0);
}
void* do_ser(void*arg) // 处理控制链接
{
	int clientfd;
	pthread_t tid;
	pthread_detach(pthread_self());
	while((clientfd = accept(*((int*)arg),NULL,NULL)) > 0)
	{
		ftpuser ftptmp;
		init(&ftptmp,clientfd);
		ftptmp.message = "220 Welcome to my ftpserver.\r\n";
		write(clientfd,ftptmp.message,strlen(ftptmp.message));
		pthread_create(&tid,NULL,do_cmd,(void*)&ftptmp);
	}	
	pthread_exit(0);
}

int main(int argc, char*argv[])
{
	ftpuser ftptmp;
	pthread_t tid;
	int listenfd,clientfd;
	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(servaddr));

	listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9877);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))!=0)
	{
		printf("bind error\n");
		return -1;
	}

	listen(listenfd,5);
	pthread_create(&tid,NULL,do_ser,(void*)&listenfd);
/*	clientfd = accept(listenfd,NULL,NULL);
	write(clientfd,"hello\r\n",sizeof("hello\r\n"));
	init(&ftptmp,clientfd);
	char cmd[255];

	setbuf(stdout,NULL);
	while(1)
	{
		read(ftptmp.sockfd,cmd,255);
		parsecmd(cmd,&ftptmp);
	}
*/
	char cmd[255];
	while(scanf("%s",cmd) == 1 && strcmp(cmd,"quit") != 0);
	close(listenfd);
	return 0;
}
