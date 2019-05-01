#ifndef UTIL_NDS
#define UTIL_NDS

#define BUFFERSIZE (int)(512)

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#define FTP_SERVER_IDLE (uint32)(0xffff1010)				//idle -> connect FTP Client (FTP_SERVER_CONNECTING_PHASE1)
#define FTP_SERVER_CONNECTING (uint32)(0xffff1011)	//FTP Server <--> FTP Client Initial Handshake (FTP_SERVER_CONNECTING_PHASE2)
#define FTP_SERVER_CONNECTED_IDLE (uint32)(0xffff1012)	//FTP Server <--> FTP Client Initial Handshake (FTP_SERVER_CONNECTING_PHASE3)
#define FTP_SERVER_WORKING (uint32)(0xffff1013)	//FTP Server <--> FTP Client Actual Session. If something fails it will disconnect here.

//error codes
#define FTP_SERVER_PROC_RUNNING (sint32)(0)
#define FTP_SERVER_PROC_FAILED (sint32)(-1)

#define FTP_PASV_DATA_TRANSFER_PORT (sint32)(1024)



#endif

#ifdef __cplusplus
extern "C" {
#endif

extern uint32 CurFTPState;
extern u32 getFTPState();
extern void setFTPState(uint32 FTPState);

int ftpResponseSender(int s, int n, char* mes);
extern const char * getpwd(const char *cwd);

#ifdef __cplusplus
}
#endif