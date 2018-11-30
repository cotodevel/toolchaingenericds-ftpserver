/*
 * The MIT License
 *
 * Copyright 2018 Ugo Cirmignani.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <types.h>
#include <socket.h>
#include <in.h>
//#include "stat.h"

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "ftpData.h"
#include "ftpServer.h"
#include "library/logFunctions.h"
#include "library/fileManagement.h"
#include "library/configRead.h"
#include "ftpCommandsElaborate.h"


/* Elaborate the User login command */
int parseCommandUser(clientDataType *theClientData)
{
    int returnCode;
    char *theUserName;
    theUserName = getFtpCommandArg("USER", theClientData->theCommandReceived, 0);

    if (strlen(theUserName) >= 1)
    {
        setDynamicStringDataType(&theClientData->login.name, theUserName, strlen(theUserName));
        returnCode = printf(theClientData->socketDescriptor, "331 User ok, Waiting for the password. ");

        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;

        return FTP_COMMAND_PROCESSED;
    }
    else
    {
        returnCode = printf(theClientData->socketDescriptor, "430 Invalid username. ");
        
        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;

        return FTP_COMMAND_PROCESSED;
    }
}

/* Elaborate the User login command */
int parseCommandSite(clientDataType *theClientData)
{
    int returnCode, setPermissionsReturnCode;
    char *theCommand;
    theCommand = getFtpCommandArg("SITE", theClientData->theCommandReceived, 0);

    if(compareStringCaseInsensitive(theCommand, "CHMOD", strlen("CHMOD")) == 1)
    {
        setPermissionsReturnCode = setPermissions(theCommand, theClientData->login.absolutePath.text, theClientData->login.ownerShip);

        switch (setPermissionsReturnCode)
        {
            case FTP_CHMODE_COMMAND_RETURN_CODE_OK:
            {
                returnCode = printf(theClientData->socketDescriptor, "200 Permissions changed. ");
            }
            break;

            case FTP_CHMODE_COMMAND_RETURN_CODE_NO_FILE:
            {
                returnCode = printf(theClientData->socketDescriptor, "550 chmod no such file. ");
            }
            break;

            case FTP_CHMODE_COMMAND_RETURN_CODE_NO_PERMISSIONS:
            {
                returnCode = printf(theClientData->socketDescriptor, "550 Some errors occoured while changing file permissions. ");
            }
            break;            

            case FTP_CHMODE_COMMAND_RETURN_NAME_TOO_LONG:
            default: 
            {
                return FTP_COMMAND_PROCESSED_WRITE_ERROR;
            }
            break;
        }
    }
    else
    {
        returnCode = printf(theClientData->socketDescriptor, "500 unknown extension. ");

        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    }
 
    return FTP_COMMAND_PROCESSED;
}

int parseCommandPass(ftpDataType * data, int socketId)
{
    int returnCode;
    char *thePass;
    loginFailsDataType element;
    int searchPosition = -1;

    thePass = getFtpCommandArg("PASS", data->clients[socketId].theCommandReceived, 0);

    strcpy(element.ipAddress, data->clients[socketId].clientIpAddress);
    element.failTimeStamp = time(NULL);
    element.failureNumbers = 1;

    searchPosition = data->loginFailsVector.SearchElement(&data->loginFailsVector, &element);

    if (searchPosition != -1)
    {
        if (element.failTimeStamp - ((loginFailsDataType *) data->loginFailsVector.Data[searchPosition])->failTimeStamp < WRONG_PASSWORD_ALLOWED_RETRY_TIME)
        {
            if (((loginFailsDataType *) data->loginFailsVector.Data[searchPosition])->failureNumbers >= data->ftpParameters.maximumUserAndPassowrdLoginTries)
            {
                //printf("TOO MANY LOGIN FAILS! ");
                data->clients[socketId].closeTheClient = 1;
                returnCode = printf(data->clients[socketId].socketDescriptor, "430 Too many login failure detected, your ip will be blacklisted for 5 minutes. ");
                if (returnCode <= 0) return FTP_COMMAND_PROCESSED_WRITE_ERROR;
                return FTP_COMMAND_PROCESSED;
            }
        }
    }

    if (strlen(thePass) >= 1)
    {
        int searchUserNameIndex;
        searchUserNameIndex = searchUser(data->clients[socketId].login.name.text, &data->ftpParameters.usersVector);

        if (searchUserNameIndex < 0 ||
            (strcmp(((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->password, thePass) != 0))
        {
            
            //Record the login fail!
            if (searchPosition == -1)
            {
                if (data->ftpParameters.maximumUserAndPassowrdLoginTries != 0)
                    data->loginFailsVector.PushBack(&data->loginFailsVector, &element, sizeof(loginFailsDataType));
            }
            else
            {
                ((loginFailsDataType *) data->loginFailsVector.Data[searchPosition])->failureNumbers++;
                ((loginFailsDataType *) data->loginFailsVector.Data[searchPosition])->failTimeStamp = time(NULL);
            }
            
            returnCode = printf(data->clients[socketId].socketDescriptor, "430 Invalid username or password. ");
            if (returnCode <= 0) return FTP_COMMAND_PROCESSED_WRITE_ERROR;
        }
        else
        {
            setDynamicStringDataType(&data->clients[socketId].login.password, thePass, strlen(thePass));
            setDynamicStringDataType(&data->clients[socketId].login.absolutePath, ((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->homePath, strlen(((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->homePath));
            setDynamicStringDataType(&data->clients[socketId].login.homePath, ((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->homePath, strlen(((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->homePath));
            setDynamicStringDataType(&data->clients[socketId].login.ftpPath, "/", strlen("/"));

            data->clients[socketId].login.ownerShip.ownerShipSet = ((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->ownerShip.ownerShipSet;
            data->clients[socketId].login.ownerShip.gid = ((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->ownerShip.gid;
            data->clients[socketId].login.ownerShip.uid = ((usersParameters_DataType *) data->ftpParameters.usersVector.Data[searchUserNameIndex])->ownerShip.uid;
            data->clients[socketId].login.userLoggedIn = 1;

            returnCode = printf(data->clients[socketId].socketDescriptor, "230 Login Ok. ");
            if (returnCode <= 0) return FTP_COMMAND_PROCESSED_WRITE_ERROR;
        }

       return FTP_COMMAND_PROCESSED;
    }
    else
    {

        //Record the login fail!
        if (searchPosition == -1)
        {
            if (data->ftpParameters.maximumUserAndPassowrdLoginTries != 0)
                data->loginFailsVector.PushBack(&data->loginFailsVector, &element, sizeof(loginFailsDataType));
        }
        else
        {
            ((loginFailsDataType *) data->loginFailsVector.Data[searchPosition])->failureNumbers++;
            ((loginFailsDataType *) data->loginFailsVector.Data[searchPosition])->failTimeStamp = time(NULL);
        }

        returnCode = printf(data->clients[socketId].socketDescriptor, "430 Invalid username or password. ");
        if (returnCode <= 0) return FTP_COMMAND_PROCESSED_WRITE_ERROR;
        return 1;
    }
}

int parseCommandAuth(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "502 Security extensions not implemented. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandPwd(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "257 %s is your current location. ", theClientData->login.ftpPath.text);
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandSyst(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "215 UNIX Type: L8. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandFeat(clientDataType *theClientData)
{
    /*
    211-Extensions supported:
     EPRT
     IDLE
     MDTM
     SIZE
     MFMT
     REST STREAM
     MLST type*;size*;sizd*;modify*;UNIX.mode*;UNIX.uid*;UNIX.gid*;unique*;
     MLSD
     AUTH TLS
     PBSZ
     PROT
     UTF8
     TVFS
     ESTA
     PASV
     EPSV
     SPSV
     ESTP
    211 End.
     */
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "211-Extensions supported: PASV UTF8 211 End. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandTypeA(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "200 TYPE is now 8-bit binary. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandTypeI(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "200 TYPE is now 8-bit binary. ");
    
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandStruF(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "200 TYPE is now 8-bit binary. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandModeS(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "200 TYPE is now 8-bit binary. ");
    
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandPasv(ftpDataType * data, int socketId)
{
    /* Create worker thread */
    void *pReturn;

    if (data->clients[socketId].workerData.threadIsAlive == 1) 
    {
        pthread_cancel(data->clients[socketId].workerData.workerThread);
    }
    
    pthread_join(data->clients[socketId].workerData.workerThread, &pReturn);
    data->clients[socketId].workerData.passiveModeOn = 1;
    data->clients[socketId].workerData.activeModeOn = 0;    
    pthread_create(&data->clients[socketId].workerData.workerThread, NULL, connectionWorkerHandle, (void *) &data->clients[socketId].clientProgressiveNumber);
    return FTP_COMMAND_PROCESSED;
}

int parseCommandPort(ftpDataType * data, int socketId)
{
    char *theIpAndPort;
    int ipAddressBytes[4];
    int portBytes[2];
    theIpAndPort = getFtpCommandArg("PORT", data->clients[socketId].theCommandReceived, 0);    
    sscanf(theIpAndPort, "%d,%d,%d,%d,%d,%d", &ipAddressBytes[0], &ipAddressBytes[1], &ipAddressBytes[2], &ipAddressBytes[3], &portBytes[0], &portBytes[1]);
    data->clients[socketId].workerData.connectionPort = (portBytes[0]*256)+portBytes[1];
    sprintf(data->clients[socketId].workerData.activeIpAddress, "%d.%d.%d.%d", ipAddressBytes[0],ipAddressBytes[1],ipAddressBytes[2],ipAddressBytes[3]);

    void *pReturn;
    if (data->clients[socketId].workerData.threadIsAlive == 1)
    {
        pthread_cancel(data->clients[socketId].workerData.workerThread);
    }
    pthread_join(data->clients[socketId].workerData.workerThread, &pReturn);
    data->clients[socketId].workerData.passiveModeOn = 0;
    data->clients[socketId].workerData.activeModeOn = 1;    
    pthread_create(&data->clients[socketId].workerData.workerThread, NULL, connectionWorkerHandle, (void *) &data->clients[socketId].clientProgressiveNumber);
    return FTP_COMMAND_PROCESSED;
}

int parseCommandAbor(ftpDataType * data, int socketId)
{
    /*
     426 ABORT
    226-Transfer aborted
    226 3.406 seconds (measured here), 1.58 Mbytes per second
    226 Since you see this ABOR must've succeeded
    */
    int returnCode;
    if (data->clients[socketId].workerData.threadIsAlive == 1)
    {
        void *pReturn;
        if (data->clients[socketId].workerData.threadIsAlive == 1)
        {
            pthread_cancel(data->clients[socketId].workerData.workerThread);
        }
        pthread_join(data->clients[socketId].workerData.workerThread, &pReturn);

        returnCode = printf(data->clients[socketId].socketDescriptor, "426 ABORT. ");
        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;
         
        returnCode = printf(data->clients[socketId].socketDescriptor, "226 Transfer aborted. ");
        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    }
    else
    {
        returnCode = printf(data->clients[socketId].socketDescriptor, "226 Since you see this ABOR must've succeeded. ");
        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    }

    return FTP_COMMAND_PROCESSED;
}

int parseCommandList(ftpDataType * data, int socketId)
{
    /*
      -1 List one file per line
      -A List all files except "." and ".."
      -a List all files including those whose names start with "."
      -C List entries by columns
      -d List directory entries instead of directory contents
      -F Append file type indicator (one of "*", "/", "=", "@" or "|") to names
      -h Print file sizes in human-readable format (e.g. 1K, 234M, 2G)
      -L List files pointed to by symlinks
      -l Use a long listing format
      -n List numeric UIDs/GIDs instead of user/group names
      -R List subdirectories recursively
      -r Sort filenames in reverse order
      -S Sort by file size
      -t Sort by modification time 
     */
    
    int isSafePath = 0;
    char *theNameToList;
    
    theNameToList = getFtpCommandArg("LIST", data->clients[socketId].theCommandReceived, 1);
    getFtpCommandArgWithOptions("LIST", data->clients[socketId].theCommandReceived, &data->clients[socketId].workerData.ftpCommand);
 
    printf("LIST COMMAND ARG: %s", data->clients[socketId].workerData.ftpCommand.commandArgs.text);
    printf("LIST COMMAND OPS: %s", data->clients[socketId].workerData.ftpCommand.commandOps.text);
    printf("theNameToList: %s", theNameToList);
    
    cleanDynamicStringDataType(&data->clients[socketId].workerData.ftpCommand.commandArgs, 0);
    cleanDynamicStringDataType(&data->clients[socketId].workerData.ftpCommand.commandOps, 0);    
    cleanDynamicStringDataType(&data->clients[socketId].listPath, 0);

    if (strlen(theNameToList) > 0)
    {
        isSafePath = getSafePath(&data->clients[socketId].listPath, theNameToList, &data->clients[socketId].login);
    }

    if (isSafePath == 0)
    {
        cleanDynamicStringDataType(&data->clients[socketId].listPath, 0);
        setDynamicStringDataType(&data->clients[socketId].listPath, data->clients[socketId].login.absolutePath.text, data->clients[socketId].login.absolutePath.textLen);
    }

    pthread_mutex_lock(&data->clients[socketId].workerData.conditionMutex);
    memset(data->clients[socketId].workerData.theCommandReceived, 0, CLIENT_COMMAND_STRING_SIZE);
    strcpy(data->clients[socketId].workerData.theCommandReceived, data->clients[socketId].theCommandReceived);
    data->clients[socketId].workerData.commandReceived = 1;
    pthread_mutex_unlock(&data->clients[socketId].workerData.conditionMutex);
    pthread_cond_signal(&data->clients[socketId].workerData.conditionVariable);
    return FTP_COMMAND_PROCESSED;
}

int parseCommandNlst(ftpDataType * data, int socketId)
{
    int isSafePath = 0;
    char *theNameToNlist;
    theNameToNlist = getFtpCommandArg("NLIST", data->clients[socketId].theCommandReceived, 1);
    cleanDynamicStringDataType(&data->clients[socketId].nlistPath, 0);

    printf("NLIST COMMAND ARG: %s", data->clients[socketId].workerData.ftpCommand.commandArgs.text);
    printf("NLIST COMMAND OPS: %s", data->clients[socketId].workerData.ftpCommand.commandOps.text);
    printf("theNameToNlist: %s", theNameToNlist);
    
    if (strlen(theNameToNlist) > 0)
    {
        isSafePath = getSafePath(&data->clients[socketId].nlistPath, theNameToNlist, &data->clients[socketId].login);
    }

    if (isSafePath == 0)
    {
        cleanDynamicStringDataType(&data->clients[socketId].nlistPath, 0);
        setDynamicStringDataType(&data->clients[socketId].nlistPath, data->clients[socketId].login.absolutePath.text, data->clients[socketId].login.absolutePath.textLen);
    }
    
    pthread_mutex_lock(&data->clients[socketId].workerData.conditionMutex);
    memset(data->clients[socketId].workerData.theCommandReceived, 0, CLIENT_COMMAND_STRING_SIZE);
    strcpy(data->clients[socketId].workerData.theCommandReceived, data->clients[socketId].theCommandReceived);
    data->clients[socketId].workerData.commandReceived = 1;
    pthread_mutex_unlock(&data->clients[socketId].workerData.conditionMutex);
    pthread_cond_signal(&data->clients[socketId].workerData.conditionVariable);
    return FTP_COMMAND_PROCESSED;
}

int parseCommandRetr(ftpDataType * data, int socketId)
{
    int isSafePath = 0;
    char *theNameToRetr;

    theNameToRetr = getFtpCommandArg("RETR", data->clients[socketId].theCommandReceived, 0);
    cleanDynamicStringDataType(&data->clients[socketId].fileToRetr, 0);

    if (strlen(theNameToRetr) > 0)
    {
        isSafePath = getSafePath(&data->clients[socketId].fileToRetr, theNameToRetr, &data->clients[socketId].login);
    }

    if (isSafePath == 1 &&
        FILE_IsFile(data->clients[socketId].fileToRetr.text) == 1)
    {
        pthread_mutex_lock(&data->clients[socketId].workerData.conditionMutex);
        memset(data->clients[socketId].workerData.theCommandReceived, 0, CLIENT_COMMAND_STRING_SIZE);
        strcpy(data->clients[socketId].workerData.theCommandReceived, data->clients[socketId].theCommandReceived);
        data->clients[socketId].workerData.commandReceived = 1;
        pthread_mutex_unlock(&data->clients[socketId].workerData.conditionMutex);
        pthread_cond_signal(&data->clients[socketId].workerData.conditionVariable);
        return FTP_COMMAND_PROCESSED;
    }
    else
    {
        return FTP_COMMAND_NOT_RECONIZED;
    }

    return FTP_COMMAND_PROCESSED;
}

int parseCommandStor(ftpDataType * data, int socketId)
{
    int isSafePath = 0;
    char *theNameToStor;
    theNameToStor = getFtpCommandArg("STOR", data->clients[socketId].theCommandReceived, 0);
    cleanDynamicStringDataType(&data->clients[socketId].fileToStor, 0);
    
    if (strlen(theNameToStor) > 0)
    {
        isSafePath = getSafePath(&data->clients[socketId].fileToStor, theNameToStor, &data->clients[socketId].login);
    }

    if (isSafePath == 1)
    {
        pthread_mutex_lock(&data->clients[socketId].workerData.conditionMutex);
        memset(data->clients[socketId].workerData.theCommandReceived, 0, CLIENT_COMMAND_STRING_SIZE);
        strcpy(data->clients[socketId].workerData.theCommandReceived, data->clients[socketId].theCommandReceived);
        data->clients[socketId].workerData.commandReceived = 1;
        pthread_mutex_unlock(&data->clients[socketId].workerData.conditionMutex);
        pthread_cond_signal(&data->clients[socketId].workerData.conditionVariable);
    }
    else
    {
        return FTP_COMMAND_NOT_RECONIZED;
    }
    
    return FTP_COMMAND_PROCESSED;
}

int parseCommandCwd(clientDataType *theClientData)
{
    dynamicStringDataType absolutePathPrevious, ftpPathPrevious, theSafePath;
    int isSafePath;
    int returnCode;
    char *thePath;

    cleanDynamicStringDataType(&absolutePathPrevious, 1);
    cleanDynamicStringDataType(&ftpPathPrevious, 1);
    cleanDynamicStringDataType(&theSafePath, 1);

    thePath = getFtpCommandArg("CWD", theClientData->theCommandReceived, 0);

    if (strlen(thePath) > 0)
    {
        isSafePath = getSafePath(&theSafePath, thePath, &theClientData->login);
    }

    if (isSafePath == 1)
    {
        printf("The Path requested for CWD IS: %s", theSafePath.text);
        setDynamicStringDataType(&absolutePathPrevious, theClientData->login.absolutePath.text, theClientData->login.absolutePath.textLen);
        setDynamicStringDataType(&ftpPathPrevious, theClientData->login.ftpPath.text, theClientData->login.ftpPath.textLen);
        
        if (theSafePath.text[0] != '/')
        {
            if (theClientData->login.absolutePath.text[theClientData->login.absolutePath.textLen-1] != '/')
                appendToDynamicStringDataType(&theClientData->login.absolutePath, "/", 1);

            if (theClientData->login.ftpPath.text[theClientData->login.ftpPath.textLen-1] != '/')
                appendToDynamicStringDataType(&theClientData->login.ftpPath, "/", 1);

            appendToDynamicStringDataType(&theClientData->login.absolutePath, theSafePath.text, theSafePath.textLen);
            appendToDynamicStringDataType(&theClientData->login.ftpPath, theSafePath.text, theSafePath.textLen);
        }
        else if (theSafePath.text[0] == '/')
        {
            cleanDynamicStringDataType(&theClientData->login.ftpPath, 0);
            cleanDynamicStringDataType(&theClientData->login.absolutePath, 0);

            setDynamicStringDataType(&theClientData->login.ftpPath, theSafePath.text, theSafePath.textLen);
            setDynamicStringDataType(&theClientData->login.absolutePath, theClientData->login.homePath.text, theClientData->login.homePath.textLen);

            if (strlen(theSafePath.text)> 0)
            {
                char *theDirPointer = theSafePath.text;
                
                if (theClientData->login.absolutePath.text[theClientData->login.absolutePath.textLen-1] == '/')
                {
                    while(theDirPointer[0] == '/')
                        theDirPointer++;
                }
                
                if (strlen(theDirPointer) > 0)
                    appendToDynamicStringDataType(&theClientData->login.absolutePath, theDirPointer, strlen(theDirPointer));
            }
        }

        if (FILE_IsDirectory(theClientData->login.absolutePath.text) == 1)
        {
            returnCode = printf(theClientData->socketDescriptor, "250 OK. Current directory is  %s. ", theClientData->login.ftpPath.text);
        }
        else
        {
            setDynamicStringDataType(&theClientData->login.absolutePath, absolutePathPrevious.text, absolutePathPrevious.textLen);
            setDynamicStringDataType(&theClientData->login.ftpPath, ftpPathPrevious.text, ftpPathPrevious.textLen);
            returnCode = dprintf(theClientData->socketDescriptor, "530 Can't change directory to %s: No such file or directory. ", theClientData->login.absolutePath.text);
        }
        
        cleanDynamicStringDataType(&absolutePathPrevious, 0);
        cleanDynamicStringDataType(&ftpPathPrevious, 0);
        cleanDynamicStringDataType(&theSafePath, 0);
        
        if (returnCode <= 0) 
            return FTP_COMMAND_PROCESSED_WRITE_ERROR;


        return FTP_COMMAND_PROCESSED;
    }
    else
    {
        cleanDynamicStringDataType(&absolutePathPrevious, 0);
        cleanDynamicStringDataType(&ftpPathPrevious, 0);
        cleanDynamicStringDataType(&theSafePath, 0);
        return FTP_COMMAND_NOT_RECONIZED;
    }
}

int parseCommandRest(clientDataType *theClientData)
{
    int returnCode;
    int i, theSizeIndex;
    char theSize[FTP_COMMAND_ELABORATE_CHAR_BUFFER];
    memset(theSize, 0, FTP_COMMAND_ELABORATE_CHAR_BUFFER);
    theSizeIndex = 0;

    for (i = strlen("REST")+1; i < theClientData->commandIndex; i++)
    {
        if (theClientData->theCommandReceived[i] == '\r' ||
            theClientData->theCommandReceived[i] == '\0' ||
            theClientData->theCommandReceived[i] == '\n')
            {
            break;
            }
        
        if (theSizeIndex < FTP_COMMAND_ELABORATE_CHAR_BUFFER &&
            (theClientData->theCommandReceived[i] == '0' ||
            theClientData->theCommandReceived[i] == '1' ||
            theClientData->theCommandReceived[i] == '2' ||
            theClientData->theCommandReceived[i] == '3' ||
            theClientData->theCommandReceived[i] == '4' ||
            theClientData->theCommandReceived[i] == '5' ||
            theClientData->theCommandReceived[i] == '6' ||
            theClientData->theCommandReceived[i] == '7' ||
            theClientData->theCommandReceived[i] == '8' ||
            theClientData->theCommandReceived[i] == '9' ))
        {
            theSize[theSizeIndex++] = theClientData->theCommandReceived[i];
        }
    }

    theClientData->workerData.retrRestartAtByte = atoll(theSize);
    returnCode = dprintf(theClientData->socketDescriptor, "350 Restarting at %s ", theSize);

    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandMkd(clientDataType *theClientData)
{
    int returnCode;
    int functionReturnCode = FTP_COMMAND_NOT_RECONIZED;
    int isSafePath;
    char *theDirectoryFilename;
    dynamicStringDataType mkdFileName;

    theDirectoryFilename = getFtpCommandArg("MKD", theClientData->theCommandReceived, 0);
    
    cleanDynamicStringDataType(&mkdFileName, 1);
    
    isSafePath = getSafePath(&mkdFileName, theDirectoryFilename, &theClientData->login);
    
    if (isSafePath == 1)
    {
        int returnStatus;
        returnStatus = mkdir(mkdFileName.text, S_IRWXU | S_IRWXG | S_IRWXO);

        if (returnStatus == -1)
        {
            returnCode = printf(theClientData->socketDescriptor, "550 error while creating directory %s ", theDirectoryFilename);

            if (returnCode <= 0) 
            {
                functionReturnCode = FTP_COMMAND_PROCESSED_WRITE_ERROR;
            }
            else
            {
                functionReturnCode = FTP_COMMAND_PROCESSED;
            }
        }
        else
        {
            if (theClientData->login.ownerShip.ownerShipSet == 1)
            {
                returnStatus = FILE_doChownFromUidGid(mkdFileName.text, theClientData->login.ownerShip.uid, theClientData->login.ownerShip.gid);
            }

            returnCode = printf(theClientData->socketDescriptor, "257 \"%s\" : The directory was successfully created ", theDirectoryFilename);

            if (returnCode <= 0) 
            {
                functionReturnCode = FTP_COMMAND_PROCESSED_WRITE_ERROR;
            }
            else
            {
                functionReturnCode = FTP_COMMAND_PROCESSED;
            }
        }
    }
    else
    {
        cleanDynamicStringDataType(&mkdFileName, 0);
        functionReturnCode = FTP_COMMAND_NOT_RECONIZED;
    }

    cleanDynamicStringDataType(&mkdFileName, 0);    
    return functionReturnCode;
}

int parseCommandOpts(clientDataType *theClientData)
{
    int returnCode;
    returnCode = dprintf(theClientData->socketDescriptor, "200 OK ");

    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandDele(clientDataType *theClientData)
{
    int functionReturnCode = 0;
    int returnCode;
    int isSafePath;
    int returnStatus = 0;
    char *theFileToDelete;
    dynamicStringDataType deleFileName;

    theFileToDelete = getFtpCommandArg("DELE", theClientData->theCommandReceived, 0);

    cleanDynamicStringDataType(&deleFileName, 1);
    isSafePath = getSafePath(&deleFileName, theFileToDelete, &theClientData->login);
    
    if (isSafePath == 1)
    {
        //printf("The file to delete is: %s", deleFileName.text);
        if (FILE_IsFile(deleFileName.text) == 1)
        {
            returnStatus = remove(deleFileName.text);
            
            if (returnStatus == -1)
            {
                returnCode = printf(theClientData->socketDescriptor, "550 Could not delete the file:  %s some errors occoured. ", theFileToDelete);
            }
            else
            {
                returnCode = printf(theClientData->socketDescriptor, "250 Deleted %s ", theFileToDelete);
            }

            functionReturnCode = FTP_COMMAND_PROCESSED;

            if (returnCode <= 0) 
                functionReturnCode = FTP_COMMAND_PROCESSED_WRITE_ERROR;
        }
        else
        {
            returnCode = printf(theClientData->socketDescriptor, "550 Could not delete the file: No such file or file is a directory. ");
            functionReturnCode = FTP_COMMAND_PROCESSED;

            if (returnCode <= 0) 
                functionReturnCode = FTP_COMMAND_PROCESSED_WRITE_ERROR;
        }
    }
    else
    {
        functionReturnCode = FTP_COMMAND_NOT_RECONIZED;
    }
    
    cleanDynamicStringDataType(&deleFileName, 0);
    return functionReturnCode;
}

int parseCommandNoop(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "200 Zzz... ");
   
    if (returnCode <= 0) {
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    }

    return FTP_COMMAND_PROCESSED;
}

int notLoggedInMessage(clientDataType *theClientData)
{
    int returnCode;
    returnCode = printf(theClientData->socketDescriptor, "530 You aren't logged in. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

int parseCommandQuit(ftpDataType * data, int socketId)
{
    int returnCode;
    returnCode = printf(data->clients[socketId].socketDescriptor, "221 Logout. ");
    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    data->clients[socketId].closeTheClient = 1;
    return FTP_COMMAND_PROCESSED;
}

int parseCommandRmd(clientDataType *theClientData)
{
    int functionReturnCode = 0;
    int returnCode;
    int isSafePath;
    int returnStatus = 0;
    char *theDirectoryFilename;
    dynamicStringDataType rmdFileName;

    theDirectoryFilename = getFtpCommandArg("RMD", theClientData->theCommandReceived, 0);

    cleanDynamicStringDataType(&rmdFileName, 1);
    
    isSafePath = getSafePath(&rmdFileName, theDirectoryFilename, &theClientData->login);
    
    if (isSafePath == 1)
    {
        if (FILE_IsDirectory(rmdFileName.text) == 1) 
        {
            returnStatus = rmdir(rmdFileName.text);

            if (returnStatus == -1)
            {
                returnCode = printf(theClientData->socketDescriptor, "550 Could not remove the directory, some errors occoured. ");
            }
            else
            {
                returnCode = printf(theClientData->socketDescriptor, "250 The directory was successfully removed. ");
            }

            functionReturnCode = FTP_COMMAND_PROCESSED;

            if (returnCode <= 0)
                functionReturnCode = FTP_COMMAND_PROCESSED_WRITE_ERROR;
        }
        else
        {
            returnCode = printf(theClientData->socketDescriptor, "550 Could not delete the directory:No such directory. ");
            functionReturnCode = FTP_COMMAND_PROCESSED;

            if (returnCode <= 0) 
                functionReturnCode = FTP_COMMAND_PROCESSED_WRITE_ERROR;
        }
    }
    else
    {
        functionReturnCode = FTP_COMMAND_NOT_RECONIZED;
    }

    cleanDynamicStringDataType(&rmdFileName, 0);

    return functionReturnCode;
}

int parseCommandSize(clientDataType *theClientData)
{
    int returnCode;
    int isSafePath;
    long long int theSize;
    char *theFileName;
    dynamicStringDataType getSizeFromFileName;

    theFileName = getFtpCommandArg("SIZE", theClientData->theCommandReceived, 0);

    cleanDynamicStringDataType(&getSizeFromFileName, 1);
    
    isSafePath = getSafePath(&getSizeFromFileName, theFileName, &theClientData->login);

    if (isSafePath == 1)
    {

        if (FILE_IsFile(getSizeFromFileName.text)== 1)
        {
            theSize = FILE_GetFileSizeFromPath(getSizeFromFileName.text);
            returnCode = printf(theClientData->socketDescriptor, "213 %lld ", theSize);
        }
        else
        {
            returnCode = printf(theClientData->socketDescriptor, "550 Can't check for file existence ");
        }
    }
    else
    {
        returnCode = dprintf(theClientData->socketDescriptor, "550 Can't check for file existence ");
    }
    cleanDynamicStringDataType(&getSizeFromFileName, 0);

    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    
    return FTP_COMMAND_PROCESSED;
}

int parseCommandRnfr(clientDataType *theClientData)
{
    int returnCode;
    int isSafePath;
    char *theRnfrFileName;

    theRnfrFileName = getFtpCommandArg("RNFR", theClientData->theCommandReceived, 0);
    cleanDynamicStringDataType(&theClientData->renameFromFile, 0);
    
    isSafePath = getSafePath(&theClientData->renameFromFile, theRnfrFileName, &theClientData->login);
    
    if (isSafePath == 1&&
        (FILE_IsFile(theClientData->renameFromFile.text) == 1 ||
         FILE_IsDirectory(theClientData->renameFromFile.text) == 1))
    {
        if (FILE_IsFile(theClientData->renameFromFile.text) == 1 ||
            FILE_IsDirectory(theClientData->renameFromFile.text) == 1)
        {
            returnCode = printf(theClientData->socketDescriptor, "350 RNFR accepted - file exists, ready for destination ");
        }
        else
        {
            returnCode = printf(theClientData->socketDescriptor, "550 Sorry, but that file doesn't exist ");
        }
    }
    else
    {
        returnCode = printf(theClientData->socketDescriptor, "550 Sorry, but that file doesn't exist ");
    }
    

    if (returnCode <= 0) return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    return FTP_COMMAND_PROCESSED;
}

int parseCommandRnto(clientDataType *theClientData)
{
    int returnCode;
    int isSafePath;
    char *theRntoFileName;

    theRntoFileName = getFtpCommandArg("RNTO", theClientData->theCommandReceived, 0);
    cleanDynamicStringDataType(&theClientData->renameToFile, 0);
    
    isSafePath = getSafePath(&theClientData->renameToFile, theRntoFileName, &theClientData->login);

    if (isSafePath == 1 &&
        theClientData->renameFromFile.textLen > 0)
    {

        if (FILE_IsFile(theClientData->renameFromFile.text) == 1 ||
            FILE_IsDirectory(theClientData->renameFromFile.text) == 1)
        {
            int returnCode = 0;
            returnCode = rename (theClientData->renameFromFile.text, theClientData->renameToFile.text);
            if (returnCode == 0) 
            {
                returnCode = printf(theClientData->socketDescriptor, "250 File successfully renamed or moved ");
            }
            else
            {
                returnCode = printf(theClientData->socketDescriptor, "503 Error Renaming the file ");
            }
        }
        else
        {
            returnCode = printf(theClientData->socketDescriptor, "503 Need RNFR before RNTO ");
        }
    }
    else
    {
        returnCode = printf(theClientData->socketDescriptor, "503 Error Renaming the file ");
    }

    if (returnCode <= 0) return FTP_COMMAND_PROCESSED_WRITE_ERROR;
    return FTP_COMMAND_PROCESSED;
}

int parseCommandCdup(clientDataType *theClientData)
{
    int returnCode;

    FILE_DirectoryToParent(&theClientData->login.absolutePath.text);
    FILE_DirectoryToParent(&theClientData->login.ftpPath.text);
    theClientData->login.absolutePath.textLen = strlen(theClientData->login.absolutePath.text);
    theClientData->login.ftpPath.textLen = strlen(theClientData->login.ftpPath.text);

    if(strncmp(theClientData->login.absolutePath.text, theClientData->login.homePath.text, theClientData->login.homePath.textLen) != 0)
    {
        setDynamicStringDataType(&theClientData->login.absolutePath, theClientData->login.homePath.text, theClientData->login.homePath.textLen);
    }

    returnCode = printf(theClientData->socketDescriptor, "250 OK. Current directory is %s ", theClientData->login.ftpPath.text);

    if (returnCode <= 0) 
        return FTP_COMMAND_PROCESSED_WRITE_ERROR;

    return FTP_COMMAND_PROCESSED;
}

long long int writeRetrFile(char * theFilename, int thePasvSocketConnection, long long int startFrom, FILE *retrFP)
{
    long long int readen = 0;
    long long int toReturn = 0, writtenSize = 0;
    long long int currentPosition = 0;
    long long int theFileSize;
    char buffer[FTP_COMMAND_ELABORATE_CHAR_BUFFER];

    #ifdef _LARGEFILE64_SOURCE
        retrFP = fopen64(theFilename, "rb");
    #endif

    #ifndef _LARGEFILE64_SOURCE
        retrFP = fopen(theFilename, "rb");
    #endif

    if (retrFP == NULL)
    {
        return -1;
    }

    theFileSize = FILE_GetFileSize(retrFP);

    if (startFrom > 0)
    {

        #ifdef _LARGEFILE64_SOURCE
            currentPosition = (long long int) lseek64(fileno(retrFP), startFrom, SEEK_SET);
        #endif

        #ifndef _LARGEFILE64_SOURCE
            currentPosition = (long long int) lseek(fileno(retrFP), startFrom, SEEK_SET);
        #endif

        if (currentPosition == -1)
        {
            fclose(retrFP);
            retrFP = NULL;
            return toReturn;
        }
    }

    while ((readen = (long long int) fread(buffer, sizeof(char), FTP_COMMAND_ELABORATE_CHAR_BUFFER, retrFP)) > 0)
    {
      writtenSize = write(thePasvSocketConnection, buffer, readen);
 
      if (writtenSize <= 0)
      {
          fclose(retrFP);
          retrFP = NULL;
          return -1;
      }
      else
      {
            toReturn = toReturn + writtenSize;
      }
    }
    fclose(retrFP);
    retrFP = NULL;
    return toReturn;
}

char *getFtpCommandArg(char * theCommand, char *theCommandString, int skipArgs)
{
    char *toReturn = theCommandString + strlen(theCommand);

   /* Pass spaces */ 
    while (toReturn[0] == ' ')
    {
        toReturn += 1;
    }

    /* Skip eventual secondary arguments */
    if(skipArgs == 1)
    {
        if (toReturn[0] == '-')
        {
            while (toReturn[0] != ' ' &&
                   toReturn[0] != '\r' &&
                   toReturn[0] != '\n' &&
                   toReturn[0] != 0)
                {
                    toReturn += 1;
                }
        }

        /* Pass spaces */ 
        while (toReturn[0] == ' ')
        {
            toReturn += 1;
        }
    }

    return toReturn;
}

int getFtpCommandArgWithOptions(char * theCommand, char *theCommandString, ftpCommandDataType *ftpCommand)
{
    #define CASE_ARG_MAIN   0
    #define CASE_ARG_SECONDARY   1

    int i = 0;
    int isSecondArg = 0;
    char argMain[FTP_COMMAND_ELABORATE_CHAR_BUFFER];
    char argSecondary[FTP_COMMAND_ELABORATE_CHAR_BUFFER];
    memset (argMain, 0, FTP_COMMAND_ELABORATE_CHAR_BUFFER);
    memset (argSecondary, 0, FTP_COMMAND_ELABORATE_CHAR_BUFFER);

    int argMainIndex = 0;
    int argSecondaryIndex = 0;

    char *theCommandPointer = theCommandString;
    theCommandPointer += strlen(theCommand);
    
    /*Remove spaces*/
    while (theCommandPointer[0] == ' ')
    {
        theCommandPointer += 1;
    }
    
    if (theCommandPointer[i] == '-') 
    {
        isSecondArg = 1;
        theCommandPointer++;
    }
    
    while (theCommandPointer[i] != '\r' &&
           theCommandPointer[i] != '\n' &&
           theCommandPointer[i] != 0)
    {
        switch(isSecondArg)
        {
            case CASE_ARG_MAIN:
            {
                if (argMainIndex < FTP_COMMAND_ELABORATE_CHAR_BUFFER)
                    argMain[argMainIndex++] = theCommandPointer[i];
                i++;
            }
            break;
                
            case CASE_ARG_SECONDARY:
            {
                if (theCommandPointer[i] != ' ')
                {
                    if (argSecondaryIndex < FTP_COMMAND_ELABORATE_CHAR_BUFFER)
                        argSecondary[argSecondaryIndex++] = theCommandPointer[i];
                    i++;
                }
                else 
                {
                    isSecondArg = 0;
                    while (theCommandPointer[0] == ' ')
                    {
                        theCommandPointer += 1;
                    }
                }
            }
            break;
        }
    }
    
    if (argMainIndex > 0)
        setDynamicStringDataType(&ftpCommand->commandArgs, argMain, argMainIndex);

    if (argSecondaryIndex > 0)
        setDynamicStringDataType(&ftpCommand->commandOps, argSecondary, argSecondaryIndex);
        
    return 1;
}

int setPermissions(char * permissionsCommand, char * basePath, ownerShip_DataType ownerShip)
{
    #define STATUS_INCREASE 0
    #define STATUS_PERMISSIONS 1
    #define STATUS_LOCAL_PATH 2

    int permissionsCommandCursor = 0;
    int returnCode = 0;

    int status = STATUS_INCREASE;
    char thePermissionString[1024];
    char theLocalPath[1024];
    char theFinalFilename[2048];
    int returnCodeSetPermissions, returnCodeSetOwnership;

    memset(theLocalPath, 0, 1024);
    memset(thePermissionString, 0, 1024);
    memset(theFinalFilename, 0, 2048);
    int thePermissionStringCursor = 0, theLocalPathCursor = 0;

    while (permissionsCommand[permissionsCommandCursor] != '\r' &&
           permissionsCommand[permissionsCommandCursor] != '\n' &&
           permissionsCommand[permissionsCommandCursor] != '\0')
    {
        switch (status)
        {
            case STATUS_INCREASE:
                if (permissionsCommandCursor == strlen("chmod"))
                {
                    status = STATUS_PERMISSIONS;
                }
            break;
            
            case STATUS_PERMISSIONS:
                if (permissionsCommand[permissionsCommandCursor] == ' ')
                {
                    status = STATUS_LOCAL_PATH;
                    break;
                }
                if (thePermissionStringCursor < 1024 )
                    thePermissionString[thePermissionStringCursor++] = permissionsCommand[permissionsCommandCursor];
                else
                    return FTP_CHMODE_COMMAND_RETURN_NAME_TOO_LONG;
            break;
            
            case STATUS_LOCAL_PATH:
                    if (theLocalPathCursor < 1024)
                        theLocalPath[theLocalPathCursor++] = permissionsCommand[permissionsCommandCursor];
                    else
                        return FTP_CHMODE_COMMAND_RETURN_NAME_TOO_LONG;
            break;
        }

        permissionsCommandCursor++;
    }

    memset(theFinalFilename, 0, 2048);

    if ((strlen(basePath) + strlen(theLocalPath) + 2) >= 2048)
    {
        return FTP_CHMODE_COMMAND_RETURN_NAME_TOO_LONG;
    }

    if (basePath[strlen(basePath)-1] != '/') 
    {
        sprintf(theFinalFilename, "%s/%s", basePath, theLocalPath);
    }
    else 
    {
        sprintf(theFinalFilename, "%s%s", basePath, theLocalPath);
    }
    
    if (FILE_IsFile(theFinalFilename) != 1 && 
        FILE_IsDirectory(theFinalFilename) != 1)
    {
        return FTP_CHMODE_COMMAND_RETURN_CODE_NO_FILE;
    }

    if (ownerShip.ownerShipSet == 1)
    {
        returnCodeSetOwnership = FILE_doChownFromUidGid(theFinalFilename, ownerShip.uid, ownerShip.gid);
    }

    returnCode = strtol(thePermissionString, 0, 8);
    if (returnCodeSetPermissions = chmod (theFinalFilename, returnCode) < 0)
    {
        printf(" ---> ERROR WHILE SETTING FILE PERMISSION");
    }

    if (returnCodeSetOwnership != 1 || returnCodeSetPermissions == -1)
    {
        return FTP_CHMODE_COMMAND_RETURN_CODE_NO_PERMISSIONS;
    }

    return FTP_CHMODE_COMMAND_RETURN_CODE_OK;
}
