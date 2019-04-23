#include "ftpServer.h"
#include "ftp_cmd.h"
#include "main.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "dswnifi_lib.h"

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

/*for O_RDONLY*/
#include<fcntl.h>
#include "util.h"
#include "sgIP_Config.h"
#include "sgIP_Hub.h"
#include "wifi_arm9.h"
#include "dswifi9.h"
#include "wifi_shared.h"
#include "utilsTGDS.h"
#include "memoryHandleTGDS.h"
#include "fsfatlayerTGDS.h"

#include "netdb_dswifi.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <socket_dswifi.h>
#include <assert.h>


struct sockaddr_in server, client, server_datasck;
struct stat obj;
int sock1 = -1, sock2 = -1, server_datasocket = -1;
char buf[100], command[5], filename[20];
int k, size, srv_len, cli_len = 0, c;
int filehandle;
bool globaldatasocketEnabled = false;

//current working directory
char currentPath[4096];
char tempBuf[4096];


int do_ftp_server(){
	int curFTPStatus = 0;
	//handle FTP Server handshake internal cases
	switch(getFTPState()){
		case(FTP_SERVER_IDLE):{
			
			switch_dswnifi_mode(dswifi_idlemode);
			connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);
			
			if(sock1 != -1){
				close(sock1);
			}
			
			if(server_datasocket != -1){
				close(server_datasocket);
			}
			
			globaldatasocketEnabled = false;
			//set server
			memset(&server, 0, sizeof(struct sockaddr_in));
			
			srv_len = sizeof(struct sockaddr_in);
			server.sin_port = htons((int)FTP_PORT);//default listening port
			server.sin_addr.s_addr = INADDR_ANY;	//the socket will be bound to all local interfaces (and we just have one up to this point, being the DS Client IP acquired from the DHCP server).
			
			//prepare socket (server)
			sock1 = socket(AF_INET, SOCK_STREAM, 0);
			int enable = 1;
			if (setsockopt(sock1, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){	//socket can be respawned ASAP if it's dropped
				printf("setsockopt(SO_REUSEADDR) failed");
				return FTP_SERVER_PROC_FAILED;
			}
			if(sock1 == -1){
				printf("Socket creation failed");
				return FTP_SERVER_PROC_FAILED;
			}
			k = bind(sock1,(struct sockaddr*)&server,srv_len);
			if(k == -1){
				close(sock1);
				printf("Binding error");
				return FTP_SERVER_PROC_FAILED;
			}
			int MAXCONN = 20;
			k = listen(sock1,MAXCONN);
			if(k == -1){
				close(sock1);
				printf("Listen failed");
				return FTP_SERVER_PROC_FAILED;
			}
			
			printf("FTP Server Begins");
			printf("server address: %s ", (char*)print_ip((uint32)Wifi_GetIP()));
			printf("local port: %d ", (int) ntohs(server.sin_port));
			printf("Waiting for connection:");
			/*
			The accept() system call is used with connection-based socket types
			   (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first connection
			   request on the queue of pending connections for the listening socket,
			   sockfd, creates a new connected socket, and returns a new file
			   descriptor referring to that socket.  The newly created socket is not
			   in the listening state.  The original socket sockfd is unaffected by
			   this call.
			*/
			
			//int i=1;
			//i=ioctl(sock2, FIONBIO,&i);
			
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
			printf("[Client Connected]");
			printf("Address: %s ", inet_ntoa(client.sin_addr));
			//printf("Port: %d ", (int) ntohs(client.sin_port));
			
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
			char buffer[256] = {0};
			int res = recv(sock2, buffer, 256, 0);
			int sendResponse = 0;
			if(res >= 0){
				int len = strlen(buffer);
				if(len > 3){
					char command[5] = {0};
					char Debugcommand[5] = {0};
					memcpy((uint8*)&command[0], buffer, 4);
					command[4] = '\0';
					strcpy (Debugcommand, command);
					
					//printf("CMD:[%s]",command);
					
					bool isValidcmd = false;
					//four or less cmds
					if(!strcmp(command, "USER"))
					{
						printf("Sent user resp! ");
						sendResponse = ftp_cmd_USER(sock2, 200, "password ?");
						isValidcmd = true;
					}
					else if(!strcmp(command, "PASS"))
					{
						printf("Sent pass resp! ");
						sendResponse = ftp_cmd_PASS(sock2, 200, "ok");
						isValidcmd = true;
					}
					else if(!strcmp(command, "AUTH"))
					{
						printf("Sent AUTH resp! ");
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
					else if(!strcmp(command, "LIST")){
						printf("LIST command! ");
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
						isValidcmd = true;
					}
					else if(!strcmp(command, "STOR")){
						printf("STOR command! ");
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
						isValidcmd = true;
					}
					else if(!strcmp(command, "RETR")){
						printf("STOR command! ");
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
						isValidcmd = true;
					}
					//default unsupported, accordingly by: https://tools.ietf.org/html/rfc2389
					else if(!strcmp(command, "FEAT")){
						sendResponse = ftp_cmd_FEAT(sock2, 211, "no-features");
						isValidcmd = true;
					}
					//default unsupported, accordingly by: https://cr.yp.to/ftp/syst.html
					else if(!strcmp(command, "SYST")){
						printf("sent SYST cmd ");
						sendResponse = ftp_cmd_SYST(sock2, 215, "UNIX Type: L8");
						isValidcmd = true;
					}
					//TYPE: I binary data by default
					else if(!strcmp(command, "TYPE")){
						//
						printf("TYPE > Switching to Binary mode.");
						sendResponse = ftp_cmd_TYPE(sock2, 200, "Switching to Binary mode.");
						isValidcmd = true;
					}
					
					//PASV: mode that opens a data port aside the current server port, so binary data can be transfered through it.
					else if(!strcmp(command, "PASV")){
						
						bool datasocketEnabled = true;
						if(globaldatasocketEnabled == false){
							//set server PASV data socket
							memset(&server_datasck, 0, sizeof(struct sockaddr_in));
							
							int socksrv_len = sizeof(struct sockaddr_in);
							server_datasck.sin_port = htons((int)FTP_PASV_DATA_TRANSFER_PORT); //default listening port
							server_datasck.sin_addr.s_addr = INADDR_ANY;	//the socket will be bound to all local interfaces (and we just have one up to this point, being the DS Client IP acquired from the DHCP server).
							
							//prepare socket (server_datasck)
							server_datasocket = socket(AF_INET, SOCK_STREAM, 0);
							int enable = 1;
							if (setsockopt(server_datasocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){	//socket can be respawned ASAP if it's dropped
								printf("setsockopt(SO_REUSEADDR) failed");
								datasocketEnabled = false;
							}
							if(server_datasocket == -1){
								printf("Socket creation failed");
								datasocketEnabled = false;
							}
							if(bind(server_datasocket,(struct sockaddr*)&server_datasck,socksrv_len) == -1){
								close(server_datasocket);
								printf("Binding error");
								datasocketEnabled = false;
							}
							int MAXCONNSOCKDATASERV = 2;
							if(listen(server_datasocket,MAXCONNSOCKDATASERV) == -1){
								close(server_datasocket);
								printf("Listen failed");
								datasocketEnabled = false;
							}
							globaldatasocketEnabled = true;
						}
						
						if( (globaldatasocketEnabled == true) || (datasocketEnabled == true) ){
							printf("PASV > set data transfer port @ %d", FTP_PASV_DATA_TRANSFER_PORT);
							char buf[256] = {0};
							sprintf(buf, "Entering Passive Mode (%d).",FTP_PASV_DATA_TRANSFER_PORT);
							sendResponse = ftp_cmd_PASV(sock2, 227, buf);
						}
						else{
							sendResponse = ftp_cmd_PASV(sock2, 425, "Cannot open data connection");
						}
						isValidcmd = true;
					}
					
					else if(!strcmp(command, "CDUP"))
					{
						char tempnewDir[MAX_TGDSFILENAME_LENGTH+1] = {0};
						char * CurrentWorkingDirectory = (char*)&TGDSCurrentWorkingDirectory[0];
						strcpy (tempnewDir, CurrentWorkingDirectory);
						bool cdupStatus = leaveDir(tempnewDir);
						char buf[256] = {0};
						if(cdupStatus == true){
							sprintf(buf,"%s","OK");
						}
						else{
							sprintf(buf,"%s","ERROR");
						}
						sendResponse = ftp_cmd_USER(sock2, 200, buf);
						isValidcmd = true;
					}
					
					if(isValidcmd == false){
						//three or less cmds
						command[3] = '\0';
						
						if(!strcmp(command,"GET"))
						{
							printf("get command!");
							sscanf(buf, "%s%s", filename, filename);
							stat(filename, &obj);
							filehandle = open(filename, O_RDONLY);
							size = obj.st_size;
							if(filehandle == -1)
								size = 0;
							sendResponse = send(sock2, &size, sizeof(int), 0);
							if(size){
								//sendfile(sock2, filehandle, NULL, size);
							}
							
							isValidcmd = true;
						}
						else if(!strcmp(command, "PUT"))
						{
							printf("put command! ");
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
							isValidcmd = true;
						}
						else if(!strcmp(command, "CWD")){
							char * CurrentWorkingDirectory = (char*)&TGDSCurrentWorkingDirectory[0];
							char buf[256] = {0};
							if (chdir(CurrentWorkingDirectory)) {
								sprintf(buf, "%s", "Directory successfully changed.");
							}
							else{
								sprintf(buf, "%s", "Error changing directory");
							}
							sendResponse = ftp_cmd_CWD(sock2, 250, buf);
							isValidcmd = true;
						}
						
						//print working directory
						else if(!strcmp(command, "PWD")){
							//Send CWD 
							char * CurrentWorkingDirectory = (char*)&TGDSCurrentWorkingDirectory[0];
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
							system("ls >temps.txt");
							stat("temps.txt",&obj);
							size = obj.st_size;
							sendResponse = send(sock2, &size, sizeof(int),0);
							filehandle = open("temps.txt", O_RDONLY);
							//sendfile(sock2,filehandle,NULL,size);
							
							sendResponse = ftpResponseSender(sock2, 502, "invalid command");//todo
							isValidcmd = true;
						}
						else if(!strcmp(command, "CD"))
						{
							if(chdir(buf+3) == 0)
								c = 1;
							else
								c = 0;
							sendResponse = send(sock2, &c, sizeof(int), 0);
							isValidcmd = true;
						}
					}
					
					if(isValidcmd == false){
						printf("unhandled command: [%s]",Debugcommand);
						sendResponse = ftpResponseSender(sock2, 502, "invalid command");
					}
				}
				
				if(sendResponse < 0){
					/*
					close(sock2);
					close(sock1);
					setFTPState(FTP_SERVER_IDLE);
					printf("Client disconnected!. Press A to retry.");
					scanKeys();
					while(!(keysPressed() & KEY_A)){
						scanKeys();
						IRQVBlankWait();
					}
					main(0, (sint8**)"");
					*/
					curFTPStatus = FTP_SERVER_PROC_FAILED;	//WIFI connected but FTP client disconnected
				}
			}
		}
		break;
	}
	return curFTPStatus;
	
	//close(sock2); //close ftp server
}

/*
int ftp_openCommandChannel()
{
	if(sock1<0)
	{
		//struct sockaddr_in serv_addr; //struct server

		sock1 = socket(AF_INET, SOCK_STREAM, 0);
		memset(&server, '0', sizeof(server));
		
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = htonl("192.168.43.108");
		server.sin_port = htons(FTP_PORT); 

		bind(sock1, (struct sockaddr*)&server, sizeof(server)); 
		fcntl(sock1, F_SETFL, O_NONBLOCK);

		listen(sock1, 10); 
		
	}
	
	printf("Waiting for connection from Client. ");
	
	int ret  = accept(sock1,(struct sockaddr*)&client, &cli_len);
	
	
	//free up server since we recv commands from client
	if(ret>=0)
	{
		sock2 = ret;
		//client part
		memset(&client, '0', sizeof(client));
		cli_len = sizeof(client);
		client.sin_addr.s_addr = htonl(INADDR_ANY);	//ACCEPT CONN FROM ANY CLIENT
		client.sin_port = htons((int)FTP_PORT);
		
		//closesocket(sock1);
		//sock1=-1;
		//fcntl(ret, F_SETFL, O_NONBLOCK);
		
		//will print once client is connected
		//printf(" Client Connected ");
		//printf("client address: %s ", inet_ntoa( client.sin_addr));
		//printf("client port: %d ", (int) ntohs(client.sin_port));
		
	}

	return ret;
}
*/
/*
int updateclient_ftp_conn(){
	printf("Waiting for connection. ");
	sock2 = accept(sock1,(struct sockaddr*)&client, &cli_len);
	i = 1;
	
	//will print once client is connected
	printf("Client Connected. ");
	printf("client address: %s ", inet_ntoa( client.sin_addr));
	printf("client port: %d ", (int) ntohs(client.sin_port));
	

}
*/

/*
int ftp_getConnection()
{
	int connfd = ftp_openCommandChannel();
	//Do we have Client activity?
	if(connfd>=0)
	{
		printf("received connection! Socket: %d welcome! ",connfd);
		ftpResponseSender(connfd, 200, "hello");
	}
	return connfd;
}
*/


