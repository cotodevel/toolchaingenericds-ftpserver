#include "ftpserver.h"
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


struct sockaddr_in server, client;
struct stat obj;
int sock1 = -1, sock2 = -1;
char buf[100], command[5], filename[20];
int k, size, srv_len, cli_len, c;
int filehandle;


//current working directory
char currentPath[4096];
char tempBuf[4096];


int do_ftp_server(){
	
	//handle FTP Server handshake internal cases
	switch(getFTPState()){
		case(FTP_SERVER_IDLE):{
			
			switch_dswnifi_mode(dswifi_idlemode);
			connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE);
			
			if(sock1 != -1){
				close(sock1);
			}
			
			//set server
			memset(&server, 0, sizeof(struct sockaddr_in));
			memset(&client, 0, sizeof(struct sockaddr_in));
			
			srv_len = sizeof(struct sockaddr_in);
			server.sin_port = htons((int)FTP_PORT);//default listening port
			server.sin_addr.s_addr = INADDR_ANY;	//the socket will be bound to all local interfaces (and we just have one up to this point, being the DS Client IP acquired from the DHCP server).
			
			//prepare socket (server)
			sock1 = socket(AF_INET, SOCK_STREAM, 0);
			
			//can't, the full frame from the DS must be valid to PC Client
			//int i=1;
			//i=ioctl(sock1, FIONBIO,&i); // Client Socket: set non-blocking port (so it allows us to retrieve data without blocking more requests DS FTP Server requires)
			
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
			sock2 = accept(sock1,(struct sockaddr*)&client, &cli_len);
			//can't otherwise duplicate packets 
			//int i=1;
			//i=ioctl(sock2, FIONBIO,&i); // Client Socket: set non-blocking port (so it allows us to retrieve data without blocking more requests DS FTP Server requires)
			
			//will print once client is connected
			printf("[Client Connected]");
			printf("Address: %s ", inet_ntoa(client.sin_addr));
			printf("Port: %d ", (int) ntohs(client.sin_port));
			
			setFTPState(FTP_SERVER_CONNECTING);
		}
		break;
	
		case(FTP_SERVER_CONNECTING):{
			ftpResponseSender(sock2, 200, "ANY-DOMAIN.COM FTP Service");	//after this, a command should be received in the DS
			//todo: find out why client is sending garbage data 
			
			setFTPState(FTP_SERVER_WORKING);
		}
		
		case(FTP_SERVER_WORKING):{
			
			//Actual FTP Service			
			char buffer[512];
			memset(buffer, 0, 512);
			
			int flags = 0;
			int ret = recv_all(sock2, (char*)&buffer[0], sizeof(buffer), flags);
			
			if(ret < 0){
				close(sock2);
				
				setFTPState(FTP_SERVER_IDLE);
				printf("Client disconnected!. Press A to retry.");
				switch_dswnifi_mode(dswifi_idlemode);
				
				scanKeys();
				while(!(keysPressed() & KEY_A)){
					scanKeys();
					IRQVBlankWait();
				}
				menuShow();
				return FTP_SERVER_PROC_FAILED;
			}
			
			//test
			/*
			printf("CMD: %s ",command);
			
			//LOGIN PART
			if(!strcmp(command, "USER"))
			{
				
			}
			
			if(!strcmp(command, "PASS"))
			{
				printf("PASS");
			}
			
			if(!strcmp(command, "ls"))
			{
				system("ls >temps.txt");
				i = 0;
				stat("temps.txt",&obj);
				size = obj.st_size;
				send(sock2, &size, sizeof(int),0);
				filehandle = open("temps.txt", O_RDONLY);
				//sendfile(sock2,filehandle,NULL,size);
			}
			else if(!strcmp(command,"get"))
			{
				sscanf(buf, "%s%s", filename, filename);
				stat(filename, &obj);
				filehandle = open(filename, O_RDONLY);
				size = obj.st_size;
				if(filehandle == -1)
					size = 0;
				send(sock2, &size, sizeof(int), 0);
				if(size){
					//sendfile(sock2, filehandle, NULL, size);
				}
			}
			else if(!strcmp(command, "put"))
			{
				int c = 0, len;
				char *f;
				sscanf(buf+strlen(command), "%s", filename);
				recv(sock2, &size, sizeof(int), 0);
				i = 1;
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
				send(sock2, &c, sizeof(int), 0);
			}
			else if(!strcmp(command, "pwd"))
			{
				system("pwd>temp.txt");
				i = 0;
				FILE*f = fopen("temp.txt","r");
				while(!feof(f))
					buf[i++] = fgetc(f);
				buf[i-1] = '\0';
				fclose(f);
				send(sock2, buf, 100, 0);
			}
			else if(!strcmp(command, "cd"))
			{
				if(chdir(buf+3) == 0)
					c = 1;
				else
					c = 0;
				send(sock2, &c, sizeof(int), 0);
			}


			else if(!strcmp(command, "bye") || !strcmp(command, "quit"))
			{
				printf("FTP server quitting.. ");
				i = 1;
				send(sock2, &i, sizeof(int), 0);
				//exit(0);
			}
			
			*/
			bool valid = false;
			
			//void * memcpy ( void * destination, const void * source, size_t num );
			memcpy ((u8*)command,(u8*)buffer, 4);
			
			//printf("CMD: %s ",command);
			//while(1==1);
			
			if(!strcmp(command, "USER"))
			{
				printf("Sent user resp! ");
				ftp_cmd_USER(sock2, 200, "password ?");
				valid = true;
			}
			
			if(!strcmp(command, "PASS"))
			{
				printf("Sent pass resp! ");
				ftp_cmd_PASS(sock2, 200, "ok");
				valid = true;
			}
			
			if(!strcmp(command, "LIST")){
				valid = true;
			}
			
			if(!strcmp(command, "STOR")){
				valid = true;
			}
			
			if(!strcmp(command, "RETR")){
				valid = true;
			}
			
			//default unsupported, accordingly by: https://tools.ietf.org/html/rfc2389
			if(!strcmp(command, "FEAT")){
				ftp_cmd_FEAT(sock2, 211, "no-features");
				valid = true;
			}
			
			//default unsupported, accordingly by: https://cr.yp.to/ftp/syst.html
			if(!strcmp(command, "SYST")){
				printf("sent SYST cmd ");
				ftp_cmd_SYST(sock2, 215, "UNIX Type: L8");
				valid = true;
			}
			
			//TYPE:PASV by default
			if(!strcmp(command, "TYPE")){
				//227
				//500, 501, 502, 421, 530
				printf("set PASV response ");
				ftp_cmd_TYPE(sock2, 227, "Entering Passive Mode");
				//while(1==1){}	//after this it does nothing. check why
				valid = true;
			}
			
			
			//try 3 CMD
			if(valid==false){
				memset(command, 0, sizeof(command));
				//void * memcpy ( void * destination, const void * source, size_t num );
				memcpy ((u8*)command,(u8*)buffer, 3);
			}
			
			
			if(!strcmp(command, "CWD")){
				
				//first arg: 	socket will be replaced anyway; 
				//second arg: 	response number will be overriden by CWD´s file/dir availability
				//third arg: 	another dir you want to access right now
				ftp_cmd_CWD(0, 0, "");
				valid = true;
			}
			
			if(!strcmp(command, "PWD")){
				
				//Set default PWD
				sprintf(currentPath,"%s","/");
				
				/*
				if (getpwd((const char *)currentPath) != NULL){
					
					memcpy ( currentPath, (u8*) getpwd((const char *)currentPath), sizeof(getpwd((const char *)currentPath)));
					
					//fprintf(stdout, "Current working dir: %s ", cwd);
					printf("PWD OK: %s ",currentPath);
				}
				else{
					printf("getcwd() error");
				}
				*/
				
				//void * memcpy ( void * destination, const void * source, size_t num );
				//memcpy((u8*)tmpStr,cwd,sizeof(cwd));
				
				printf("trying dir: %s",currentPath);
				
				if (chdir(currentPath)) {
					printf("PWD ERROR");
				}
				else{
					printf("PWD OK");
				}
				
				//while(1==1){}	//works so far.
				
				ftp_cmd_PWD(sock2, 257, currentPath);
				valid = true;
			}
			
			if(!strcmp(command, "MKD")){
				valid = true;
			}
			
			if(!strcmp(command, "RMD")){
				valid = true;
			}
			
			if(!valid){
				ftpResponseSender(sock2, 502, "invalid command");
			}
		}
		break;
	}
	
	return FTP_SERVER_PROC_RUNNING;
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
