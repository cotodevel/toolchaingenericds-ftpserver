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


int readn(int sd,char *ptr,int size)
{         int no_left,no_read;
          no_left = size;
          while (no_left > 0) 
                     { no_read = read(sd,ptr,no_left);
                       if(no_read <0)  return(no_read);
                       if (no_read == 0) break;
                       no_left -= no_read;
                       ptr += no_read;
                     }
          return(size - no_left);
}


int writen(int sd,char *ptr,int size)
{         int no_left,no_written;
          no_left = size;
          while (no_left > 0) 
                     { no_written = write(sd,ptr,no_left);
                       if(no_written <=0)  return(no_written);
                       no_left -= no_written;
                       ptr += no_written;
                     }
          return(size - no_left);
}



int ftpResponseSender(int s, int n, char* mes)
{
	char data[128];
	sprintf(data, "%d %s\r\n", n, mes);
	return send(s,data,strlen(data),0);
}




//FTP CMDS

const char * getpwd(const char *cwd)
{
	const char *pwd;
	struct stat cst, pst;

	if (!(pwd = getenv("PWD")) || pwd[0] != '/' || stat(pwd, &pst) < 0)
		return cwd;
	if (stat(cwd, &cst) < 0){
		//iprintf("stat %s:", cwd);
	}
	return (pst.st_dev == cst.st_dev && pst.st_ino == cst.st_ino) ? pwd : cwd;
}