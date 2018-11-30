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

//C++ part
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iterator>

using namespace std;

#include "socket.h"
#include "in.h"
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "main.h"
#include "InterruptsARMCores_h.h"
#include "specific_shared.h"
#include "ff.h"
#include "memoryHandleTGDS.h"
#include "fileHandleTGDS.h"
#include "reent.h"
#include "sys/types.h"
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "consoleTGDS.h"
#include "utilsTGDS.h"

#include "devoptab_devices.h"
#include "fsfatlayerTGDS.h"
#include "usrsettingsTGDS.h"
#include "exceptionTGDS.h"

#include "videoTGDS.h"
#include "keypadTGDS.h"
#include "dldi.h"
#include "SpecialFunctions.h"
#include "dmaTGDS.h"
#include "biosTGDS.h"
#include "nds_cp15_misc.h"
#include "notifierProcessor.h"
#include "limitsTGDS.h"
#include "ftpserver.h" //#include "ftpServer.h"

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

string ToStr( char c ) {
   return string( 1, c );
}


void menuShow(){
	clrscr();
	printf("                              ");
	printf("A: start FTP server. An IP and Port will be drawn shortly.");
}


template<class Iter>
Iter splitStrings(const std::string &s, const std::string &delim, Iter out)
{
	if (delim.empty()) {
		*out++ = s;
		return out;
	}
	size_t a = 0, b = s.find(delim);
	for (; b != std::string::npos;
		a = b + delim.length(), b = s.find(delim, a))
	{
		*out++ = std::move(s.substr(a, b - a));
	}
	*out++ = std::move(s.substr(a, s.length() - a));
	return out;
}

vector<string> splitCustom(string str, string token){
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index != (int)string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}


std::string parseDirNameTGDS(std::string dirName){
	if ((dirName.at(0) == '/') && (dirName.at(1) == '/')) {
		dirName.erase(0,1);	//trim the starting / if it has one
	}
	dirName.erase(dirName.length());	//trim the leading "/"
	return dirName;
}

std::string parsefileNameTGDS(std::string fileName){
	if ((fileName.at(2) == '/') && (fileName.at(3) == '/')) {
		fileName.erase(2,2);	//trim the starting // if it has one (since getfspath appends 0:/)
		if(fileName.at(2) != '/'){	//if we trimmed by accident the only leading / such as 0:filename instead of 0:/filename, restore it so it becomes the latter
			fileName.insert(2, ToStr('/') );
		}
	}
	return fileName;
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.4 Standard ARM9 Init code start	*/
	IRQInit();
	
	bool project_specific_console = false;	//set default console or custom console: default console
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
	/*			TGDS 1.4 Standard ARM9 Init code end	*/
	
	//custom Handler
	menuShow();
	
	ftp_server_setup();
	
	while (1){
	
		do_ftp_server();
		
		scanKeys();
		
		if (keysPressed() & KEY_A){
			
			//FTP
			printf("alive!");
			while(keysPressed() & KEY_A){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_SELECT){
			clrscr();
			menuShow();
		}
		
		IRQVBlankWait();
	}

}