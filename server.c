#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#define MAXLINE 4096
#define ROOT /home/adtim/ftp
char curpath[256];
char buf[MAXLINE];

void str_ser(int connfd)
{
	int fd,n;
	fd = open("ftpcommon.c",O_RDONLY);
	if(fd == -1)
	{
		printf("open file error\n");
		return;
	}
	while((n = read(fd,buf,MAXLINE)) > 0)
	{
again:
		write(connfd,buf,n);
	}
	if(n < 0 && errno == EINTR)
		goto again;
	else if(n < 0)
	{
		printf("read error\n");
		return;
	}

/*	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(connfd,(struct sockaddr*)&addr,&len);
	printf("%s",inet_ntoa(addr.sin_addr));
*/
}
int main(int argc,char*argv[])
{
	int listenfd,connfd;
	struct sockaddr_in servaddr;
	
	listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	bzero(&servaddr,sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9877);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));

	listen(listenfd,5);
	connfd = accept(listenfd,NULL,NULL);
	str_ser(connfd);
	exit(0);

}
