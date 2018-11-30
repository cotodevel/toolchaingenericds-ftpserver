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

extern void ftp_cmd_USER(int s, int cmd, char* arg);
extern void ftp_cmd_PASS(int s, int cmd, char* arg);
extern void ftp_cmd_PWD(int s, int cmd, char* arg);
extern void ftp_cmd_SYST(int s, int cmd, char* arg);
extern void ftp_cmd_FEAT(int s, int cmd, char* arg);
extern void ftp_cmd_CWD(int s, int cmd, char* arg);
extern void ftp_cmd_TYPE(int s, int cmd, char* arg);

#ifdef __cplusplus
}
#endif