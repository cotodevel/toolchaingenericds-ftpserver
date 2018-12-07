// DUE TO THE FACT THAT BUFFER LIMITS IN KERNEL FOR THE SOCKET MAY BE 
//   REACHED, IT IS POSSIBLE THAT READ AND WRITE MAY RETURN A POSITIVE VALUE
//   LESS THAN THE NUMBER REQUESTED. HENCE WE CALL THE TWO PROCEDURES
//   BELOW TO TAKE CARE OF SUCH EXIGENCIES 

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"

#include <stdio.h>
#include "util.h"

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

uint32 CurFTPState = 0;
u32 getFTPState(){
	return CurFTPState;
}

void setFTPState(uint32 FTPState){
	CurFTPState = FTPState;
} 

int recv_all(int sockfd, void *buf, size_t len, int flags)
{
    size_t toread = len;
    char  *bufptr = (char*) buf;
    while (toread > 0)
    {
        ssize_t rsz = recv(sockfd, bufptr, toread, flags);
        if (rsz <= 0){
            return rsz;  /* Error or other end closed cnnection */
		}
        toread -= rsz;  /* Read less next time */
        bufptr += rsz;  /* Next buffer position to read into */
    }

    return len;
}


bool send_all(int socket, void *buffer, size_t length)
{
    char *ptr = (char*) buffer;
    while (length > 0)
    {
        int i = send(socket, ptr, length);
        if (i < 1) return false;
        ptr += i;
        length -= i;
    }
    return true;
}


int ftpResponseSender(int s, int n, char* mes)
{
	char data[128];
	sprintf(data, "%d %s ", n, mes);
	return send_all(s, data, strlen(data));
}


//FTP CMDS

const char * getpwd(const char *cwd)
{
	const char *pwd;
	struct stat cst, pst;

	if (!(pwd = getenv("PWD")) || pwd[0] != '/' || stat(pwd, &pst) < 0)
		return cwd;
	if (stat(cwd, &cst) < 0){
		//printf("stat %s:", cwd);
	}
	return (pst.st_dev == cst.st_dev && pst.st_ino == cst.st_ino) ? pwd : cwd;
}