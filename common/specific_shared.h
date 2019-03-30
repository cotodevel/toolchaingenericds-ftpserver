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

//inherits what is defined in: ipcfifoTGDS.h
#ifndef __specific_shared_h__
#define __specific_shared_h__

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"
#include "dswnifi.h"
#include "memoryHandleTGDS.h"

//---------------------------------------------------------------------------------
struct sIPCSharedTGDSSpecific {
//---------------------------------------------------------------------------------
};

#endif

#ifdef __cplusplus
extern "C" {
#endif


extern struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific();
#ifdef __cplusplus
}
#endif