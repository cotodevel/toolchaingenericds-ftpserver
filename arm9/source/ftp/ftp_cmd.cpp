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

#include "netdb.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <assert.h>

#include <string>

#include "ftpServer.h"
#include "ftp_cmd.h"
#include "main.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "dswnifi_lib.h"
#include "filehandleTGDS.h"

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




int ftp_cmd_STOR(int s, int cmd, char* arg){
	char * fname = getFtpCommandArg("STOR", arg, 0); 
	string fnameRemote = parsefileNameTGDS(string("0:/") + string(fname));
	printf("STOR cmd: %s",fnameRemote.c_str());
	
	//stor
	//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
	struct sockaddr_in clientAddr;
	int clisock = openAndListenFTPDataPort(&clientAddr);
	int sendResponse = ftpResponseSender(sock2, 150, "Opening BINARY mode data connection for retrieve file from server.");
	
	if(clisock >= 0){
		std::string fileToRetr = fnameRemote;
		int total_len = FS_getFileSize((char*)fileToRetr.c_str());
		FILE * fh = fopen(fileToRetr.c_str(), "w+");
		
		if(fh != NULL){
			//retrieve from server, to client.
			//printf("RETR file %s open OK: size: %d ",fileToRetr.c_str(), total_len);
			//int written = send_file(clisock, fh, total_len);
			//printf("file written %d bytes", written);
			
			char client_reply[5000];
			int received_len = 0;
			int total_len = 0;
			while( ( received_len = recv(clisock, client_reply, sizeof(client_reply), 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
				if(received_len>0) { // data was received!
					total_len += received_len;
					fwrite(client_reply , 1, received_len , fh);
					//printf("Received byte size = %d", received_len);
				}
			}
			
			disconnectAsync(clisock);
			fclose(fh);
			sendResponse = ftpResponseSender(sock2, 226, "Transfer complete.");
		}
		//could not open file
		else{
			printf("STOR file %s open ERROR",fileToRetr.c_str());
			sendResponse = ftpResponseSender(sock2, 451, "Could not open file.");
		}
	}
	else{
		sendResponse = ftpResponseSender(sock2, 425, "Connection closed; transfer aborted.");
	}
	return sendResponse;
}
