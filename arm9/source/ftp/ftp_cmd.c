#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include "ftp_cmd.h"
#include "ftpserver.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <dswifi9.h>
#include <netdb.h>


/*FTP server*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 
/*for getting file size using stat()*/
#include<sys/stat.h>
 
/*for sendfile()*/
//#include<sys/sendfile.h>
 
/*for O_RDONLY*/
#include<fcntl.h>
#include "util.h"
#include "sgIP_Config.h"


void ftp_cmd_USER(int s, int cmd, char* arg)
{
	ftpResponseSender(s, cmd, arg);
}

void ftp_cmd_PASS(int s, int cmd, char* arg)
{
	ftpResponseSender(s, cmd, arg);
}

void ftp_cmd_PWD(int s, int cmd, char* arg){
	ftpResponseSender(s, cmd, arg);
}

void ftp_cmd_SYST(int s, int cmd, char* arg){
	ftpResponseSender(s, cmd, arg);
}

void ftp_cmd_FEAT(int s, int cmd, char* arg){
	ftpResponseSender(s, cmd, arg);
}

void ftp_cmd_TYPE(int s, int cmd, char* arg){
	ftpResponseSender(s, cmd, arg);
}

void ftp_cmd_CWD(int s, int cmd, char* arg){
	//if(arg[0]=='/')strcpy(currentPath,arg);
	//else strcat(currentPath,arg);
	//int l=strlen(currentPath);
	//if(!l || currentPath[l-1]!='/')strcat(currentPath,"/");
	
	if (chdir(currentPath)) {
		//memset(tempBuf, 0, sizeof(tempBuf));
		//otherwise return 500/550
		//sprintf(tempBuf,"%s: No such file or directory.",currentPath);
		ftpResponseSender(sock2, 550, currentPath);
		printf("CWD FAIL => %s ", currentPath);
	}
	else{
		//if dir/file exists then ret: 250 or 200.
		ftpResponseSender(sock2, 250, "ok");
		printf(" CWD OK => %s ", currentPath);
	}
	
	//while(1==1){}	//ok so far
}