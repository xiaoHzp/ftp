#include "ftpcommon.h"

void toUpper(char *arg)
{
	char *c = arg;
	while(*c)
	{
		*c = toupper(*c);
		c++;
	}
}

void init(ftpuser* ftpusr,int servfd)
{
	ftpusr->sockfd = servfd;
	ftpusr->datafd = INVALID_SOCKET;
	bzero(ftpusr->curPath,sizeof(ftpusr->curPath));
	strcat(ftpusr->curPath,"/");
	ftpusr->Daddr = (struct sockaddr_in){AF_INET};
	bzero(ftpusr->username,sizeof(ftpusr->username));
	bzero(ftpusr->fileToName,sizeof(ftpusr->fileToName));
	ftpusr->filepos = 0;
	ftpusr->type = 0;
	ftpusr->mode = 0;
}

int isDir(const char* path)
{
	struct stat bufstat;
	if(stat(path,&bufstat) < 0)
		return 0;
	if(S_ISDIR(bufstat.st_mode))
		return 1;
}

int isFile(const char* path)
{
	struct stat bufstat;
	if(stat(path,&bufstat) < 0)
		return 0;
	if(S_ISREG(bufstat.st_mode))
		return 1;
}

int buildDatafd(ftpuser * ftpusr)
{
	if(ftpusr->datafd == INVALID_SOCKET)
	{
		int connfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(20);
		bind(connfd,(struct sockaddr*)&addr,sizeof(addr));
		connect(connfd,(struct sockaddr*)&(ftpusr->Daddr),sizeof(ftpusr->Daddr));
		return connfd;
	}else // 执行过pasv
	{
		int fd =	accept(ftpusr->datafd,NULL,NULL);
		//printf("%d",fd);
		return fd;
	}

}
void gettime(const time_t  time,char *info)
{
	struct tm *tmp;
	tmp = localtime(&time);
	strftime(info,255,"%b %d %R ",tmp);	
}

void getmode(const mode_t mode,char *info)
{
	if(S_ISDIR(mode))
		strcat(info,"d");
	else
		strcat(info,"-");
	if(S_IRUSR & mode)
		strcat(info,"r");
	else
		strcat(info,"-");
	if(S_IWUSR & mode)
		strcat(info,"w");
	else
		strcat(info,"-");
	if(S_IXUSR & mode)
		strcat(info,"x");
	else
		strcat(info,"-");
	if(S_IRGRP & mode)
		strcat(info,"r");
	else
		strcat(info,"-");
	if(S_IWGRP & mode)
		strcat(info,"w");
	else
		strcat(info,"-");
	if(S_IXGRP & mode)
		strcat(info,"x");
	else
		strcat(info,"-");
	if(S_IROTH & mode)
		strcat(info,"r");
	else
		strcat(info,"-");
	if(S_IWOTH & mode)
		strcat(info,"w");
	else
		strcat(info,"-");
	if(S_IXOTH & mode)
		strcat(info,"x");
	else
		strcat(info,"-");
	strcat(info," ");
}

const char* getList(char* path)
{
	DIR *dp;
	struct dirent *dirp;
	struct stat bufstat;
	char *aInfo = (char*)malloc(4096*sizeof(char));
	char info[256];
	bzero(aInfo,sizeof(aInfo));
	
	bzero(info,sizeof(info));
	if(lstat(path,&bufstat) == 0)
	{
		if(S_ISREG(bufstat.st_mode))
		{
			getmode(bufstat.st_mode,info);
			//strcat(info,"1 ");
			strcat(info,"adtim ");
			strcat(info,"adtim ");
			char tmp[20];
			sprintf(tmp,"%8ld ",bufstat.st_size);
			strcat(info,tmp);
			gettime(bufstat.st_mtime,&info[strlen(info)]);
			strcat(info,basename(path));
			strcat(info,"\r\n");
			strcpy(aInfo,info);
			return aInfo;
		}
	}else
	{
		strcpy(aInfo,"No such file or directory.\r\n");
		return aInfo;
	}
	char tmpname[256];
	strcpy(tmpname,path);
	strcat(tmpname,"/");
	int n = strlen(tmpname);
	if((dp = opendir(path)) ==NULL)
	{
		strcpy(aInfo,"read the directory error.\r\n");
		return aInfo;
	}
	while((dirp = readdir(dp)) != NULL)
	{
		strcpy(&tmpname[n],dirp->d_name);
		lstat(tmpname,&bufstat);
		bzero(info,sizeof(info));
		getmode(bufstat.st_mode,info);
		//strcat(info,"1 ");
		strcat(info,"adtim ");
		strcat(info,"adtim ");
		char tmp[20];
		sprintf(tmp,"%8ld ",bufstat.st_size);
		strcat(info,tmp);
		gettime(bufstat.st_ctime,&info[strlen(info)]);
		strcat(info,dirp->d_name);
		strcat(info,"\r\n");
		strcat(aInfo,info);
	}
	return aInfo;
}
void parsecmd(const char* arg,ftpuser* ftpusr)
{
	int i;
	char cmd[5],str[30];
	sscanf(arg,"%s",cmd);
	toUpper(cmd);
	for(i = 0;i<CMDCNT;i++)
		if(strncmp(cmd,cmdlist[i],4) == 0)
			break;
	if(i == CMDCNT)
	{
		// 错误的命令
		ftpusr->message = "500 Syntax error, command unrecognized.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}else if(ftpusr->usrID == -1 && i > 1)
	{
		// 没有登陆，只能进行user命令
		ftpusr->message = "530 Not logged in.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	int ret = sscanf(&arg[strlen(cmd)],"%s",str);
	if(ret == -1)
	{
		strcpy(str,"");
	}
		cmdfun[i](str,ftpusr);
	//do_pasv(str,ftpusr);

}

void do_user(const char* arg,ftpuser* ftpusr)
{
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
	else
	{
		strcpy(ftpusr->username,arg);
		ftpusr->usrID = -1;
		ftpusr->message = "331 User name okay, need password.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_pass(const char* arg,ftpuser* ftpusr)
{
	int i;
	char pd[20];
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
//		printf("%s",ftpusr->message);
		return;
	}
	strcpy(pd,arg);
	for(i = 0;i < USRCNT;i++)
		if(strcmp(userinfo[i].usrname,ftpusr->username) == 0)
			break;

	if(i < USRCNT){
		if(strcmp(userinfo[i].passwd,pd) == 0)
		{
			ftpusr->message = "230 User logged in, proceed.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			ftpusr->usrID = i;
//			printf("%s",ftpusr->message);
			return;
		}else 
		{
			ftpusr->message = "530 Wrong password.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
//			printf("%s",ftpusr->message);
			return;
		}
	}
	ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
//	printf("%s",ftpusr->message);
	bzero(ftpusr->username,sizeof(ftpusr->username));
}

void do_acct(const char*arg,ftpuser* ftpusr)
{
	ftpusr->message = "230 User logged in,proceed.\r\n";
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
}

void do_cwd(const char*arg,ftpuser* ftpusr)
{
	char str[256];
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
//		printf("%s",ftpusr->message);
		return;
	}
	strcpy(str,arg);
	char tmp[256];
	strcpy(tmp,ROOT_DIR);
	if(str[0] != '/') // 相对路径
		strcat(tmp,ftpusr->curPath);
	strcat(tmp,str);
	
	if(isDir(tmp)){
		strcpy(ftpusr->curPath,tmp);
		ftpusr->message = "250 Directory changed.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
//		printf("%s",ftpusr->message);
		if(ftpusr->curPath[strlen(ftpusr->curPath) - 1] != '/')
			strcat(ftpusr->curPath,"/");
	}else
	{
		ftpusr->message = "550 No such directory.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
//		printf("%s",ftpusr->message);
	}
}

void do_rein(const char* arg,ftpuser* ftpusr)
{
	ftpusr->message = "220 Service ready for new user.\r\n";
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	if(ftpusr->datafd != INVALID_SOCKET)
		close(ftpusr->datafd);
	init(ftpusr,ftpusr->sockfd);
}

void do_quit(const char* arg,ftpuser* ftpusr)
{
	ftpusr->message = "221 Service closing control connection.\r\n";
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	if(ftpusr->datafd != INVALID_SOCKET)
		close(ftpusr->datafd);
	close(ftpusr->sockfd);
}

void do_port(const char* arg,ftpuser* ftpusr)
{
	if(strcmp(arg,"") == 0)
	{
label:
		ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	int tmp[6];
	if(sscanf(arg,"%d,%d,%d,%d,%d,%d",&tmp[0],&tmp[1],&tmp[2],&tmp[3],&tmp[4],&tmp[5]) != 6)
		goto label;		
	char str[20];
	sprintf(str,"%d.%d.%d.%d",tmp[0],tmp[1],tmp[2],tmp[3]);
//	printf("%ld",strlen(str));
	inet_pton(AF_INET,str,&(ftpusr->Daddr.sin_addr));
	short port = (short)(tmp[4] * 256+ tmp[5]);
	ftpusr->Daddr.sin_port = htons(port);	
	ftpusr->message = "200 Command okay.\r\n";
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
}

void do_pasv(const char* arg,ftpuser *ftpusr)
{
	short port = 1025;
	struct sockaddr_in addr;
	socklen_t addrlen;
	addrlen = sizeof(addr);
	if(ftpusr->datafd == INVALID_SOCKET)
	{
		ftpusr->datafd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		getsockname(ftpusr->sockfd,(struct sockaddr*)&addr,&addrlen);
		ftpusr->Daddr.sin_addr = addr.sin_addr;
		for(;;port++)
		{
			ftpusr->Daddr.sin_port = htons(port);
			if(bind(ftpusr->datafd,(struct sockaddr*)&(ftpusr->Daddr),sizeof(ftpusr->Daddr)) == 0)
				break;
		//	printf("bind error\n");
		}

		if(listen(ftpusr->datafd,1) < 0)
		{
			ftpusr->message = "421 Failed to create a data connection.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			close(ftpusr->datafd);
			return;
		}
		char str[256];
		sprintf(str,"227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n",ftpusr->Daddr.sin_addr.s_addr & 0xff,(ftpusr->Daddr.sin_addr.s_addr >> 8) & 0xff,
									(ftpusr->Daddr.sin_addr.s_addr >> 16) & 0xff,(ftpusr->Daddr.sin_addr.s_addr >> 24) & 0xff,
									port / 256, port % 256);
		ftpusr->message = str;
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		//fdtmp = accept(ftpusr->datafd,NULL,NULL);
	}
}

void do_type(const char*arg,ftpuser* ftpusr)
{
	char c ;
	if(strcmp(arg,"") == 0 || sscanf(arg,"%c",&c) !=1)
	{
		ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	switch(c)
	{
		case 'A':
			ftpusr->type = 0;
			ftpusr->message = "200 Type set to A\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		break;
		case 'I':
			ftpusr->type = 1;
			ftpusr->message = "200 Type set to I.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			break;
		case 'E':
		case 'L':
			ftpusr->message = "504 Command not impemented for that parameter.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			break;
		default:
			ftpusr->message = "501 Syntax error in parameters or arguments.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_mode(const char*arg, ftpuser *ftpusr)
{
	char ch;
	if(strcmp(arg,"") == 0 || sscanf(arg,"%c",&ch) != 1)
	{
		ftpusr->message = "501 Syntax error in parameters or argument.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	switch(ch)
	{
		case 'S':
			ftpusr->mode = 0;
			ftpusr->message = "200 Mode set to S.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			break;
		case 'B':
		case 'C':
			ftpusr->message = "504 Command not implemented for that parameter.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			break;
		default:
			ftpusr->message = "501 Synatax error in parameters or argument.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_retr(const char *arg,ftpuser *ftpusr)
{
	if(userinfo[ftpusr->usrID].access & USER_RD)
	{
		char path[256];
		strcpy(path,ROOT_DIR);
		int clientDataSock;
		if(strcmp(arg,"") == 0)
		{
			ftpusr->message = "501 Syntax error in parameters or arguement.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			return;
		}
		if(arg[0] != '/') //相对地址
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		if(isFile(path))
		{
			if((clientDataSock = buildDatafd(ftpusr)) > 0){
				char buf[MAXLINE];
				int n;
				int fd = open(path,O_RDONLY);
				lseek(fd,ftpusr->filepos,SEEK_SET);
				while((n = read(fd,buf,MAXLINE)) > 0)
					write(clientDataSock,buf,n);
				ftpusr->message = "226 Transfer complete,closing data connection.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
				close(clientDataSock);

			}else
			{
				ftpusr->message ="425 Can't open data connection.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
				close(clientDataSock);
			}
		}else
		{
			ftpusr->message = "550 No such file.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}

	}else // 没有读取文件的权限
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}	
}

void do_stor(const char* arg,ftpuser *ftpusr)
{
	if(userinfo[ftpusr->usrID].access & USER_WR)
	{
		char path[256];
		int clientDataSock;
		strcpy(path,ROOT_DIR);
		if(strcmp(arg,"") == 0)
		{
			ftpusr->message = "501 Syntax error in parameters or arguement.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			return;
		}
		if(arg[0] !='/')
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		
		if(isFile(path))
		{
			if((clientDataSock = buildDatafd(ftpusr)) > 0){
				char buf[MAXLINE];
				int n;
				int fd = open(path,O_WRONLY | O_CREAT,0764);
				while((n = read(fd,buf,MAXLINE)) > 0)
					write(clientDataSock,buf,n);
				ftpusr->message = "226 Transfer complete,closing data connection.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
				close(clientDataSock);
			}else
			{
				ftpusr->message = "425 Can't open data connection.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
				close(clientDataSock);
			}
		}else
		{
			ftpusr->message = "550 No such file.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_rest(const char* arg, ftpuser *ftpusr)
{
	off_t pos;
	if(sscanf(arg,"%ld",&pos) == 1)
	{
		ftpusr->message = "350 filepos set successfully.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}else
	{
		ftpusr->message = "501 Syntax error in parameters or arguement.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		ftpusr->filepos = pos;
	}
}

void do_rnfr(const char* arg, ftpuser *ftpusr)
{
	char path[256];
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguement.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}	
	if(userinfo[ftpusr->usrID].access & USER_WR){
		strcpy(path,ROOT_DIR);
		if(arg[0]  != '/')
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		if(isFile(path) || isDir(path))
		{
			ftpusr->message = "350 Use RNTO to set the new file(folder) name.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			strcpy(ftpusr->fileToName,path);
		}else
		{
			ftpusr->message = "550 No such file or directory.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_rnto(const char* arg,ftpuser *ftpusr)
{
	char path[256];
	if(strcmp(arg,"") == 0 || strcmp(ftpusr->fileToName,"") == 0)
	{
		ftpusr->message = "550 Syntax error in parameters or arguement.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	if(userinfo[ftpusr->usrID].access & USER_WR){
		strcpy(path,ROOT_DIR);
		if(arg[0] !='/')
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		if(rename(ftpusr->fileToName,path) == 0)
		{
			ftpusr->message = "250 Requested file action okay,complete.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}else
		{
			ftpusr->message = "553 Requested action not taken.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
	bzero(ftpusr->fileToName,sizeof(ftpusr->fileToName));
}

void do_dele(const char* arg,ftpuser *ftpusr)
{
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguements.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	if(userinfo[ftpusr->usrID].access & USER_WR)
	{
		char path[256];
		strcpy(path,ROOT_DIR);
		if(arg[0] != '/')
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		if(isFile(path))
		{
			if(remove(path) == 0)
			{
				ftpusr->message = "250 Requested file action okay,complete.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			}else
			{
				ftpusr->message = "550 Failed to delete the file.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			}
		}else
		{
			ftpusr->message = "550 No such file.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_rmd(const char* arg,ftpuser *ftpusr)
{
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguement.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	if(userinfo[ftpusr->usrID].access & USER_WR)
	{
		char path[256];
		strcpy(path,ROOT_DIR);
		if(arg[0] !='/')
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		if(isDir(path))
		{
			if(remove(path) == 0)
			{
				ftpusr->message = "250 Rqueseted file action okay,complete.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			}else
			{
				ftpusr->message = "550 Fail to remove the directory.\r\n";
				write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			}
		}else
		{
			ftpusr->message = "550 No such directory.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}
}

void do_mkd(const char* arg,ftpuser *ftpusr)
{
	if(strcmp(arg,"") == 0)
	{
		ftpusr->message = "501 Syntax error in parameters or arguements.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		return;
	}
	if(userinfo[ftpusr->usrID].access & USER_WR)
	{
		char path[256];
		strcpy(path,ROOT_DIR);
		if(arg[0] !='/')
			strcat(path,ftpusr->curPath);
		strcat(path,arg);
		if(mkdir(path,0755) == 0)
		{
			ftpusr->message = "257  Create directory successfully.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}else
		{
			ftpusr->message = "550 Failed to remove th directory.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}

}

void do_pwd(const char* arg,ftpuser *ftpusr)
{
	char tmp[256];
	strcpy(tmp,"257 ");
	strcat(tmp,ROOT_DIR);
	strcat(tmp,ftpusr->curPath);
	strcat(tmp,"\r\n");
	ftpusr->message = tmp;
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
}

void do_list(const char* arg,ftpuser *ftpusr)
{
	if(userinfo[ftpusr->usrID].access & USER_RD)
	{
		const	char *p;
		int clientDataSock;
		char path[256];
		strcpy(path,ROOT_DIR);
		if(strcmp("",arg) == 0 || arg[0] !='/')
			strcat(path,ftpusr->curPath);
		//strcat(path,arg);
		if((clientDataSock = buildDatafd(ftpusr)) > 0)
		{
			printf("%s\n",path);
			p = getList(path);
			ftpusr->message = "2017/06/21 09:24 <DIR> contacts\n";
			printf("ok %d\n",write(clientDataSock,ftpusr->message,strlen(ftpusr->message)));
			//write(clientDataSock,p,strlen(p));
			ftpusr->message = "226 Transfer complete,closing data connection.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
			close(clientDataSock);
		}else
		{
			ftpusr->message = "425 Can't open data connection.\r\n";
			write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
		}
	}else
	{
		ftpusr->message = "550 Permission denied.\r\n";
		write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
	}	
}

void do_noop(const char*arg,ftpuser* ftpusr)
{
	ftpusr->message = "200 Command okay.\r\n";
	write(ftpusr->sockfd,ftpusr->message,strlen(ftpusr->message));
}
