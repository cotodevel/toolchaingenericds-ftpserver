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

#include "socket.h"
#include "in.h"
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "ftpMisc.h"
#include "main.h"
#include "InterruptsARMCores_h.h"
#include "specific_shared.h"
#include "ff.h"
#include "memoryHandleTGDS.h"
#include "fileHandleTGDS.h"
#include "reent.h"
#include "sys/types.h"
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "consoleTGDS.h"
#include "utilsTGDS.h"

#include "devoptab_devices.h"
#include "fsfatlayerTGDS.h"
#include "usrsettingsTGDS.h"
#include "exceptionTGDS.h"

#include "videoTGDS.h"
#include "keypadTGDS.h"
#include "dldi.h"
#include "SpecialFunctions.h"
#include "dmaTGDS.h"
#include "biosTGDS.h"
#include "nds_cp15_misc.h"
#include "notifierProcessor.h"
#include "limitsTGDS.h"
#include "ftpServer.h"
#include "wifi_arm9.h"
#include "dswnifi_lib.h"

void menuShow(){
	clrscr();
	printf("                              ");
	printf("A: start FTP server. An IP and Port will be drawn shortly.");
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	/*			TGDS 1.5 Standard ARM9 Init code end	*/
	
	//custom Handler
	menuShow();
	setFTPState(FTP_SERVER_IDLE);
	
	while (1){
		sint32 FTP_STATUS = do_ftp_server();
		if(FTP_STATUS == FTP_SERVER_PROC_RUNNING){
			//Server Running
		}
		else if(FTP_STATUS == FTP_SERVER_CLIENT_DISCONNECTED){
			//Server Disconnected/Idle!
			closeFTPDataPort(sock1);
			setFTPState(FTP_SERVER_IDLE);
			printf("Client disconnected!. Press A to retry.");
			switch_dswnifi_mode(dswifi_idlemode);
			scanKeys();
			while(!(keysPressed() & KEY_A)){
				scanKeys();
				IRQVBlankWait();
			}
			main(0, (sint8**)"");
			
		}
	}
}
