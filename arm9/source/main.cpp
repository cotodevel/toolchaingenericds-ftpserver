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

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "ftpMisc.h"
#include "dswnifi_lib.h"
#include "ftpServer.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"

static bool setApp = false;

void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("Available heap memory: %d", getMaxRam());
	printf("Starting FTP server. An IP and Port will be drawn shortly.");
	printarm7DebugBuffer();
}

int main(int argc, char argv[argvItems][MAX_TGDSFILENAME_LENGTH]) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	if(setApp == false){
		bool isCustomTGDSMalloc = true;
		setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	
		sint32 fwlanguage = (sint32)getLanguage();
		#ifdef ARM7_DLDI
		setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
		#endif
		int ret=FS_init();
		if (ret == 0)
		{
			printf("FS Init ok.");
		}
		else if(ret == -1)
		{
			printf("FS Init error.");
		}
		
		setApp = true;	
	}
	
	switch_dswnifi_mode(dswifi_idlemode);
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Remove logo and restore Main Engine registers
	//restoreFBModeMainEngine();
	
	menuShow();
	ftpInit();
	
	while (1){
		sint32 FTP_SERVER_STATUS = FTPServerService();
		switch(FTP_SERVER_STATUS){
			//Server Running
			case(FTP_SERVER_ACTIVE):{
				
			}
			break;
			
			//Server Disconnected/Idle!
			case(FTP_SERVER_CLIENT_DISCONNECTED):{				
				closeFTPDataPort(sock1);
				setFTPState(FTP_SERVER_IDLE);
				printf("Client disconnected!. Press A to retry.");
				switch_dswnifi_mode(dswifi_idlemode);
				scanKeys();
				while(!(keysPressed() & KEY_A)){
					scanKeys();
					IRQVBlankWait();
				}
				main(argc, argv);
			}
			break;
		}
		
	}
}
