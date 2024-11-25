/*
    socket2.cpp
    Copyright (C) 2010 yellow wood goblin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

//TCP
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <in.h>
#include <string.h>
#include "socket2.h"
#include "biosTGDS.h"

// Includes
#include "WoopsiTemplate.h"

void CSocket2::Panic(void)
{
  while(true) HaltUntilIRQ(); //Save power until next Vblank
}

CSocket2::CSocket2(bool aNonBlock): iSocket(-1)
{
  iSocket=socket(AF_INET,SOCK_STREAM,0);
  if(iSocket<0)
  {
	sprintf(scrollingBoxLoggerOutput, "socket error: %d", errno);
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
  SetNonBlock(aNonBlock);
}

CSocket2::CSocket2(int aSocket,bool aNonBlock): iSocket(aSocket)
{
  SetNonBlock(aNonBlock);
}

CSocket2::~CSocket2()
{
  if(shutdown(iSocket,0)<0)
  {
	sprintf(scrollingBoxLoggerOutput, "shutdown error: %d", errno);
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
	Panic();
  }
  for(size_t ii=0;ii<60;++ii) IRQVBlankWait();
  if(closesocket(iSocket)<0)
  {
	sprintf(scrollingBoxLoggerOutput, "closesocket error %s", strerror(errno));
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
}

void CSocket2::Bind(unsigned short aPort)
{
  sockaddr_in sa;
  memset(&sa,0,sizeof(sa));
  sa.sin_family=AF_INET;
  sa.sin_addr.s_addr=htonl(INADDR_ANY);
  sa.sin_port=htons(aPort);
  if(bind(iSocket,(const sockaddr*)&sa,sizeof(sa))<0)
  {
	sprintf(scrollingBoxLoggerOutput, "bind error");
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
}

void CSocket2::Connect(const char* anAddress,unsigned short aPort)
{
  sockaddr_in sa;
  memset(&sa,0,sizeof(sa));
  sa.sin_family=AF_INET;
  if(!inet_aton(anAddress,&(sa.sin_addr)))
  {
	sprintf(scrollingBoxLoggerOutput, "inet_aton error: %d",errno);
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
  sa.sin_port=htons(aPort);
  if(connect(iSocket,(const sockaddr*)&sa,sizeof(sa))<0)
  {
	sprintf(scrollingBoxLoggerOutput, "connect error: %d",errno);
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
}

void CSocket2::Listen(void)
{
  if(listen(iSocket,1)<0)
  {
	sprintf(scrollingBoxLoggerOutput, "listen error");
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
}

CSocket2* CSocket2::Accept(bool aWait)
{
  sockaddr_in clientAddr;
  int clientAddrLen=sizeof(clientAddr);
  int newSocket=-1;
  while(true)
  {
    newSocket=accept(iSocket,(sockaddr*)&clientAddr,&clientAddrLen);
    if(newSocket<0)
    {
      if(errno==EAGAIN)
      {
        if(aWait)
        {
          IRQVBlankWait();
          continue;
        }
        else return NULL;
      }
	  sprintf(scrollingBoxLoggerOutput, "accept error: %d, %d", newSocket, errno);
	  WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
	  Panic();
    }
    else break;
  }
  CSocket2* result=new CSocket2(newSocket,aWait);
  return result;
}

int CSocket2::Receive(char* aBuffer,int aSize,bool aWait)
{
  int lenb;
  while(true)
  {
    lenb=recv(iSocket,aBuffer,aSize,0);
    if(lenb<0)
    {
      if(errno==EAGAIN)
      {
        if(aWait)
        {
          IRQVBlankWait();
        }
        else
        {
          lenb=0;
          break;
        }
        continue;
      }
      if(errno==ECONNRESET)
      {
        lenb=0;
        break;
      }
	  sprintf(scrollingBoxLoggerOutput, "recv error: %d, %d",lenb,errno);
	  WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
      Panic();
    }
    else break;
  }
  return lenb;
}

int CSocket2::Send(const char* aBuffer)
{
  return Send(aBuffer,strlen(aBuffer),NULL,NULL);
}

int CSocket2::Send(const char* aBuffer,int aSize,TSendCallback aCallBack,void *aParam)
{
  int total=0;
  do
  {
    int sended;
    while(true)
    {
      sended=send(iSocket,aBuffer+total,aSize-total,0);
      if(sended<0)
      {
        if(aCallBack)
        {
          if(!aCallBack(aParam)) return -1;
        }
        if(errno==EAGAIN)
        {
          continue;
        }
		sprintf(scrollingBoxLoggerOutput, "send error: %d, %d",sended,errno);
		WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
        Panic();
      }
      else break;
    }
    if(sended==0) break;
    total+=sended;
  } while(total<aSize);
  return total;
}

void CSocket2::SetNonBlock(bool aNonBlock)
{
  iNonBlock=aNonBlock;
  unsigned long arg=iNonBlock?1:0;
  if(ioctl(iSocket,FIONBIO,&arg))
  {
    closesocket(iSocket);
	sprintf(scrollingBoxLoggerOutput, "ioctl error");
	WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
    Panic();
  }
}


#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
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
	
	//is it a directory / file? If so, prevent "/0:" bad file/dir parsing from the FTP Client
	if(
		(toReturn[0] == '/') &&
		(toReturn[1] == '0') &&
		(toReturn[2] == ':')
	){
		toReturn += 1;
	}
	
    return toReturn;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool send_all(int socket, void *buffer, size_t length, int * written) 
{
    char *ptr = (char*) buffer;
    while (length > 0)
    {
        int i = send(socket, ptr, length, 0);
		swiDelay(100);
		if (i < 1) return false;
		ptr += i;
		(*written)+=i;
        length -= i;
    }
    return true;
}


#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int send_file(int peer, FILE *f, int fileSize) {
	char * filebuf = (char*)TGDSARM9Malloc(SENDRECVBUF_SIZE);
    int written = 0;
	int readSofar= 0;
	int lastwritten = 0;
    while((readSofar=fread(filebuf, 1, SENDRECVBUF_SIZE, f)) > 0) {
		int write = 0;
		if(send_all(peer, filebuf, readSofar, &write) == true){
			written+=write;
			lastwritten = write;
		}
		else{
			//failed to write %d bytes
		}
    }
	TGDSARM9Free(filebuf);
	
    return written;
}