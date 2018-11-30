#include "ftpserver.h"
#include "ftp_cmd.h"

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
#include "sgIP_Hub.h"


struct sockaddr_in server, client;
struct stat obj;
int sock1, sock2;
char buf[100], command[5], filename[20];
int k, i, size, srv_len,cli_len, c;
int filehandle;


//current working directory
char currentPath[4096];
char tempBuf[4096];

void ftp_server_setup(){
	//prepare socket (server)
	sock1 = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sock1 == -1)
	{
		printf("Socket creation failed");
		//exit(1);
	}
	
	//set server
	srv_len = sizeof(server);
	server.sin_port = htons((int)FTP_PORT);//default listening port
	server.sin_addr.s_addr = inet_addr("192.168.43.108"); //192.168.43.108
	k = bind(sock1,(struct sockaddr*)&server,srv_len);
	if(k == -1)
	{
		printf("Binding error");
		//exit(1);
	}
	k = listen(sock1,1);
	if(k == -1)
	{
		printf("Listen failed");
		//exit(1);
	}
	
	iprintf("\t FTP Server Begins\n");
	iprintf("server address: %s\n", inet_ntoa( server.sin_addr));
	iprintf("local port: %d\n", (int) ntohs(server.sin_port));
	
	
	iprintf("WAITING FOR CONNECTION");
	sock2 = accept(sock1,(struct sockaddr*)&client, &cli_len);
	i = 1;
	
		
	//will print once client is connected
	iprintf("\t Client Connected\n");
	ftpResponseSender(sock2, 200, "hello");
	
	iprintf("client address: %s\n", inet_ntoa( client.sin_addr));
	iprintf("client port: %d\n", (int) ntohs(client.sin_port));
	

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
	
	iprintf("WAITING FOR CONNECTION FROM CLIENT \n");
	
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
		//iprintf("\t Client Connected\n");
		//iprintf("client address: %s\n", inet_ntoa( client.sin_addr));
		//iprintf("client port: %d\n", (int) ntohs(client.sin_port));
		
	}

	return ret;
}
*/
/*
int updateclient_ftp_conn(){
	iprintf("WAITING FOR CONNECTION");
	sock2 = accept(sock1,(struct sockaddr*)&client, &cli_len);
	i = 1;
	
	//will print once client is connected
	iprintf("\t Client Connected\n");
	iprintf("client address: %s\n", inet_ntoa( client.sin_addr));
	iprintf("client port: %d\n", (int) ntohs(client.sin_port));
	

}
*/

/*
int ftp_getConnection()
{
	int connfd = ftp_openCommandChannel();
	//Do we have Client activity?
	if(connfd>=0)
	{
		iprintf("received connection ! %d\ngreeting... \n ",connfd);
		ftpResponseSender(connfd, 200, "hello");
	}
	return connfd;
}
*/


int do_ftp_server(){
	
		
	char buffer[256];
	memset(buffer, 0, 256);
	int ret=recv(sock2,buffer,256,0);
	
	if(!ret)return 1; //client has disconnected
	//else return ftp_processCommand(s,buffer);
	
	//test
	/*
	iprintf("CMD: %s \n",command);
	
	//LOGIN PART
	if(!strcmp(command, "USER"))
	{
		
	}
	
	if(!strcmp(command, "PASS"))
	{
		iprintf("PASS");
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
		printf("FTP server quitting..\n");
		i = 1;
		send(sock2, &i, sizeof(int), 0);
		//exit(0);
	}
	
	*/
	bool valid = false;
	
	//void * memcpy ( void * destination, const void * source, size_t num );
	memcpy ((u8*)command,(u8*)buffer, 4);
	
	iprintf("CMD: %s \n",command);
	
	if(!strcmp(command, "USER"))
	{
		iprintf("sent user resp!\n ");
		ftp_cmd_USER(sock2, 200, "password ?");
		valid = true;
	}
	
	if(!strcmp(command, "PASS"))
	{
		iprintf("sent pass resp!\n ");
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
		iprintf("sent SYST cmd \n");
		ftp_cmd_SYST(sock2, 215, "UNIX Type: L8");
		valid = true;
	}
	
	//TYPE:PASV by default
	if(!strcmp(command, "TYPE")){
		//227
		//500, 501, 502, 421, 530
		iprintf("set PASV response \n");
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
			
			//fprintf(stdout, "Current working dir: %s\n", cwd);
			iprintf("PWD OK: %s \n",currentPath);
		}
		else{
			//perror("getcwd() error");
			iprintf("PWD PHAIL \n");
		}
		*/
		
		//void * memcpy ( void * destination, const void * source, size_t num );
		//memcpy((u8*)tmpStr,cwd,sizeof(cwd));
		
		iprintf("trying dir: %s",currentPath);
		
		if (chdir(currentPath)) {
			iprintf("PWD ERROR");
		}
		else{
			iprintf("PWD OK");
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
	
	
	if(!valid)
		ftpResponseSender(sock2, 502, "invalid command");
	
	return 0;
}