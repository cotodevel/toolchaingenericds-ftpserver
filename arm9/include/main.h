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

#ifndef __main9_h__
#define __main9_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "limitsTGDS.h"

#ifdef __cplusplus

//C++ part
using namespace std;
#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#endif

#ifdef __cplusplus
class FileDirEntry
{
  public:
    int Index;
	std::string filePathFilename;
    int type;	//FT_DIR / FT_FILE / FT_NONE	//  setup on Constructor / updated by getFileFILINFOfromPath(); / must be init from the outside 
    // Constructor
    FileDirEntry(int indexInst, std::string filePathFilenameInst, int typeInst)
	{
		Index = indexInst;
		filePathFilename = filePathFilenameInst;
		type = typeInst;
	}
	
	//helpers if/when Constructor is not available
	int getIndex()
    {
		return Index;
    }
    std::string getfilePathFilename()
    {
		return filePathFilename;
    }
	int gettype()
    {
		return type;
    }
	
	void setIndex(int IndexInst){
		Index = IndexInst;
	}
	void setfilename(std::string filePathFilenameInst){
		filePathFilename = filePathFilenameInst;
	}
	void settype(int typeInst){
		type = typeInst;
	}
};
#endif

#endif


#ifdef __cplusplus
extern "C" {
#endif

extern char CWDFTP[512];

extern int main(int _argc, sint8 **_argv);
extern void menuShow();

extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
extern bool ShowBrowser(char * Path);

extern char ListPathPrint[2048];
extern char * buildList();	//C++ <- C

#ifdef __cplusplus
extern std::list<std::string> completePath;
extern void getValidDir(std::string &dirName);
extern std::string getCurrentWorkingDir(bool showRootPath);
extern std::vector<class FileDirEntry> browse(std::string dir, bool strict);
extern std::string getDldiDefaultPath();
extern std::string parseDirNameTGDS(std::string dirName);
extern std::string parsefileNameTGDS(std::string fileName);
#endif

#ifdef __cplusplus
}
#endif