
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

//TGDS required version: IPC Version: 1.3

//IPC FIFO Description: 
//		struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress; 														// Access to TGDS internal IPC FIFO structure. 		(ipcfifoTGDS.h)
//		struct sIPCSharedTGDSSpecific * TGDSUSERIPC = (struct sIPCSharedTGDSSpecific *)TGDSIPCUserStartAddress;		// Access to TGDS Project (User) IPC FIFO structure	(ipcfifoTGDSUser.h)

#include "ipcfifoTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "InterruptsARMCores_h.h"
#include "loader.h"

#ifdef ARM7
#include <string.h>

#include "main.h"
#include "wifi_arm7.h"
#include "spifwTGDS.h"

#endif

#ifdef ARM9
#include <stdbool.h>
#include "main.h"
#include "wifi_arm9.h"
#include <stdbool.h>
#include "main.h"
#include "wifi_arm9.h"
#include "nds_cp15_misc.h"
#include "videoTGDS.h"
#include "arm7bootldr.h"
#include "dmaTGDS.h"
#endif


#ifdef ARM9
__attribute__((section(".itcm")))
#endif
struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific(){
	struct sIPCSharedTGDSSpecific* sIPCSharedTGDSSpecificInst = (struct sIPCSharedTGDSSpecific*)(TGDSIPCUserStartAddress);
	return sIPCSharedTGDSSpecificInst;
}

//inherits what is defined in: ipcfifoTGDS.c
#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoNotEmptyWeakRef(volatile u32 cmd1){
	switch (cmd1) {
		#ifdef ARM7
		case(FIFO_ARM7_RELOAD):{
			//NDS_LOADER_IPC_BOOTSTUBARM7_CACHED has ARM7.bin bootcode now
			u32 arm7entryaddress = NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm7EntryAddress;
			int arm7BootCodeSize = NDS_LOADER_IPC_CTX_UNCACHED_NTR->bootCode7FileSize;
			dmaTransferWord(3, (uint32)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (uint32)arm7entryaddress, (uint32)arm7BootCodeSize);
			reloadARMCore((u32)arm7entryaddress);	//Run Bootstrap7 
		}
		break;
		
		case(FIFO_DIRECTVIDEOFRAME_SETUP):{
			handleARM7FSSetup();
		}
		break;
		
		#endif
		
		#ifdef ARM9
		case(FIFO_ARM7_RELOAD_OK):{
			reloadStatus = 0;
		}
		break;
		#endif
	}
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2){
}

//project specific stuff
#ifdef ARM9
void updateStreamCustomDecoder(u32 srcFrmt){

}

void freeSoundCustomDecoder(u32 srcFrmt){

}
#endif

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void reloadARM7PlayerPayload(u32 arm7entryaddress, int arm7BootCodeSize){
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm7EntryAddress = arm7entryaddress;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->bootCode7FileSize = arm7BootCodeSize;
	//copy payload to NDS_LOADER_IPC_BOOTSTUBARM7_CACHED region
	coherent_user_range_by_size((u32)&arm7bootldr[0], arm7bootldr_size);
	dmaTransferWord(0, (u32)&arm7bootldr[0], (u32)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (uint32)arm7bootldr_size);
	SendFIFOWords(FIFO_ARM7_RELOAD);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
u32 playARM7ADPCMAudio(char * adpcmFile){
	struct sIPCSharedTGDSSpecific* sharedIPC = getsIPCSharedTGDSSpecific();
	char * filename = (char*)&sharedIPC->filename[0];
	strcpy(filename, adpcmFile);
	
	uint32 * fifomsg = (uint32 *)NDS_UNCACHED_SCRATCHPAD;
	fifomsg[33] = (uint32)0xFFFFCCAA;
	SendFIFOWords(FIFO_DIRECTVIDEOFRAME_SETUP);
	while(fifomsg[33] == (uint32)0xFFFFCCAA){
		swiDelay(1);
	}
	return fifomsg[33];
}

#endif