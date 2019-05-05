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

#include "fileoperator.h"

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
#include "ftpServer.h"
#include "wifi_arm9.h"
#include "dswnifi_lib.h"

#include <sstream>

template <class T>
inline std::string to_string (const T& t)
{
   std::stringstream ss;
   ss << t;
   return ss.str();
}

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

// check error cases, e.g. newPath = '..//' , '/home/user/' , 'subdir' (without trailing slash), etc... and return a clean, valid string in the form 'subdir/'
void getValidDir(std::string &dirName) {
    std::string slash = "/";
    size_t foundSlash = 0;
    while ( (foundSlash = dirName.find_first_of(slash),(foundSlash)) != std::string::npos) {
//        std::cout << " / @ " << foundSlash << std::endl;
        dirName.erase(foundSlash++,1); // Remove all slashs
    }
    dirName.append(slash); // Trailing slash is good and required, append it
}

// Returns the path to the current working dir starting from the server root dir
std::string getCurrentWorkingDir(bool showRootPath) {
	return string(getTGDSCurrentWorkingDirectory());
}

char ListPathPrint[2048];
char * buildList(){
	std::string res = "";
	// dir to browse
	std::string curDir = "/";
	std::vector<class FileDirEntry> filedirEntries = browse(curDir, false);
	
	int listFiles = 0;
	int listDirs = 0;
	for(int i = 0; i < (int)filedirEntries.size() ; i++){
		if(filedirEntries.at(i).gettype() == FT_FILE){
			int fSize = FS_getFileSize((char*)filedirEntries.at(i).getfilePathFilename().c_str());
			res += (std::string("-rwxrwxrwx   2 DS        " + to_string(fSize) + " Feb  1  2009 " +filedirEntries.at(i).getfilePathFilename() +" \r\n"));
			listFiles++;
		}
		else if(filedirEntries.at(i).gettype() == FT_DIR){
			//res +=  "%crwxrwxrwx   2 DS        %d Feb  1  2009 %s\r\n" + "d" << 0 << filedirEntries.at(i));
			res += (std::string("drwxrwxrwx   2 DS        " + to_string(0) + " Feb  1  2009 " +filedirEntries.at(i).getfilePathFilename() +" \r\n"));
			listDirs++;
		}
	}
	//printf("Dir: %s Browsing End. %d files - %d dir(s)", curDir.c_str(), listFiles, listDirs);
	int sizeList = strlen(res.c_str());
	if((int)sizeList > (int)sizeof(ListPathPrint)){
		sizeList = sizeof(ListPathPrint);
	}
	strncpy( (char*)&ListPathPrint[0], (const char *) res.c_str(), sizeList);
	return (char*)&ListPathPrint[0];
}

// Lists all files and directories in the specified directory and returns them in a string vector
std::vector<class FileDirEntry> browse(std::string dir, bool strict){
    std::vector<class FileDirEntry> fdirEnt;
	/*
	if (strict) {// When using strict mode, the function only allows one subdirectory and not several subdirectories, e.g. like sub/subsub/dir/ ...
        getValidDir(dir);
    }
    if (dir.compare("/") != 0) {
        dir = getCurrentWorkingDir(true).append(dir);
    } else {
        dir = getCurrentWorkingDir(true);
//        std::cout << "Yes" << std::endl;
    }
    */
	char fname[256];
	sprintf(fname,"%s",dir.c_str());
	
	int curFileDirIndx = 0;
	int retf = FAT_FindFirstFile(fname);
	while(retf != FT_NONE){
		struct FileClass * fileClassInst = NULL;
		
		//directory?
		if(retf == FT_DIR){
			fileClassInst = getFileClassFromList(LastDirEntry);
			std::string newCurDirEntry = parseDirNameTGDS(std::string(fileClassInst->fd_namefullPath));
			//printf("dir: %s", newCurDirEntry.c_str());
			FileDirEntry fent = FileDirEntry(curFileDirIndx, newCurDirEntry, retf);
			fdirEnt.push_back(fent);
		}
		//file?
		else if(retf == FT_FILE){
			fileClassInst = getFileClassFromList(LastFileEntry);
			std::string newCurFileEntry = parsefileNameTGDS(std::string(fileClassInst->fd_namefullPath));
			//printf("file: %s", newCurFileEntry.c_str());
			FileDirEntry fent = FileDirEntry(curFileDirIndx, newCurFileEntry, retf);
			fdirEnt.push_back(fent);
		}
		
		//more file/dir objects?
		retf = FAT_FindNextFile(fname);
		curFileDirIndx++;
	}
	
	return fdirEnt;
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = false;	//set default console or custom console: default console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	printf("     ");
	printf("     ");
	
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
	
	
	//custom Handler
	menuShow();
	setFTPState(FTP_SERVER_IDLE);
	
	while (1){
		
		sint32 FTP_STATUS = do_ftp_server();
		if(FTP_STATUS == FTP_SERVER_PROC_RUNNING){
			//Server Running
		}
		else if(FTP_STATUS == FTP_SERVER_PROC_FAILED){
			//Server Disconnected/Idle!
			setFTPState(FTP_SERVER_IDLE);
			printf("Client disconnected!. Press A to retry.");
			scanKeys();
			while(!(keysPressed() & KEY_A)){
				scanKeys();
				IRQVBlankWait();
			}
			main(0, (sint8**)"");
			
		}
	}

}
