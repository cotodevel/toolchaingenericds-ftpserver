#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include "ftpMisc.h"
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
#include "ftpMisc.h"
#include "main.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "dswnifi_lib.h"
#include "filehandleTGDS.h"

#include <socket.h>
#include <in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 
#include "sgIP_Config.h"
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

//FTP server
#include <stdio.h>
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
#include "sgIP_Config.h"

//C++ part
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iterator>
#include <sstream>

using namespace std;


template <class T> std::string to_string (const T& t)
{
   std::stringstream ss;
   ss << t;
   return ss.str();
}

char CWDFTP[512];
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
char ListPathPrint[2048];

char * buildList(){
	memset(ListPathPrint, 0, sizeof(ListPathPrint));
	std::string res = "";
	// dir to browse
	std::string curDir = string(CWDFTP);
	std::vector<class FileDirEntry> filedirEntries = browse(curDir, false);
	
	int listFiles = 0;
	int listDirs = 0;
	for(int i = 0; i < (int)filedirEntries.size() ; i++){
		if(filedirEntries.at(i).gettype() == FT_FILE){
			int fSize = FS_getFileSize((char*)filedirEntries.at(i).getfilePathFilename().c_str());
			res += (std::string("-rwxrwxrwx   2 DS        " + to_string(fSize) + " Feb  1  2009 " +filedirEntries.at(i).getfilePathFilename() +" \r\n"));
			listFiles++;
		}
		else if(filedirEntries.at(i).gettype() == FT_DIR){
			res += (std::string("drwxrwxrwx   2 DS        " + to_string(0) + " Feb  1  2009 " +filedirEntries.at(i).getfilePathFilename() +" \r\n"));
			listDirs++;
		}
	}
	printf(" >> browse(%s)", curDir.c_str());
	printf("Dir: %s Browsing End. %d files - %d dir(s)", curDir.c_str(), listFiles, listDirs);
	int sizeList = strlen(res.c_str());
	if((int)sizeList > (int)sizeof(ListPathPrint)){
		sizeList = sizeof(ListPathPrint);
	}
	strncpy( (char*)&ListPathPrint[0], (const char *) res.c_str(), sizeList);
	return (char*)&ListPathPrint[0];
}

//FTP Command implementation.
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

int ftp_cmd_RETR(int s, int cmd, char* arg){
	char * fname = getFtpCommandArg("RETR", arg, 0); 
	string fnameRemote = parsefileNameTGDS(string("0:/") + string(fname));
	printf("RETR cmd: %s",fnameRemote.c_str());
	
	//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
	struct sockaddr_in clientAddr;
	int clisock = openAndListenFTPDataPort(&clientAddr);
	int sendResponse = ftpResponseSender(s, 150, "Opening BINARY mode data connection for retrieve file from server.");
	
	if(clisock >= 0){
		std::string fileToRetr = fnameRemote;
		int total_len = FS_getFileSize((char*)fileToRetr.c_str());
		FILE * fh = fopen(fileToRetr.c_str(), "r");
		
		if(fh != NULL){
			//retrieve from server, to client.
			int written = send_file(clisock, fh, total_len);
			//printf("file written %d bytes", written);
			disconnectAsync(clisock);
			fclose(fh);
			sendResponse = ftpResponseSender(s, 226, "Transfer complete.");
		}
		//could not open file
		else{
			printf("RETR file %s open ERROR",fileToRetr.c_str());
			sendResponse = ftpResponseSender(s, 451, "Could not open file.");
		}
	}
	else{
		sendResponse = ftpResponseSender(s, 425, "Connection closed; transfer aborted.");
	}
	return sendResponse;
}


int ftp_cmd_STOR(int s, int cmd, char* arg){
	char * fname = getFtpCommandArg("STOR", arg, 0); 
	string fnameRemote = parsefileNameTGDS(string("0:/") + string(fname));
	printf("STOR cmd: %s",fnameRemote.c_str());
	
	//Open Data Port for FTP Server so Client can connect to it (FTP Passive Mode)
	struct sockaddr_in clientAddr;
	int clisock = openAndListenFTPDataPort(&clientAddr);
	int sendResponse = ftpResponseSender(s, 150, "Opening BINARY mode data connection for retrieve file from server.");
	
	if(clisock >= 0){
		std::string fileToRetr = fnameRemote;
		int total_len = FS_getFileSize((char*)fileToRetr.c_str());
		FILE * fh = fopen(fileToRetr.c_str(), "w+");
		
		if(fh != NULL){
			//retrieve data from client socket.
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
			sendResponse = ftpResponseSender(s, 226, "Transfer complete.");
		}
		//could not open file
		else{
			printf("STOR file %s open ERROR",fileToRetr.c_str());
			sendResponse = ftpResponseSender(s, 451, "Could not open file.");
		}
	}
	else{
		sendResponse = ftpResponseSender(s, 425, "Connection closed; transfer aborted.");
	}
	return sendResponse;
}


uint32 CurFTPState = 0;
u32 getFTPState(){
	return CurFTPState;
}

void setFTPState(uint32 FTPState){
	CurFTPState = FTPState;
} 

int ftpResponseSender(int s, int n, char* mes){
	volatile char data[MAX_TGDSFILENAME_LENGTH];
	sprintf((char*)data, "%d %s \n", n, mes);
	return send(s, (char*)&data[0], strlen((char*)&data[0]), 0);
}

int currserverDataListenerSock = -1;

//These two open/close a FTP Server (Passive Mode) Data Port
int openAndListenFTPDataPort(struct sockaddr_in * sain){

	int cliLen = sizeof(struct sockaddr_in);
	int serverDataListenerSock = openServerSyncConn(FTP_SERVER_SERVICE_DATAPORT, sain);
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

int send_file(int peer, FILE *f, int fileSize) {
    char filebuf[BUF_SIZE+1];
    int written = 0;
    while(fileSize > 0) {
		int readSofar=fread(filebuf, 1, BUF_SIZE, f);
		int n = readSofar;
		int st = 0; 	//sent physically
		int ofst = 0;	//internal offset			
		while(n > 0){
			st = send(peer, filebuf + ofst, BUF_SIZE, 0);
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

string ToStr( char c ) {
   return string( 1, c );
}

template<class Iter>
Iter splitStrings(const std::string &s, const std::string &delim, Iter out)
{
	if (delim.empty()) {
		*out++ = s;
		return out;
	}
	size_t a = 0, b = s.find(delim);
	for (; b != std::string::npos;
		a = b + delim.length(), b = s.find(delim, a))
	{
		*out++ = std::move(s.substr(a, b - a));
	}
	*out++ = std::move(s.substr(a, s.length() - a));
	return out;
}

vector<string> splitCustom(string str, string token){
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index != (int)string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}


std::string parseDirNameTGDS(std::string dirName){
	int dirlen = strlen(dirName.c_str());
	if(dirlen > 2){
		if ((dirName.at(0) == '/') && (dirName.at(1) == '/')) {
			dirName.erase(0,1);	//trim the starting / if it has one
		}
		dirName.erase(dirName.length());	//trim the leading "/"
	}
	return dirName;
}

std::string parsefileNameTGDS(std::string fileName){
	int filelen = strlen(fileName.c_str());
	if(filelen > 4){
		if (fileName.at(0) == '/') {
			fileName.erase(0,1);	//trim the starting / if it has one
		}
		if ((fileName.at(2) == '/') && (fileName.at(3) == '/')) {
			fileName.erase(2,2);	//trim the starting // if it has one (since getfspath appends 0:/)
			if(fileName.at(2) != '/'){	//if we trimmed by accident the only leading / such as 0:filename instead of 0:/filename, restore it so it becomes the latter
				fileName.insert(2, ToStr('/') );
			}
		}
	}
	return fileName;
}

// check error cases, e.g. newPath = '..//' , '/home/user/' , 'subdir' (without trailing slash), etc... and return a clean, valid string in the form 'subdir/'
void getValidDir(std::string &dirName) {
    std::string slash = "/";
    size_t foundSlash = 0;
    while ( (foundSlash = dirName.find_first_of(slash),(foundSlash)) != std::string::npos) {
//        std::cout << " / @ " << foundSlash << std::endl;
        dirName.erase(foundSlash++,1); // Remove all slashs
    }
    dirName.append(slash); // Trailing slash is good and required, append it
}

// Returns the path to the current working dir starting from the server root dir
std::string getCurrentWorkingDir(bool showRootPath) {
	return string(getTGDSCurrentWorkingDirectory());
}

// Lists all files and directories in the specified directory and returns them in a string vector
std::vector<class FileDirEntry> browse(std::string dir, bool strict){
    std::vector<class FileDirEntry> fdirEnt;
	/*
	if (strict) {// When using strict mode, the function only allows one subdirectory and not several subdirectories, e.g. like sub/subsub/dir/ ...
        getValidDir(dir);
    }
    if (dir.compare("/") != 0) {
        dir = getCurrentWorkingDir(true).append(dir);
    } else {
        dir = getCurrentWorkingDir(true);
//        std::cout << "Yes" << std::endl;
    }
    */
	char fname[MAX_TGDSFILENAME_LENGTH];
	sprintf(fname,"%s",dir.c_str());
	
	//strcpy(TGDSCurrentWorkingDirectory, fname);	//update tgds cwd 
	if(chdir(fname) == 0){
		printf("change to dir %s OK",fname);
	}
	else{
		printf("change to dir %s ERROR",fname);
	}
	
	int curFileDirIndx = 0;
	int retf = FAT_FindFirstFile(fname);
	while(retf != FT_NONE){
		struct FileClass * fileClassInst = NULL;
		
		//directory?
		if(retf == FT_DIR){
			fileClassInst = getFileClassFromList(LastDirEntry);
			std::string newCurDirEntry = parseDirNameTGDS(std::string(fileClassInst->fd_namefullPath));
			//printf("dir: %s", newCurDirEntry.c_str());
			FileDirEntry fent = FileDirEntry(curFileDirIndx, newCurDirEntry, retf);
			fdirEnt.push_back(fent);
		}
		//file?
		else if(retf == FT_FILE){
			fileClassInst = getFileClassFromList(LastFileEntry);
			std::string newCurFileEntry = parsefileNameTGDS(std::string(fileClassInst->fd_namefullPath));
			//printf("file: %s", newCurFileEntry.c_str());
			FileDirEntry fent = FileDirEntry(curFileDirIndx, newCurFileEntry, retf);
			fdirEnt.push_back(fent);
		}
		
		//more file/dir objects?
		retf = FAT_FindNextFile(fname);
		curFileDirIndx++;
	}
	
	return fdirEnt;
}

char *getFtpCommandArg(char * theCommand, char *theCommandString, int skipArgs)
{
    char *toReturn = theCommandString + strlen(theCommand);

   /* Pass spaces */ 
    while (toReturn[0] == ' ')
    {
        toReturn += 1;
    }

    /* Skip eventual secondary arguments */
    if(skipArgs == 1)
    {
        if (toReturn[0] == '-')
        {
            while (toReturn[0] != ' ' &&
                   toReturn[0] != '\r' &&
                   toReturn[0] != '\n' &&
                   toReturn[0] != 0)
                {
                    toReturn += 1;
                }
        }

        /* Pass spaces */ 
        while (toReturn[0] == ' ')
        {
            toReturn += 1;
        }
    }

    return toReturn;
}