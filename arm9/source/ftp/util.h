#ifndef UTIL_NDS
#define UTIL_NDS

#define BUFFERSIZE 512

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int writen(int sd,char *ptr,int size);
extern int readn(int sd,char *ptr,int size);
extern void wy_fileName_collector(char *_buffer, char  *_nameBuffer);

int ftpResponseSender(int s, int n, char* mes);
extern const char * getpwd(const char *cwd);

#ifdef __cplusplus
}
#endif