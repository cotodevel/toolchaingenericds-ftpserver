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
#include "biosTGDS.h"
#include "loader.h"
#include "busTGDS.h"
#include "dmaTGDS.h"
#include "spifwTGDS.h"
#include "wifi_arm7.h"
#include "pff.h"
#include "ipcfifoTGDSUser.h"
#include "ima_adpcm.h"
#include "soundTGDS.h"

FATFS Fatfs;					// Petit-FatFs work area 
struct soundPlayerContext soundData;
char fname[256];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O2")))
#endif
void handleARM7FSSetup(){
	uint32 * fifomsg = (uint32 *)NDS_CACHED_SCRATCHPAD;			
	UINT br;	/* Bytes read */
	uint8_t fresult;
	
	fresult = pf_mount(&Fatfs);	/* Initialize file system */
	if (fresult != FR_OK) { /* File System could not be mounted */
		fifomsg[33] = 0xAABBCCDD;
	}
	struct sIPCSharedTGDSSpecific* sharedIPC = getsIPCSharedTGDSSpecific();
	char * filename = (char*)&sharedIPC->filename[0];
	strcpy((char*)fname, filename);
	fresult = pf_open(fname);
	
	int argBuffer[MAXPRINT7ARGVCOUNT];
	memset((unsigned char *)&argBuffer[0], 0, sizeof(argBuffer));
	argBuffer[0] = 0xc070ffff;
	
	if (fresult != FR_OK) { /* File System could not be mounted */
		//strcpy((char*)0x02000000, "File open ERR!");
	}
	else{
		//strcpy((char*)0x02000000, "File open OK!");	//TGDS-FTPServer so far OK
	}
	pf_lseek(0);
	
	//decode audio here
	bool loop_audio = false;
	bool automatic_updates = false;
	if(player.play(loop_audio, automatic_updates, ADPCM_SIZE, stopSoundStreamUser) == 0){	//out of memory corruption, todo fix
		//ADPCM Playback!
	}
	
	fifomsg[33] = (u32)fresult;
}

void handleARM7FSRender(){
	
}

//Returns: fetched into TGDS_ARM7_RENDERBUFFER's size
int fetchVideoFrame(int fileOffset, int bufferSize){
	return 0;
}

int main(int argc, char **argv)  {
	/*			TGDS 1.6 Standard ARM7 Init code start	*/
	installWifiFIFO();		
	/*			TGDS 1.6 Standard ARM7 Init code end	*/
	
	SendFIFOWords(FIFO_ARM7_RELOAD_OK);
	while (1) {
		handleARM7SVC();	/* Do not remove, handles TGDS services */
		IRQWait(0, IRQ_VBLANK);
	}
   
	return 0;
}

bool stopSoundStreamUser(){

}