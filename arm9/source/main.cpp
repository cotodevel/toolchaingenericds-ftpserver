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
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dswnifi_lib.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "click_raw.h"
#include "ima_adpcm.h"
#include "spitscTGDS.h"
#include "loader.h"

// Includes
#include "WoopsiTemplate.h"

//ARM7 VRAM core
#include "arm7vram.h"
#include "arm7vram_twl.h"

u32 * getTGDSMBV3ARM7Bootloader(){
	if(__dsimode == false){
		return (u32*)&arm7vram[0];	
	}
	else{
		return (u32*)&arm7vram_twl[0];
	}
}

//TGDS Soundstreaming API
int internalCodecType = SRC_NONE; //Returns current sound stream format: WAV, ADPCM or NONE
struct fd * _FileHandleVideo = NULL; 
struct fd * _FileHandleAudio = NULL;

bool stopSoundStreamUser(){
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

static inline void menuShow(){
	clrscr();
	printf("toolchaingenericds-FTPServer: ");
	printf("(Select): This menu. ");
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
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
		payload = (u32*)&arm7vram[0];	
	}
	else{
		payload = (u32*)&arm7vram_twl[0];
	}
	executeARM7Payload((u32)0x02380000, 96*1024, payload);
	
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	int ret=FS_init();
	if (ret == 0)
	{
		
	}
	else{
		
	}
	
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	/////////////////////////////////////////////////////////Reload TGDS Proj///////////////////////////////////////////////////////////
	char tmpName[256];
	char ext[256];
	if(__dsimode == true){
		char TGDSProj[256];
		char curChosenBrowseFile[256];
		strcpy(TGDSProj,"0:/");
		strcat(TGDSProj, "ToolchainGenericDS-multiboot");
		if(__dsimode == true){
			strcat(TGDSProj, ".srl");
		}
		else{
			strcat(TGDSProj, ".nds");
		}
		//Force ARM7 reload once 
		if( 
			(argc < 2) 
			&& 
			(strncmp(argv[1], TGDSProj, strlen(TGDSProj)) != 0) 	
		){
			REG_IME = 0;
			MPUSet();
			REG_IME = 1;
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			strcpy(curChosenBrowseFile, TGDSProj);
			
			char thisTGDSProject[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(thisTGDSProject, "0:/");
			strcat(thisTGDSProject, TGDSPROJECTNAME);
			if(__dsimode == true){
				strcat(thisTGDSProject, ".srl");
			}
			else{
				strcat(thisTGDSProject, ".nds");
			}
			
			//Boot .NDS file! (homebrew only)
			strcpy(tmpName, curChosenBrowseFile);
			separateExtension(tmpName, ext);
			strlwr(ext);
			
			//pass incoming launcher's ARGV0
			char arg0[256];
			int newArgc = 2;
			if (argc > 2) {
				//Arg0:	Chainload caller: TGDS-MB
				//Arg1:	This NDS Binary reloaded through ChainLoad
				//Arg2: This NDS Binary reloaded through ChainLoad's Argument0
				strcpy(arg0, (const char *)argv[2]);
				newArgc++;
			}
			
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], curChosenBrowseFile);	//Arg0:	Chainload caller: TGDS-MB
			strcpy(&thisArgv[1][0], thisTGDSProject);	//Arg1:	NDS Binary reloaded through ChainLoad
			strcpy(&thisArgv[2][0], (char*)arg0);	//Arg2: NDS Binary reloaded through ChainLoad's ARG0
			u32 * payload = getTGDSMBV3ARM7Bootloader();
			if(TGDSMultibootRunNDSPayload(curChosenBrowseFile, (u8*)payload, newArgc, (char*)&thisArgv) == false){ //should never reach here, nor even return true. Should fail it returns false
				
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	REG_IME = 0;
	set0xFFFF0000FastMPUSettings();
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	REG_IME = 1;
	
	setupDisabledExceptionHandler();
	
	// Create Woopsi UI
	WoopsiTemplate WoopsiTemplateApp;
	WoopsiTemplateProc = &WoopsiTemplateApp;
	return WoopsiTemplateApp.main(argc, argv);
	
	while(1) {
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}

	return 0;
}

