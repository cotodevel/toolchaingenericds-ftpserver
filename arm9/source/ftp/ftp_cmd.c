#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include "ftp_cmd.h"
#include "ftpServer.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <dswifi9.h>
#include <netdb.h>


/*FTP server*/
#include <socket.h>
#include <in.h>
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


int ftp_cmd_USER(int s, int cmd, char* arg)
{
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_PASS(int s, int cmd, char* arg)
{
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_PWD(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_SYST(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_FEAT(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_TYPE(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_PASV(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}

int ftp_cmd_default(int s, int cmd, char* arg){
	return ftpResponseSender(s, cmd, arg);
}