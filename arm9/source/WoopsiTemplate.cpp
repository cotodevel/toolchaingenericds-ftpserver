// Includes
#include "WoopsiTemplate.h"
#include "woopsiheaders.h"
#include "bitmapwrapper.h"
#include "bitmap.h"
#include "graphics.h"
#include "rect.h"
#include "gadgetstyle.h"
#include "fonts/newtopaz.h"
#include "woopsistring.h"
#include "colourpicker.h"
#include "filerequester.h"
#include "soundTGDS.h"
#include "main.h"
#include "posixHandleTGDS.h"
#include "keypadTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "dswnifi_lib.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "dldi.h"
#include "conf.h"

//TCP
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <in.h>
#include <string.h>
#include "socket2.h"

char scrollingBoxLoggerOutput[256];

__attribute__((section(".dtcm")))
WoopsiTemplate * WoopsiTemplateProc = NULL;

void WoopsiTemplate::startup(int argc, char **argv) {
	
	Rect rect;

	/** SuperBitmap preparation **/
	// Create bitmap for superbitmap
	Bitmap* superBitmapBitmap = new Bitmap(164, 191);

	// Get a graphics object from the bitmap so that we can modify it
	Graphics* gfx = superBitmapBitmap->newGraphics();

	// Clean up
	delete gfx;

	// Create screens
	AmigaScreen* newScreen = new AmigaScreen(TGDSPROJECTNAME, Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(newScreen);
	newScreen->setPermeable(true);
	
	// Add child windows: Indicates current NTR/TWLmode
	WoopsiString currentMode;
	if(__dsimode == true){
		currentMode = WoopsiString("TWL Mode (NTR WIFI)");
	}
	else{
		currentMode = WoopsiString("NTR Mode (NTR WIFI)");
	}
	AmigaWindow* _controlWindow = new AmigaWindow(0, 13, 256, 14, currentMode, Gadget::GADGET_DRAGGABLE, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
	newScreen->addGadget(_controlWindow);
	
	// Add File listing screen
	_fileScreen = new AmigaScreen("FTP Server status:", Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(_fileScreen);
	_fileScreen->setPermeable(true);
	_fileScreen->flipToTopScreen();
	// Add screen background
	_fileScreen->insertGadget(new Gradient(0, SCREEN_TITLE_HEIGHT, 256, 192 - SCREEN_TITLE_HEIGHT, woopsiRGB(0, 31, 0), woopsiRGB(0, 0, 31)));
	
	scrollingBoxLogger = new ScrollingTextBox(rect.x + 1, rect.y + 1, 246, 127, "ToolchainGenericDS-FTPServer start.\n"
		"--------------------------------\n\n", Gadget::GADGET_DRAGGABLE, 50);
	
	scrollingBoxLogger->setRefcon(1);
	_fileScreen->addGadget(scrollingBoxLogger);
	scrollingBoxLogger->addGadgetEventHandler(this);
	scrollingBoxLogger->setTextAlignmentHoriz(MultiLineTextBox::TEXT_ALIGNMENT_HORIZ_LEFT);
	scrollingBoxLogger->hideCursor();
	
	currentFileRequesterIndex = 0;
	
	_MultiLineTextBoxLogger = NULL;	//destroyable TextBox
	disableWaitForVblank();
	
	enableDrawing();	// Ensure Woopsi can now draw itself
	redraw();			// Draw initial state
}

void WoopsiTemplate::shutdown() {
	Woopsi::shutdown();
}

void WoopsiTemplate::waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText){
	thisLineTextBox->appendText(thisText);
	scanKeys();
	while((!(keysDown() & KEY_A)) && (!(keysDown() & KEY_TOUCH))){
		scanKeys();
	}
	scanKeys();
	while((keysDown() & KEY_A) && (keysDown() & KEY_TOUCH)){
		scanKeys();
	}
}

void WoopsiTemplate::handleValueChangeEvent(const GadgetEventArgs& e) {

	// Did a gadget fire this event?
	if (e.getSource() != NULL) {
	
		// Is the gadget the file requester?
		if ((e.getSource()->getRefcon() == 1) && (((FileRequester*)e.getSource())->getSelectedOption() != NULL)) {
			
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			pendPlay = 1;

			/* 
			//Destroyable Textbox implementation init
			Rect rect;
			_fileScreen->getClientRect(rect);
			_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y, 262, 170, "Loading\n...", Gadget::GADGET_DRAGGABLE, 5);
			_fileScreen->addGadget(_MultiLineTextBoxLogger);
			
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText("File open OK: ");
			_MultiLineTextBoxLogger->appendText(strObj);
			_MultiLineTextBoxLogger->appendText("\n");
			_MultiLineTextBoxLogger->appendText("Please wait calculating CRC32... \n");
			
			char arrBuild[256+1];
			sprintf(arrBuild, "%s%x\n", "Invalid file: crc32 = 0x", crc32);
			_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			
			sprintf(arrBuild, "%s%x\n", "Expected: crc32 = 0x", 0x5F35977E);
			_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			
			waitForAOrTouchScreenButtonMessage(_MultiLineTextBoxLogger, "Press (A) or tap touchscreen to continue. \n");
			
			_MultiLineTextBoxLogger->invalidateVisibleRectCache();
			_fileScreen->eraseGadget(_MultiLineTextBoxLogger);
			_MultiLineTextBoxLogger->destroy();	//same as delete _MultiLineTextBoxLogger;
			//Destroyable Textbox implementation end
			*/
		}
	}
}

void WoopsiTemplate::handleLidClosed() {
	// Lid has just been closed
	_lidClosed = true;

	// Run lid closed on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidClose();
		i++;
	}
}

void WoopsiTemplate::handleLidOpen() {
	// Lid has just been opened
	_lidClosed = false;

	// Run lid opened on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidOpen();
		i++;
	}
}

void WoopsiTemplate::handleClickEvent(const GadgetEventArgs& e) {
	switch (e.getSource()->getRefcon()) {
		//_Index Event
		case 2:{
			
		}	
		break;
		
		//_lastFile Event
		case 3:{
			
		}	
		break;
		
		//_nextFile Event
		case 4:{
			
		}	
		break;
		
		//_play Event
		case 5:{
			
		}	
		break;
		
		//_stop Event
		case 6:{
			
		}	
		break;
	}
}

__attribute__((section(".dtcm")))
u32 pendPlay = 0;

char currentFileChosen[256];

//Called once Woopsi events are ended: TGDS Main Loop
__attribute__((section(".itcm")))
void Woopsi::ApplicationMainLoop(){
	//Earlier.. main from Woopsi SDK.
	
	//Handle TGDS stuff...
	
	//FTP Server start
	if(_Wifi_InitTGDSFTPServer() == true){
		int FTPPort = 21;
		print_ip((uint32)Wifi_GetIP(), (sint8*)&DSServerIP);
		sprintf(scrollingBoxLoggerOutput, "\nNintendoDS IP: %s \nPort: %d \n", (char*)&DSServerIP, FTPPort);
		WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
		CSocket2* server=new CSocket2(true);
		server->Bind(FTPPort);
		server->Listen();
		while(true)
		{
			CSocket2* conn;
			do
			{
				conn=server->Accept(false);
				if(conn) {
					sprintf(scrollingBoxLoggerOutput, "\nClient connected\n");
					WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
					break;
				}
				IRQVBlankWait();
			} while(true);
			process(conn);
			delete conn;
		}
	}
	else{
		sprintf(scrollingBoxLoggerOutput, "\nNintendoDS AP Connection failure. Halting.\n");
		WoopsiTemplateProc->scrollingBoxLogger->appendText(WoopsiString(scrollingBoxLoggerOutput));
		while(1==1){
			IRQVBlankWait();
		}
	}
}