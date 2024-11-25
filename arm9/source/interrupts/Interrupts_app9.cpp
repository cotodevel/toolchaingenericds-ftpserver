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

#include "InterruptsARMCores_h.h"
#include "ipcfifoTGDSUser.h"
#include "dsregs_asm.h"
#include "main.h"
#include "keypadTGDS.h"
#include "interrupts.h"
#include "utilsTGDS.h"
#include "spifwTGDS.h"
#include "powerTGDS.h"

//User Handler Definitions
#include "woopsifuncs.h"
#include "WoopsiTemplate.h"

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void IpcSynchandlerUser(uint8 ipcByte){
	switch(ipcByte){
		default:{
			//ipcByte should be the byte you sent from external ARM Core through sendByteIPC(ipcByte);
		}
		break;
	}
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer0handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer1handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer2handlerUser(){
	handleTurnOnTurnOffScreenTimeout();
	
	//Handle normal input to turn back on bottom screen 
	scanKeys();
	u32 pressed = keysDown();
	if(
		(pressed&KEY_TOUCH)
		||
		(pressed&KEY_A)
		||
		(pressed&KEY_B)
		||
		(pressed&KEY_UP)
		||
		(pressed&KEY_DOWN)
		||
		(pressed&KEY_LEFT)
		||
		(pressed&KEY_RIGHT)
		||
		(pressed&KEY_L)
		||
		(pressed&KEY_R)
		){
		bottomScreenIsLit = true; //input event triggered
	}
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer3handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HblankUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void VblankUser(){
	woopsiVblFunc();
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void VcounterUser(){
	
}

//Note: this event is hardware triggered from ARM7, on ARM9 a signal is raised through the FIFO hardware
#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void screenLidHasOpenedhandlerUser(){
	if(WoopsiTemplateProc != NULL){
		WoopsiTemplateProc->handleLidOpen();
	}
	setBacklight(POWMAN_BACKLIGHT_BOTTOM_BIT); 
}

//Note: this event is hardware triggered from ARM7, on ARM9 a signal is raised through the FIFO hardware
#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void screenLidHasClosedhandlerUser(){
	if(WoopsiTemplateProc != NULL){
		WoopsiTemplateProc->handleLidClosed();
	}
}