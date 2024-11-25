/*
			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "ff.h"
#include "dldi.h"
#include "loader.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "dswnifi_lib.h"
#include "wifi_arm9.h"
#include "utilsTGDS.h"
#include "conf.h"
#include "timerTGDS.h"
#include "powerTGDS.h"
#include "biosTGDS.h"
#include "dmaTGDS.h"

// Includes
#include "WoopsiTemplate.h"

//TCP
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <in.h>
#include <string.h>
#include "socket2.h"

//ARM7 VRAM core
#include "arm7bootldr.h"
#include "arm7bootldr_twl.h"

u32 * getTGDSMBV3ARM7Bootloader(){
	if(__dsimode == false){
		return (u32*)&arm7bootldr[0];	
	}
	else{
		return (u32*)&arm7bootldr_twl[0];
	}
}

//Back to loader, based on Whitelisted DLDI names
static char curLoaderNameFromDldiString[MAX_TGDSFILENAME_LENGTH+1];
static inline char * canGoBackToLoader(){
	char * dldiName = dldi_tryingInterface();
	if(dldiName != NULL){
		if(strcmp(dldiName, "R4iDSN") == 0){	//R4iGold loader
			strcpy(curLoaderNameFromDldiString, "0:/_DS_MENU.dat");	//this allows to return to original payload code, and autoboot to TGDS-multiboot 
			return (char*)&curLoaderNameFromDldiString[0];
		}
		else if(strcmp(dldiName, "Ninjapass X9 (SD Card)") == 0){	//Ninjapass X9SD
			strcpy(curLoaderNameFromDldiString, "0:/loader.nds");
			return (char*)&curLoaderNameFromDldiString[0];
		}
		else{
			shutdownNDSHardware();
		}
	}
	return NULL;
}


//TGDS Soundstreaming API
int internalCodecType = SRC_NONE; //Returns current sound stream format: WAV, ADPCM or NONE
struct fd * _FileHandleVideo = NULL; 
struct fd * _FileHandleAudio = NULL;

void menuShow(){
	clrscr();
	printf("                              ");
	printf("                              ");
	printf("ToolchainGenericDS-WoopsiSDK stub:");
	printf("[%s] >%d ", TGDSPROJECTNAME, TGDSPrintfColor_Yellow);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool stopSoundStreamUser() {
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void closeSoundUser() {
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

char DSServerIP[32];
char renameFnameSource[256];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	//Save Stage 1: IWRAM ARM7 payload: NTR/TWL (0x03800000)
	memcpy((void *)TGDS_MB_V3_ARM7_STAGE1_ADDR, (const void *)0x02380000, (int)(96*1024));	//
	coherent_user_range_by_size((uint32)TGDS_MB_V3_ARM7_STAGE1_ADDR, (int)(96*1024)); //		also for TWL binaries 
	
	//Execute Stage 2: VRAM ARM7 payload: NTR/TWL (0x06000000)
	u32 * payload = NULL;
	if(__dsimode == false){
		payload = (u32*)&arm7bootldr[0];	
	}
	else{
		payload = (u32*)&arm7bootldr_twl[0];
	}
	executeARM7Payload((u32)0x02380000, 96*1024, payload);
	
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	//switch_dswnifi_mode(dswifi_idlemode); //breaks remoteboot, can't be enabled here
	
	printf("   ");
	printf("   ");
	
	int ret=FS_init();
	if (ret != 0){
		printf("%s: FS Init error: %d >%d", TGDSPROJECTNAME, ret, TGDSPrintfColor_Red);
		while(1==1){
			swiDelay(1);
		}
	}
	
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	REG_IME = 0;
	
	//MPUSet(); //seems to crash reloaded DKARM NTR homebrew if enabled
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	setupDisabledExceptionHandler();
	REG_IME = 1;
	
	//VBLANK can't be used to count up screen power timeout because sound stutters. Use timer instead
	TIMERXDATA(2) = TIMER_FREQ((int)1);
	TIMERXCNT(2) = TIMER_DIV_1 | TIMER_IRQ_REQ | TIMER_ENABLE;
	irqEnable(IRQ_TIMER2);

	//ARGV Implementation test
	if(getTGDSDebuggingState() == true){
		if (0 != argc ) {
			int i;
			for (i=0; i<argc; i++) {
				if (argv[i]) {
					printf("[%d] %s ", i, argv[i]);
				}
			}
		} 
		else {
			printf("No arguments passed!");
		}
	}
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	menuShow();
	
	powerOFF3DEngine(); //Power off ARM9 3D Engine to save power
	enableScreenPowerTimeout();
	bottomScreenIsLit = true;

	GUI.GBAMacroMode = true;	//GUI console at top screen
	TGDSLCDSwap();

	// Create Woopsi UI
	WoopsiTemplate WoopsiTemplateApp;
	WoopsiTemplateProc = &WoopsiTemplateApp;
	return WoopsiTemplateApp.main(argc, argv);
	
	while (1){
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
	return 0;
}

void enableScreenPowerTimeout(){
	REG_IE |= IRQ_TIMER2;
	setBacklight(POWMAN_BACKLIGHT_BOTTOM_BIT);
}

void disableScreenPowerTimeout(){
	REG_IE &= ~(IRQ_TIMER2);
	setBacklight(POWMAN_BACKLIGHT_BOTTOM_BIT);
}

bool bottomScreenIsLit = false;
static int secondsElapsed = 0;
void handleTurnOnTurnOffScreenTimeout(){
	secondsElapsed ++;
	if (  secondsElapsed == 12300 ){ //2728hz per unit @ 33Mhz
		setBacklight(0);
		secondsElapsed = 0;
	}
	//turn on bottom screen if input event
	if(bottomScreenIsLit == true){
		setBacklight(POWMAN_BACKLIGHT_BOTTOM_BIT);
		bottomScreenIsLit = false;
		secondsElapsed = 0;
	}
}


////////////////////////////////////////// FTP Server code ////////////////////////////////////////// 
char line[256];
char buffer[SENDRECVBUF_SIZE];
char bufferSend[SENDRECVBUF_SIZE];
char globalPath[256];

bool check(const char* aBuffer,const char* aCommand)
{
  return strncmp(aBuffer,aCommand,strlen(aCommand))==0;
}

bool SendCallback(void *aParam)
{
  TCallbackData* data=(TCallbackData*)aParam;
  u32 controlSize=data->iConn->Receive(bufferSend,1023,false);
  bufferSend[controlSize]=0;
  if(controlSize)
  {
    if(check(bufferSend,"NOOP"))
    {
      data->iConn->Send("200 OK\r\n");
    }
    else //all other commands interpreted as ABOR.
    {
      data->iAbor=true;
      return false;
    }
  }
  return true;
}



#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void process(CSocket2* aConn)
{
  aConn->Send("220 Cthulhu ready\r\n");
  char address[44]="";
  unsigned short port=0;
  u32 restore=0;
  bool passive=false;
  CSocket2* passiveConn=new CSocket2(false);
  passiveConn->Bind(4000);
  passiveConn->Listen();
  while(true)
  {
    int size=aConn->Receive(buffer,SENDRECVBUF_SIZE,true);
    buffer[size]=0;
    //printf(buffer);
    if(size==0) break;
    if(!check(buffer,"RETR"))
    {
      restore=0;
    }
    if(check(buffer,"USER"))
    {
      aConn->Send("331 login accepted\r\n");
    }
    else if(check(buffer,"PASS"))
    {
		//default directory everytime an user logs in. Otherwise malformed directories happen when retrying an active session 
		memset(globalPath, 0, sizeof(globalPath));
		strcpy(globalPath,"/");
		
      aConn->Send("230 logged in\r\n");
    }
    else if(check(buffer,"SYST"))
    {
      aConn->Send("215 UNIX Type: L8\r\n");
    }
    else if(check(buffer,"PWD")||check(buffer,"XPWD"))
    {
	  sprintf(line, "257 \"%s\" \n", globalPath);
      aConn->Send(line);
    }
	
	//TYPE: I binary data by default
    else if(check(buffer,"TYPE"))
    {
      aConn->Send("200 Switching to Binary mode.\r\n");
    }
    else if(check(buffer,"PORT"))
    {
      u32 h1,h2,h3,h4,p1,p2;
      sscanf(buffer+5,"%3u,%3u,%3u,%3u,%3u,%3u",&h1,&h2,&h3,&h4,&p1,&p2);
      sprintf(address,"%u.%u.%u.%u",h1,h2,h3,h4);
      port=((p1&0xff)<<8)|(p2&0xff);
      aConn->Send("200 PORT command successful\r\n");
    }
    else if(check(buffer,"LIST"))
    {
		aConn->SetNonBlock(false); //Set client's command socket blocking now, otherwise file connection closes too early.
		
      	aConn->Send("150 Opening connection\r\n");
      	CSocket2* list=NULL;
      	if(passive){
        	list=passiveConn->Accept(true);
      	}
      	else{
        	list=new CSocket2(false);
        	list->Connect(address,port);
      	}
		list->SetNonBlock(false); //Set client's data socket blocking now, otherwise file connection closes too early.
		
	  	int fSizeDir = 0;
		char * curPath = (char *)&globalPath[0];
		char fnameDir[256];
		strcpy(fnameDir, "0:");
		strcat(fnameDir, (char *)curPath);
		strcat(fnameDir, (char *)"/");
		
		FRESULT result;
		DIR dir;
		FILINFO fno;
		if(f_chdir((const TCHAR*)curPath) != FR_OK){	//Very important before listing files
			//If malformed directory by Client, reset to default directory
			memset(globalPath, 0, sizeof(globalPath));
			strcpy(globalPath,"/");
			f_chdir((const TCHAR*)globalPath);
		}
		
		result = f_opendir(&dir, (const TCHAR*)curPath);                       /* Open the directory */
		if (result == FR_OK) {
			for(;;){
				result = f_readdir(&dir, &fno);                   /* Read a directory item */
				int type = 0;
				if (fno.fattrib & AM_DIR) {			           /* It is a directory */
					type = FT_DIR;	
				}
				else if (									   /* It is a file */
				(fno.fattrib & AM_RDO)
				||
				(fno.fattrib & AM_HID)
				||
				(fno.fattrib & AM_SYS)
				||
				(fno.fattrib & AM_ARC)
				){
					type = FT_FILE;			
				}
				else{	/* It is Invalid. */
					type = FT_NONE;
				}
				
				if (result != FR_OK || fno.fname[0] == 0){	//Error or end of dir. No need to handle the error here.
					break;
				}
				else if(type == FT_DIR){
					fSizeDir = strlen((char*)fno.fname[0]);
					sprintf(line, "drw-r--r-- 1 DS group          %d Feb  1  2009 %s \r\n", fSizeDir, &fno.fname[0]);
					list->Send(line);
				}
				else if(type == FT_FILE){
					char fnameFull[256];
					strcpy(fnameFull, fnameDir);
					strcat(fnameFull, (char*)&fno.fname[0]);
					int filesizeList = FS_getFileSize(fnameFull);
					char * filenameList = &fno.fname[0];
					sprintf(line,"-rw-r--r--    1 ftp      ftp    %d Jan 01  2009 %s\r\n", filesizeList, filenameList);
					list->Send(line);
				}
			}
			f_closedir(&dir);
		}
		else{
			sprintf(scrollingBoxLoggerOutput, "debug: f_opendir fail: (%d)", result);
WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));

		}
		
		//Close LIST command properly on the (secondary, active) data socket (client) 
		u8 endByte=0x0;
		send(list->iSocket, &endByte, 1, 0);	

    	delete list;
    	aConn->Send("226 Transfer Complete\r\n");
	}
    else if(check(buffer,"RETR /\r\n"))
    {
      aConn->Send("550 File not found\r\n");
    }
    else if(check(buffer,"RETR"))
    {
      aConn->Send("150 Opening BINARY mode data connection\r\n");
      CSocket2* data=NULL;
      if(passive)
      {
        data=passiveConn->Accept(true);
	  }
      else
      {
        data=new CSocket2(false);
        data->Connect(address,port);
      }
	  data->SetNonBlock(false); //Set data socket blocking now, otherwise file connection closes too early.

		char * fname = getFtpCommandArg("RETR", buffer, 0); 
		char * curPath = (char *)&globalPath[0];
		char fnameFull[256];
		strcpy(fnameFull, "0:");
		strcat(fnameFull, (char *)curPath);
		strcat(fnameFull, (char *)"/");
		strcat(fnameFull, (char *)fname);
		
		int total_len = FS_getFileSize((char*)fnameFull);
		FILE * fh = fopen(fnameFull, "r");
		if(fh != NULL){
			//retrieve file
			int written = send_file(data->iSocket, fh, total_len);
			fclose(fh);
			aConn->Send("226 Transfer complete.\r\n");
		}
		//could not open file
		else{
			aConn->Send("451 Could not open file.\r\n");
		}
		
      delete data;
	  aConn->Send("226 Transfer Complete\r\n");
    }
    else if(check(buffer,"QUIT"))
    {
      aConn->Send("221 goodbye\r\n");
      break;
    }
    else if(check(buffer,"REST"))
    {
      sscanf(buffer+5,"%u",&restore);
      sprintf(line,"350 Restarting at %u. Send RETR to initiate transfer\r\n",restore);
      aConn->Send(line);
    }
    else if(check(buffer,"PASV"))
    {
      sprintf(line,"227 Entering Passive Mode (%s,15,160)\r\n",(char*)&DSServerIP);
      size_t lineLen=strlen(line);
      for(size_t ii=0;ii<lineLen;++ii) if(line[ii]=='.') line[ii]=',';
      aConn->Send(line);
      passive=true;
    }
    else if(check(buffer,"NOOP"))
    {
      aConn->Send("200 OK\r\n");
    }
    else if(check(buffer,"CWD"))
    {
		char * baseDir = getFtpCommandArg("CWD", buffer, 0);
		
		if(strncmpi((char*)baseDir, "..", 2) == 0){
			leaveDir((char*)&globalPath[0]);
		}
		else{
			
			//CWD adds a " " whitespace at the end. (which makes the string count that one)
			//remove it
			baseDir[strlen(baseDir)-2] = '\0';

			char outDir[256];
			trimStr((char*)baseDir);
			strcpy(outDir, baseDir);
			
			if(strlen((char*)&globalPath[0]) == 1){
				strcat((char*)&globalPath[0], outDir);
			}

			else{
				strcat((char*)&globalPath[0], "/");
				strcat((char*)&globalPath[0], outDir);
			}
			
			//trim whitespaces
			trimStr((char*)&globalPath[0]);
		}

      aConn->Send("250 CWD command successful\r\n");
    }
	else if(check(buffer,"STOR"))
    {
      char * fileReceived = getFtpCommandArg("STOR", buffer, 0);

		char * curPath = (char *)&globalPath[0];
		char fnameFull[256];
		strcpy(fnameFull, "0:");
		strcat(fnameFull, (char *)curPath);
		strcat(fnameFull, (char *)"/");
		strcat(fnameFull, (char *)fileReceived);
	
	  aConn->Send("150 Opening BINARY mode data connection\r\n");
      CSocket2* data=NULL;
      if(passive)
      {
        data=passiveConn->Accept(true);
	  }
      else
      {
        data=new CSocket2(false);
        data->Connect(address,port);
      }
	  data->SetNonBlock(false); //Set data socket blocking now, otherwise file connection closes too early.

		int total_len = FS_getFileSize((char*)fnameFull);
		FILE * fh = fopen(fnameFull, "w+");
		if(fh != NULL){
			
			//Store file from remote client socket.
			char * client_reply = (char*)TGDSARM9Malloc(SENDRECVBUF_SIZE);
			int received_len = 0;
			int total_len = 0;
			while( ( received_len = recv(data->iSocket, client_reply, SENDRECVBUF_SIZE, 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
				if(received_len>0) { // data was received!
					total_len += received_len;
					fwrite(client_reply , 1, received_len , fh);
					fsync(fileno(fh));	//Make incoming TGDS FS file coherent
				}
				swiDelay(1);
			}
			TGDSARM9Free(client_reply);
			fclose(fh);

			aConn->Send("226 Transfer complete.\r\n");
		}
		//could not open file
		else{
			aConn->Send("451 Could not open file.\r\n");
		}
		
      delete data;
    }

	else if(check(buffer,"DELE"))
    {
		char * fnameToDelete = getFtpCommandArg("DELE", buffer, 0);
		char * curPath = (char *)&globalPath[0];
		char fnameFull[256];
		strcpy(fnameFull, "0:");
		strcat(fnameFull, (char *)fnameToDelete);
		int retVal = remove(fnameFull);	//TGDS: even if f_unlink works, it won't notify properly here.
		//if(retVal==0){
			aConn->Send("250 Delete Command successful.\r\n");
		//}
		//else{
		//	aConn->Send("450 Delete Command error.\r\n");
		//}
	}
	
	//Rename commands
	else if(check(buffer,"RNFR"))
    {      
		aConn->SetNonBlock(false); //Set data socket blocking now, otherwise file connection closes too early.

		char * originalFileExist = getFtpCommandArg("RNFR", buffer, 0);
		char fnameFull[256];
		strcpy(fnameFull, "0:");
		strcat(fnameFull, (char *)originalFileExist);
		FILE * fh = fopen(fnameFull, "r");
		if(fh != NULL){
			fclose(fh);
			strcpy(renameFnameSource, fnameFull);
			aConn->Send("350 OK\r\n");
		}
		else{
			aConn->Send("450 File missing.\r\n");
		}
    }
	
	else if(check(buffer,"RNTO"))
    {
		char * targetFileToRename = getFtpCommandArg("RNTO", buffer, 0);
		char fnameFull[256];
		strcpy(fnameFull, "0:");
		strcat(fnameFull, (char *)targetFileToRename);

		int retVal = rename(renameFnameSource, fnameFull);
		//if(retVal == 0){ //TGDS: even if f_rename works, it won't notify properly here.
			aConn->Send("250 OK\r\n");
		//}
		//else{
		//	aConn->Send("550 Rename failure\r\n");
		//}
    }

	else if(check(buffer,"MKD"))
    {
		char * targetDirToCreate = getFtpCommandArg("MKD", buffer, 0);
		char dirNameFull[256];
		strcpy(dirNameFull, "/");
		strcat(dirNameFull, (char *)targetDirToCreate);

		FRESULT result = f_mkdir((const TCHAR*)dirNameFull);
		//if (result == FR_OK){	//TGDS: even if f_mkdir works, it won't notify properly here.
			aConn->Send("250 OK\r\n");
		//}
		//else{
		//	aConn->Send("550 Create directory failure\r\n");
		//}
    }

	else if( check(buffer,"RMD") || check(buffer,"XRMD") )
    {
		char * targetDirToRemove = getFtpCommandArg("RMD", buffer, 0);
		memset(dirNameFull, 0, sizeof(dirNameFull));
		strcpy(dirNameFull, "0:");
		strcat(dirNameFull, (char *)&targetDirToRemove[0]);
		*(char*)&dirNameFull[strlen(dirNameFull)] = '\0';
		trimStr((char*)&dirNameFull[0]);

		f_chdir("/"); //change to root path, otherwise fatfs doesn't allow deleting from same directory
		
		/* Delete the directory */
		FILINFO fno;
		FRESULT fr = delete_node(dirNameFull, sizeof(dirNameFull) / sizeof(dirNameFull[0]), &fno);
		char * curPath = (char *)&globalPath[0];
		f_chdir(curPath); //restore original path

		/* Check the result */
		//if (fr != FR_OK) { //TGDS: even if delete_node works, it won't notify properly here.
		//	aConn->Send("550 Remove directory failure\r\n");
		//} else {
			aConn->Send("250 OK\r\n");
		//}
    }

    else
    {
      aConn->Send("500 command not recognized\r\n");
    }
  }
  delete passiveConn;
}
