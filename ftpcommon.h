#ifndef _FTPUSER_H
#define _FTPUSER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <dirent.h>

#define USER_RD	0b01 // 读权限
#define USER_WR	0b10 // 写权限
#define CMDCNT 33
#define USRCNT 2
#define MAXLINE 4096
#define INVALID_SOCKET -1
#define ROOT_DIR "d:/cgwin/home/adtim/linux/ftp" // 根目录

typedef struct userInfo
{
	char usrname[20];
	char passwd[10];
	u_char access;
}u_info;

u_info userinfo[] = {
						{"anyone","",USER_RD},
						{"adtim","123456",USER_RD | USER_WR}
					};

typedef struct ftpUser{
		int sockfd; // 控制连接
		int datafd; // 数据端链接
	   	struct sockaddr_in Daddr; // 数据链接地址
		off_t filepos; // 传输的起始位置，由rest命令更改，仅在retr命令起作用
		char* message; // 存储反馈信息
		char username[20]; // 保存用户名
		char curPath[256]; //当前路径
		char fileToName[256]; // 保存RNFR所指定的文件路径(绝对路径)
		int usrID; // 记录登陆状态，未登录为-1
		int type; // 数据表示类型
		int mode; // 数据传输模式
}ftpuser;

typedef void (*cmd_func)(const char*,ftpuser*);
/*
enum {
	USER,PASS,ACCT,CWD,CDUP,SMNT,REIN,QUIT,PORT,PASV,TYPE,STRU,MODE,RETR,STOR,STOU,
	APPE,ALLO,REST,RNFR,RNTO,ABOR,DELE,RMD,MKD,PWD,LIST,NLST,SITE,SYST,STAT,HELP,NOOP
};
*/



const char* cmdlist[] = {
	"USER","PASS","ACCT","CWD","CDUP","SMNT",
	"REIN","QUIT","PORT","PASV","TYPE","STRU",
	"MODE","RETR","STOR","STOU","APPE","ALLO",
	"REST","RNFR","RNTO","ABOR","DELE","RMD",
	"MKD","PWD","LIST","NLST","SITE","SYST",
	"STAT","HELP","NOOP"
};

void toUpper(char*); // 小写转大写
void init(ftpuser*,int); // 初始化ftpuser结构
int isDir(const char*); // 判断当前路径是否为目录
int isFile(const char*); //  判断当前路径是否为文件 
int buildDatafd(ftpuser*); // 建立数据连接
const char* getList(char*); // 获取列表信息

void do_user(const char*,ftpuser*);
void do_pass(const char*,ftpuser*);
void do_acct(const char*,ftpuser*);
void do_cwd(const char*,ftpuser*);
void do_rein(const char*,ftpuser*);
void do_quit(const char*,ftpuser*);
void do_port(const char*,ftpuser*);
void do_pasv(const char*,ftpuser*);
void do_type(const char*,ftpuser*);
void do_mode(const char*,ftpuser*);
void do_retr(const char*,ftpuser*);
void do_stor(const char*,ftpuser*);
void do_rest(const char*,ftpuser*);
void do_rnfr(const char*,ftpuser*);
void do_rnto(const char*,ftpuser*);
void do_dele(const char*,ftpuser*);
void do_rmd(const char*,ftpuser*);
void do_mkd(const char*,ftpuser*);
void do_pwd(const char*,ftpuser*);
void do_list(const char*,ftpuser*);
void do_noop(const char*,ftpuser*);

static const cmd_func cmdfun[] = {
	&do_user,&do_pass,&do_acct,&do_cwd,NULL,NULL,
	&do_rein,&do_quit,&do_port,&do_pasv,&do_type,NULL,
	&do_mode,&do_retr,&do_stor,NULL,NULL,NULL,
	&do_rest,&do_rnfr,&do_rnto,NULL,&do_dele,&do_rmd,
	&do_mkd,&do_pwd,&do_list,NULL,NULL,NULL,
	NULL,NULL,&do_noop
								};

void parsecmd(const char*,ftpuser*);



#endif
