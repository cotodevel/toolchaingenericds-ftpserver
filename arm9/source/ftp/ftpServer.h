#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#define FTP_PORT 21

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

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


#endif

#define MY_PORT_ID 6081
#define MAXLINE 256
#define MAXSIZE 512   

#define ACK                   2
#define NACK                  3
#define REQUESTFILE           100
#define COMMANDNOTSUPPORTED   150
#define COMMANDSUPPORTED      160
#define BADFILENAME           200
#define FILENAMEOK            400

#ifdef __cplusplus
extern "C" {
#endif

extern int do_ftp_server();
int ftp_openCommandChannel();
int ftp_getConnection();

extern struct sockaddr_in server, client;
extern struct stat obj;
extern int sock1, sock2;
extern char buf[100], command[5], filename[20];
extern int k, i, size, srv_len,cli_len, c;
extern int filehandle;

extern char currentPath[4096];
extern char tempBuf[4096];

#ifdef __cplusplus
}
#endif