#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

//FTP server
#include <stdio.h>
#include "util.h"
#include "ftpServer.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <dswifi9.h>
#include <netdb.h>
#include <socket.h>
#include <in.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "util.h"
#include "sgIP_Config.h"

uint32 CurFTPState = 0;
u32 getFTPState(){
	return CurFTPState;
}

void setFTPState(uint32 FTPState){
	CurFTPState = FTPState;
} 

int ftpResponseSender(int s, int n, char* mes){
	volatile char data[256];
	sprintf((char*)data, "%d %s \n", n, mes);
	return send(s, (char*)&data[0], strlen((char*)&data[0]), 0);
}

int currserverDataListenerSock = -1;
//These two open/close a FTP Server (Passive Mode) Data Port
int openAndListenFTPDataPort(struct sockaddr_in * sain){

	int cliLen = sizeof(struct sockaddr_in);
	int serverDataListenerSock = openServerSyncConn(FTP_PASV_DATA_TRANSFER_PORT, sain);
	currserverDataListenerSock = serverDataListenerSock;
	
	if(serverDataListenerSock == -1){
		printf("failed allocing serverDataListener");
		while(1==1){}
	}
	
	int clisock = accept(serverDataListenerSock, (struct sockaddr *)sain, &cliLen);
	
	if(clisock == -1){
		printf("failed allocing incoming client");
		while(1==1){}
	}
	
	if(clisock > 0) {
		printf("FTP Server (DataPort): Got a connection from:");
		printf("[%s:%d]", inet_ntoa(sain->sin_addr), ntohs(sain->sin_port));
	}
	return clisock;
}

void closeFTPDataPort(int sock){
	disconnectAsync(sock);
	if(currserverDataListenerSock != 0){
		disconnectAsync(currserverDataListenerSock);
		currserverDataListenerSock = -1;
	}
}


/**
 * -1 error, 0 ok
 */
int send_file(int peer, FILE *f, int fileSize) {
    char filebuf[BUF_SIZE+1];
    int written = 0;
    while(fileSize > 0) {
		int readSofar=fread(filebuf, 1, BUF_SIZE, f);
		int n = readSofar;
		int st = 0; 	//sent physically
		int ofst = 0;	//internal offset			
		while(n > 0){
			st = send(peer, filebuf + ofst, n, 0);
			//printf(" %d bytes sent", st);
			n=n-st;
			fileSize-=st;
			ofst+=st;
			written+=st;
		}
		memset(filebuf, 0, sizeof(filebuf));
    }
    return written;
}