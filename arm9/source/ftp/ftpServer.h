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
extern bool globaldatasocketEnabled;

//server ctx for sock1
//client ctx for sock2
//server_datasck_sain ctx for FTP PASSIVE Mode DataPort ran from Server (DS). (Client only connects to it)
//client_datasck_sain ctx for FTP ACTIVE Mode Dataport ran from Client. (Server DS only connects to it)
extern struct sockaddr_in server, client, server_datasck_sain, client_datasck_sain;
extern struct stat obj;

//sock1 = Initial FTP port opened by Server (DS). Basic FTP commands are served through this port.
//sock2 = incoming connection context from the Client. Basically where to send cmds received from sock1.
extern int sock1, sock2;

//server_datasocket == the DATA port open by the Server (DS) whose commands are processed and sent to Client. Server generates and listens cmds through that port.
extern int server_datasocket;

//client_datasocket == the DATA port open by the Client whose commands are processed and sent to Server (DS). Client generates and listens cmds through that port.
extern int client_datasocket;

extern char buf[100], command[5], filename[20];
extern int k, i, size, srv_len,cli_len, c;
extern int filehandle;
extern char *getFtpCommandArg(char * theCommand, char *theCommandString, int skipArgs);
extern bool FTPActiveMode;

extern char currentPath[4096];
extern char tempBuf[4096];

#ifdef __cplusplus
}
#endif