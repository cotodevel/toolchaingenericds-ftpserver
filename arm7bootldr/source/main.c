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


////////////////////////////////TGDS-MB v3 VRAM Bootcode start////////////////////////////////
#include "main.h"
#include "biosTGDS.h"
#include "spifwTGDS.h"
#include "posixHandleTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "loader.h"
#include "dldi.h"
#include "exceptionTGDS.h"
#include "dmaTGDS.h"
#include "TGDS_threads.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char fname[256];
char debugBuf7[256];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void strrev(char *arr, int start, int end)
{
    char temp;

    if (start >= end)
        return;

    temp = *(arr + start);
    *(arr + start) = *(arr + end);
    *(arr + end) = temp;

    start++;
    end--;
    strrev(arr, start, end);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char *itoa(int number, char *arr, int base)
{
    int i = 0, r, negative = 0;

    if (number == 0)
    {
        arr[i] = '0';
        arr[i + 1] = '\0';
        return arr;
    }

    if (number < 0 && base == 10)
    {
        number *= -1;
        negative = 1;
    }

    while (number != 0)
    {
        r = number % base;
        arr[i] = (r > 9) ? (r - 10) + 'a' : r + '0';
        i++;
        number /= base;
    }

    if (negative)
    {
        arr[i] = '-';
        i++;
    }

    strrev(arr, 0, i - 1);

    arr[i] = '\0';

    return arr;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void bootfile(){
	
}

extern u8 testram[400*1024];
u8 testram[400*1024];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	/*			TGDS 1.6 Standard ARM7 Init code start	*/
	installWifiFIFO();
	while(!(*(u8*)0x04000240 & 2) ){} //wait for VRAM_D block
	ARM7InitDLDI(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, TGDSDLDI_ARM7_ADDRESS);
	struct task_Context * TGDSThreads = getTGDSThreadSystem();
	/*			TGDS 1.6 Standard ARM7 Init code end	*/
	
	int i = 0;
	for(i = 0; i < sizeof(testram); i++){
		testram[i] = i;
	}
	
	while (1) {
		bool waitForVblank = false;
		int threadsRan = runThreads(TGDSThreads, waitForVblank);
	}
	return 0;
}

////////////////////////////////TGDS-MB v3 VRAM Bootcode end////////////////////////////////

//stubs for custom DSWIFI library
/*
void setupSound(uint32 sourceBuf){

}

void stopSound(){

}

void DeInitWIFI(){

}

void initARM7Malloc(u32 ARM7MallocStartaddress, u32 ARM7MallocSize){

}

void micInterrupt(){

}

void wifiAddressHandler( void * address, void * userdata ){

}

void Wifi_Update(){

}

void Wifi_Interrupt(){

}

void installWifiFIFO(){
    
}

void libUtilsFIFONotEmpty(u32 cmd1, u32 cmd2){

}

void TIMER1Handler(){	
	
}
*/