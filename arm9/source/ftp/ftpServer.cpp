#include "ftpServer.h"
#include "ftpMisc.h"
#include "main.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "dswnifi_lib.h"
#include "filehandleTGDS.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <dswifi9.h>
#include <netdb.h>

#include <socket.h>
#include <in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 #include <sys/stat.h>

#include "sgIP_Config.h"
#include "sgIP_Hub.h"
#include "wifi_arm9.h"
#include "dswifi9.h"
#include "wifi_shared.h"
#include "utilsTGDS.h"
#include "memoryHandleTGDS.h"
#include "fsfatlayerTGDS.h"

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

bool FTPActiveMode = false;

//server ctx for sock1
//client ctx for sock2
//server_datasck_sain ctx for FTP PASSIVE Mode DataPort ran from Server (DS). (Client only connects to it)
//client_datasck_sain ctx for FTP ACTIVE Mode Dataport ran from Client. (Server DS only connects to it)
struct sockaddr_in server, client, server_datasck_sain, client_datasck_sain;
struct stat obj;

//sock1 = Initial FTP port opened by Server (DS). Basic FTP commands are served through this port.
//sock2 = incoming connection context from the Client. Basically where to send cmds received from sock1.
int sock1 = -1, sock2 = -1;

//server_datasocket == the DATA port open by the Server (DS) whose commands are processed and sent to Client. Server generates and listens cmds through that port.
int server_datasocket = -1;

//client_datasocket == the DATA port open by the Client whose commands are processed and sent to Server (DS). Client generates and listens cmds through that port.
int client_datasocket = -1;
int client_datasocketPortNumber = -1;
char client_datasocketIP[MAX_TGDSFILENAME_LENGTH];


char buf[100], command[5], filename[20];
int k, size, srv_len, cli_len = 0, c;
int filehandle;
bool globaldatasocketEnabled = false;

//current working directory
char currentPath[4096];
char tempBuf[4096];

void ftpInit(){
	if(ListPathPrint==NULL){
		ListPathPrint=(char*)malloc(LISTPATH_SIZE);
	}
	memset(ListPathPrint, 0, LISTPATH_SIZE);
	setFTPState(FTP_SERVER_IDLE);
}
int FTPServerService(){
	int curFTPStatus = 0;
	//handle FTP Server handshake internal cases
	switch(getFTPState()){
		case(FTP_SERVER_IDLE):{
			switch_dswnifi_mode(dswifi_idlemode);
			connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);
			if(sock1 != -1){
				disconnectAsync(sock1);
			}
			if(server_datasocket != -1){
				disconnectAsync(server_datasocket);
			}
			globaldatasocketEnabled = false;
			sock1 = openServerSyncConn(FTP_SERVER_SERVICE_PORT, &server);	//DS Server: listens at port FTP_SERVER_SERVICE_PORT now. Further access() through this port will come from a client.
			printf("[FTP Server:%s:%d]", print_ip((uint32)Wifi_GetIP()), FTP_SERVER_SERVICE_PORT);
			printf("Waiting for connection:");
			setFTPState(FTP_SERVER_CONNECTING);
			curFTPStatus = FTP_SERVER_PROC_RUNNING;
		}
		break;
	
		case(FTP_SERVER_CONNECTING):{
			printf("[Waiting for client...]");
			memset(&client, 0, sizeof(struct sockaddr_in));
			cli_len = sizeof(client);
			sock2 = accept(sock1,(struct sockaddr*)&client, &cli_len);
			
			//if blocking == will hang when recv so can't use that
			//int i=1;
			//ioctl(sock2, FIONBIO,&i); // Client socket is non blocking
			
			//int j=1;
			//ioctl(sock1, FIONBIO,&j); // Server socket is non blocking
			
			//wait for client
			printf("[Client Connected:%s:%d]",inet_ntoa(client.sin_addr), ntohs(client.sin_port));
			setFTPState(FTP_SERVER_CONNECTED_IDLE);
			curFTPStatus = FTP_SERVER_PROC_RUNNING;
		}
		case(FTP_SERVER_CONNECTED_IDLE):{
			//send hello
			ftpResponseSender(sock2, 200, "hello");
			setFTPState(FTP_SERVER_WORKING);
			curFTPStatus = FTP_SERVER_PROC_RUNNING;
		}
		break;
		case(FTP_SERVER_WORKING):{
			
			//Actual FTP Service			
			char buffer[MAX_TGDSFILENAME_LENGTH] = {0};
			int res = recv(sock2, buffer, sizeof(buffer), 0);
			int sendResponse = 0;
			if(res > 0){
				int len = strlen(buffer);
				if(len > 3){
					char command[5] = {0};
					char Debugcommand[5] = {0};
					memcpy((uint8*)&command[0], buffer, 4);
					command[4] = '\0';
					strcpy (Debugcommand, command);
					bool isValidcmd = false;
					
					//four or less cmds
					if(!strcmp(command, "USER"))
					{
						sendResponse = ftp_cmd_USER(sock2, 200, "password ?");
						isValidcmd = true;
					}
					else if(!strcmp(command, "PASS"))
					{
						sendResponse = ftp_cmd_PASS(sock2, 200, "ok");
						isValidcmd = true;
					}
					else if(!strcmp(command, "AUTH"))
					{
						sendResponse = ftpResponseSender(sock2, 504, "AUTH not supported");
						isValidcmd = true;
					}
					else if(!strcmp(command, "BYE") || !strcmp(command, "QUIT"))
					{
						printf("FTP server quitting.. ");
						int i = 1;
						sendResponse = send(sock2, &i, sizeof(int), 0);
						isValidcmd = true;
					}
					
					else if(!strcmp(command, "STOR")){
						sendResponse = ftp_cmd_STOR(sock2, 0, buffer);
						isValidcmd = true;
					}
					
					else if(!strcmp(command, "RETR")){
						sendResponse = ftp_cmd_RETR(sock2, 0, buffer);
						isValidcmd = true;
					}
					//default unsupported, accordingly by: https://tools.ietf.org/html/rfc2389
					else if(!strcmp(command, "FEAT")){
						sendResponse = ftp_cmd_FEAT(sock2, 211, "no-features");
						isValidcmd = true;
					}
					//default unsupported, accordingly by: https://cr.yp.to/ftp/syst.html
					else if(!strcmp(command, "SYST")){
						sendResponse = ftp_cmd_SYST(sock2, 215, "UNIX Type: L8");
						isValidcmd = true;
					}
					//TYPE: I binary data by default
					else if(!strcmp(command, "TYPE")){
						sendResponse = ftp_cmd_TYPE(sock2, 200, "Switching to Binary mode.");
						isValidcmd = true;
					}
					
					else if(!strcmp(command, "CDUP"))
					{
						ftp_cmd_CDUP(sock2, 0, "");
						isValidcmd = true;
					}
					
					else if(!strcmp(command, "PORT"))
					{
						//Connect to Client IP/Port here (DS is Client @ Client Data Port)
						char clientIP[MAX_TGDSFILENAME_LENGTH] = {0};
						char *theIpAndPort;
						int ipAddressBytes[4];
						int portBytes[2];
						theIpAndPort = getFtpCommandArg("PORT", buffer, 0);    
						sscanf(theIpAndPort, "%d,%d,%d,%d,%d,%d", &ipAddressBytes[0], &ipAddressBytes[1], &ipAddressBytes[2], &ipAddressBytes[3], &portBytes[0], &portBytes[1]);
						int ClientConnectionDataPort = ((portBytes[0]*256)+portBytes[1]);
						sprintf(clientIP, "%d.%d.%d.%d", ipAddressBytes[0],ipAddressBytes[1],ipAddressBytes[2],ipAddressBytes[3]);
						
						FTPActiveMode = true;	//enter FTP active mode // //data->clients[socketId].workerData.passiveModeOn = 0; //data->clients[socketId].workerData.activeModeOn = 1;
						
						//prepare client socket (IP/port) for later access
						client_datasocketPortNumber = ClientConnectionDataPort;
						strcpy(client_datasocketIP , clientIP);
						
						sendResponse = ftp_cmd_USER(sock2, 200, "PORT command successful.");
						isValidcmd = true;
					}
					
					//PASV: mode that opens a data port aside the current server port, so binary data can be transfered through it.
					else if(!strcmp(command, "PASV")){
						FTPActiveMode = false;	//enter FTP passive mode
						
						clrscr();
						printf("  ");
						printf("  ");
						printf("  ");
						
						printf("PASV > set data transfer port @ %d", FTP_SERVER_SERVICE_DATAPORT);
						char buf[MAX_TGDSFILENAME_LENGTH] = {0};
						int currentIP = (int)Wifi_GetIP();
						int dataPort = (int)FTP_SERVER_SERVICE_DATAPORT;
						sprintf(buf, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).", (int)(currentIP&0xFF), (int)((currentIP>>8)&0xFF), (int)((currentIP>>16)&0xFF), (int)(currentIP>>24) + 256, dataPort>>8, dataPort&0xFF);
						sendResponse = ftp_cmd_PASV(sock2, 227, buf);
						isValidcmd = true;
					}
					
					else if(!strcmp(command, "LIST")){
						//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
						struct sockaddr_in clientAddr;
						int clisock = openAndListenFTPDataPort(&clientAddr);
						sendResponse = ftpResponseSender(sock2, 150, "Opening BINARY mode data connection for file list.");
						
						if(clisock >= 0){
							//todo: filter different LIST args here
							// send LIST through DATA Port.
							char * listOut = buildList();
							
							int totalListSize = strlen(listOut) + 1;
							int written = 0;
							int st = 0; 	//sent physically
							int ofst = 0;	//internal offset			
							while(totalListSize > 0){
								st = send(clisock, listOut + ofst, totalListSize, 0);
								//printf(" %d bytes sent", st);
								totalListSize-=st;
								ofst+=st;
								written+=st;
							}
							
							u8 endByte=0x0;
							send(clisock, &endByte, 1, 0);
							closeFTPDataPort(clisock);
							
							sendResponse = ftpResponseSender(sock2, 226, "Transfer complete.");
						}
						else{
							sendResponse = ftpResponseSender(sock2, 426, "Connection closed; transfer aborted.");
						}
						isValidcmd = true;
					}
					
					
					else if(!strcmp(command, "DELE")){
						char * fname = getFtpCommandArg("DELE", buffer, 0);  
						string fnameRemote = parsefileNameTGDS(string(fname));
						
						int retVal = remove(fnameRemote.c_str());
						if(retVal==0){
							sendResponse = ftpResponseSender(sock2, 250, "Delete Command successful.");
						}
						else{
							sendResponse = ftpResponseSender(sock2, 450, "Delete Command error.");
						}
						
						printf("trying DELE: [%s] ret:[%d]", fnameRemote.c_str(),retVal);
						
						isValidcmd = true;
					}
					
					if(isValidcmd == false){
						//three or less cmds
						command[3] = '\0';
						
						if(!strcmp(command,"GET"))
						{
							printf("get command!");
							sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
							/*
							sscanf(buf, "%s%s", filename, filename);
							stat(filename, &obj);
							filehandle = open(filename, O_RDONLY);
							size = obj.st_size;
							if(filehandle == -1)
								size = 0;
							sendResponse = send(sock2, &size, sizeof(int), 0);
							if(size){
								sendfile(sock2, filehandle, NULL, size);
							}
							*/
							isValidcmd = true;
						}
						else if(!strcmp(command, "PUT"))
						{
							printf("put command! ");
							sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
							/*
							int c = 0;
							char *f;
							sscanf(buf+strlen(command), "%s", filename);
							recv(sock2, &size, sizeof(int), 0);
							int i = 1;
							while(1)
							{
								filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
								if(filehandle == -1)
								{
									sprintf(filename + strlen(filename), "%d", i);
								}
								else
									break;
							}
							f = malloc(size);
							recv(sock2, f, size, 0);
							c = write(filehandle, f, size);
							close(filehandle);
							sendResponse = send(sock2, &c, sizeof(int), 0);
							*/
							isValidcmd = true;
						}
						else if(!strcmp(command, "CWD")){
							
							char * CurrentWorkingDirectory = (char*)CWDFTP;
							char buf[MAX_TGDSFILENAME_LENGTH] = {0};
							
							char *pathToEnter;
							pathToEnter = getFtpCommandArg("CWD", buffer, 0);
							
							if(chdir((char*)pathToEnter) != 0) {
								printf(" CWD fail => %s ", pathToEnter);
								return ftpResponseSender(sock2, 550, "Error changing directory");
							}
							else{
								printf("1 CWD OK => %s ", pathToEnter);
								strcpy(CurrentWorkingDirectory, pathToEnter);
								return ftpResponseSender(sock2, 250, "Directory successfully changed.");
							}
							
							isValidcmd = true;
						}
						
						//print working directory
						else if(!strcmp(command, "PWD")){
							char * CurrentWorkingDirectory = (char*)CWDFTP;
							if(strlen(CurrentWorkingDirectory) >0 ){
								sendResponse = ftp_cmd_PWD(sock2, 257, CurrentWorkingDirectory);
							}
							else{
								sprintf(CurrentWorkingDirectory,"%s","/");
								sendResponse = ftp_cmd_PWD(sock2, 257, CurrentWorkingDirectory);
							}
							isValidcmd = true;
						}
						else if(!strcmp(command, "MKD")){
							sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
							isValidcmd = true;
						}
						else if(!strcmp(command, "RMD")){
							sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
							isValidcmd = true;
						}
					}
					
					if(isValidcmd == false){
						//two or less cmds
						command[2] = '\0';
						
						if(!strcmp(command, "LS"))
						{
							printf("ls command!");
							/*
							system("ls >temps.txt");
							stat("temps.txt",&obj);
							size = obj.st_size;
							sendResponse = send(sock2, &size, sizeof(int),0);
							filehandle = open("temps.txt", O_RDONLY);
							//sendfile(sock2,filehandle,NULL,size);
							*/
							sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
							isValidcmd = true;
						}
						else if(!strcmp(command, "CD"))
						{
							if(chdir((buffer+3)) == 0){
								c = 1;
							}
							else{
								c = 0;
							}
							sendResponse = send(sock2, &c, sizeof(int), 0);
							isValidcmd = true;
						}
					}
					
					if(isValidcmd == false){
						printf("unhandled command: [%s]",Debugcommand);
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");
					}
				}
			}
			else{
				closeFTPDataPort(sock2);	//sock2, client disconnected, thus server closes its port.
				return FTP_SERVER_CLIENT_DISCONNECTED;
			}
			curFTPStatus = FTP_SERVER_PROC_RUNNING;
		}
		break;
	}
	return curFTPStatus;
}

