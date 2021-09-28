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

//inherits what is defined in: ipcfifoTGDS.h
#ifndef __ipcfifoTGDSUser_h__
#define __ipcfifoTGDSUser_h__

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"
#include "dswnifi.h"
#include "posixHandleTGDS.h"

//---------------------------------------------------------------------------------
struct sIPCSharedTGDSSpecific {
//---------------------------------------------------------------------------------
	char filename[256];
};

#define FIFO_DIRECTVIDEOFRAME_SETUP (u32)(0xFFFFABC8)
#define FIFO_ARM7_RELOAD_OK (u32)(0xFFFFABC9)
#define FIFO_ARM7_RELOAD (u32)(0xFFFFABCA)

#ifdef ARM9

//TGDS Memory Layout ARM7/ARM9 Cores
#define TGDS_ARM7_MALLOCSTART (u32)((int)0x06020000 + (96*1024))	//right after program end
#define TGDS_ARM7_MALLOCSIZE (int)(32*1024)
#define TGDSDLDI_ARM7_ADDRESS (u32)((int)0x03800000 + (32*1024))	//right after adpcm decoded buf

#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

//NOT weak symbols : the implementation of these is project-defined (here)
extern void HandleFifoNotEmptyWeakRef(volatile u32 cmd1);
extern void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2);

extern struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific();

#ifdef ARM9
extern void reloadARM7PlayerPayload(u32 arm7entryaddress, int arm7BootCodeSize);
extern u32 playARM7ADPCMAudio(char * adpcmFile);
#endif

#ifdef __cplusplus
}
#endif