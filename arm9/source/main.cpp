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
#include "TGDSNDSLogo.h"

void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("Starting FTP server. An IP and Port will be drawn shortly.");
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = true;	//set default console or custom console: custom console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
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
	
	//show TGDS logo
	initFBModeSubEngine0x06200000();
	renderFBMode3SubEngine((u16*)&TGDSLogoNDSSize[0], (int)TGDSLOGONDSSIZE_WIDTH,(int)TGDSLOGONDSSIZE_HEIGHT);
	
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
				main(0, (sint8**)"");
			}
			break;
		}
	}
}
