#ifndef __ftp_misc_h__
#define __ftp_misc_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include <dswifi9.h>
#include <netdb.h>

//FTP server
#include "dswnifi_lib.h"

#include <socket.h>
#include <in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus

//C++ part
using namespace std;
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>

class FileDirEntry
{
  public:
    int Index;
	std::string filePathFilename;
    int type;	//FT_DIR / FT_FILE / FT_NONE	//  setup on Constructor / updated by getFileFILINFOfromPath(); / must be init from the outside 
    // Constructor
    FileDirEntry(int indexInst, std::string filePathFilenameInst, int typeInst)
	{
		Index = indexInst;
		filePathFilename = filePathFilenameInst;
		type = typeInst;
	}
	
	//helpers if/when Constructor is not available
	int getIndex()
    {
		return Index;
    }
    std::string getfilePathFilename()
    {
		return filePathFilename;
    }
	int gettype()
    {
		return type;
    }
	
	void setIndex(int IndexInst){
		Index = IndexInst;
	}
	void setfilename(std::string filePathFilenameInst){
		filePathFilename = filePathFilenameInst;
	}
	void settype(int typeInst){
		type = typeInst;
	}
};
#endif

#define FTP_SERVER_IDLE (uint32)(0xffff1010)				//idle -> connect FTP Client (FTP_SERVER_CONNECTING_PHASE1)
#define FTP_SERVER_CONNECTING (uint32)(0xffff1011)	//FTP Server <--> FTP Client Initial Handshake (FTP_SERVER_CONNECTING_PHASE2)
#define FTP_SERVER_CONNECTED_IDLE (uint32)(0xffff1012)	//FTP Server <--> FTP Client Initial Handshake (FTP_SERVER_CONNECTING_PHASE3)
#define FTP_SERVER_WORKING (uint32)(0xffff1013)	//FTP Server <--> FTP Client Actual Session. If something fails it will disconnect here.

#define FTP_SERVER_CLIENT_DISCONNECTED (sint32)(0)
#define FTP_SERVER_PROC_RUNNING (sint32)(1)
#define FTP_SERVER_PROC_FAILED (sint32)(2)

#define BUF_SIZE (int)4096

#define LISTPATH_SIZE (int)(64*1024)

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int ftp_cmd_USER(int s, int cmd, char* arg);
extern int ftp_cmd_PASS(int s, int cmd, char* arg);
extern int ftp_cmd_PWD(int s, int cmd, char* arg);
extern int ftp_cmd_SYST(int s, int cmd, char* arg);
extern int ftp_cmd_FEAT(int s, int cmd, char* arg);
extern int ftp_cmd_TYPE(int s, int cmd, char* arg);
extern int ftp_cmd_PASV(int s, int cmd, char* arg);
extern int ftp_cmd_STOR(int s, int cmd, char* arg);
extern int ftp_cmd_RETR(int s, int cmd, char* arg);
extern int ftp_cmd_CDUP(int s, int cmd, char* arg);

extern uint32 CurFTPState;
extern u32 getFTPState();
extern void setFTPState(uint32 FTPState);
extern int ftpResponseSender(int s, int n, char* mes);

//These two open/close a new FTP Server Data Port (Passive Mode)
extern int openAndListenFTPDataPort(struct sockaddr_in * sain);
extern void closeFTPDataPort(int sock);
extern int currserverDataListenerSock;
extern int send_file(int peer, FILE *f, int fileSize);

extern char CWDFTP[512];
extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
extern bool ShowBrowser(char * Path);

extern char * ListPathPrint;
extern char * buildList();	//C++ <- C

#ifdef __cplusplus
extern std::list<std::string> completePath;
extern void getValidDir(std::string &dirName);
extern std::string getCurrentWorkingDir(bool showRootPath);
extern std::vector<class FileDirEntry> browse(std::string dir, bool strict);
extern std::string getDldiDefaultPath();
extern std::string parseDirNameTGDS(std::string dirName);
extern std::string parsefileNameTGDS(std::string fileName);

#endif

extern char *getFtpCommandArg(char * theCommand, char *theCommandString, int skipArgs);

#ifdef __cplusplus
}
#endif