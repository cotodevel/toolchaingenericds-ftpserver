#include "serverconnection.h"
#include "ftpserver.h"

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "fsfatlayerTGDS.h"

#include "dswnifi_lib.h"

// Destructor, clean up all the mess
serverconnection::~serverconnection() {
    printf("Connection terminated to client (connection id %d)", this->connectionId);
    delete this->fo;
    close(this->fd);
    this->directories.clear();
    this->files.clear();
}

// Constructor
serverconnection::serverconnection(int filedescriptor, unsigned int connId, std::string defaultDir, std::string hostId, unsigned short commandOffset) : fd(filedescriptor), connectionId(connId), dir(defaultDir), hostAddress(hostId), commandOffset(commandOffset), closureRequested(false), uploadCommand(false), downloadCommand(false),  receivedPart(0), parameter("") {
//    this->files = std::vector<std::string>();
    this->fo = new fileoperator(this->dir); // File and directory browser
    printf("Connection to client %s established", this->hostAddress.c_str());
}

// Check for matching (commands/strings) with compare method
bool serverconnection::commandEquals(std::string a, std::string b) {
    // Convert to lower case for case-insensitive checking
    std::transform(a.begin(), a.end(),a.begin(), tolower);
    int found = a.find(b);
    return (found!=std::string::npos);
}

// Command switch for the issued client command, only called when this->command is set to 0
std::string serverconnection::commandParser(std::string command) {
    this->parameter;
    std::string res = "";
    this->uploadCommand = false;
    struct stat Status;
    // Commands can have either 0 or 1 parameters, e.g. 'browse' or 'browse ./'
    std::vector<std::string> commandAndParameter = this->extractParameters(command);
//    for (int i = 0; i < parameters.size(); i++) printf("P %d : %s ", i, parameters.at(i).c_str());
    printf("Connection %d: ", this->connectionId);

    /// @TODO: Test if prone to DOS-attacks (if loads of garbage is submitted)???
    // If command with no argument was issued
    if (commandAndParameter.size() == 1) {
        if (this->commandEquals(commandAndParameter.at(0), "list")) {
            // dir to browse
            std::string curDir = "/";
			printf("Browsing files of the current working dir");
            this->directories.clear();
            this->files.clear();
            this->fo->browse(curDir,directories,files);
            for (unsigned int j = 0; j < directories.size(); j++) {
                res += directories.at(j) + "/\n";
            }
            for (unsigned int i = 0; i < files.size(); i++) {
                res += files.at(i) + "\n";
            }
        } else
        if (this->commandEquals(commandAndParameter.at(0), "pwd")) { // Returns the current working directory on the server
            printf("Working dir requested");
            res = this->fo->getCurrentWorkingDir(false);
        } else
        if (this->commandEquals(commandAndParameter.at(0), "getparentdir")) { // Returns the parent directory of the current working directory on the server
            printf("Parent dir of working dir requested");
			res = this->fo->getParentDir();
        } else
        if (this->commandEquals(commandAndParameter.at(0), "bye") || this->commandEquals(commandAndParameter.at(0), "quit")) {
            printf("Shutdown of connection requested");
            this->closureRequested = true;
    //        close (this->fd);
        } else {
        // Unknown / no command / enter
            printf("Unknown command encountered!");
			commandAndParameter.clear();
        }
    } else // end of commands with no arguments
    // Command with a parameter received
    if (commandAndParameter.size() > 1) {
        // The parameter
        this->parameter = commandAndParameter.at(1);        
        if (this->commandEquals(commandAndParameter.at(0), "ls")) {
            // read out dir to browse
            std::string curDir = std::string(commandAndParameter.at(1));
            printf("Browsing files of directory %s", curDir.c_str());
			this->directories.clear();
            this->files.clear();
            this->fo->browse(curDir,directories,files);
            for (unsigned int j = 0; j < directories.size(); j++) {
                res += directories.at(j) + "/\n";
            }
            for (unsigned int i = 0; i < files.size(); i++) {
                res += files.at(i) + "\n";
            }
        } else
        if (this->commandEquals(commandAndParameter.at(0), "download")) {
            this->downloadCommand = true;
            printf("Preparing download of file  %s", this->parameter.c_str());
            unsigned long lengthInBytes = 0;
            char* fileBlock;
            unsigned long readBytes = 0;
            std::stringstream st;
            if (!this->fo->readFile(this->parameter)) {
                // Read the binary file block-wise
//                do {
                    st.clear();
                    fileBlock = this->fo->readFileBlock(lengthInBytes);
                    st << lengthInBytes;
                    readBytes += lengthInBytes;
//                    this->sendToClient(st.str()); // First, send length in bytes to client
                    this->sendToClient(fileBlock,lengthInBytes); // This sends the binary char-array to the client
//                    fileoperator* fn = new fileoperator(this->dir);
//                    fn->writeFileAtOnce("./test.mp3",fileBlock);
//                } while (lengthInBytes <= readBytes);
            }
            this->closureRequested = true; // Close connection after transfer
        } else
        if (this->commandEquals(commandAndParameter.at(0), "upload")) {
            this->uploadCommand = true; // upload hit!
            printf("Preparing download of file  %s", this->parameter.c_str());
			// all bytes (=parameters[2]) after the upload <file> command belong to the file
            res = this->fo->beginWriteFile(this->parameter);
//            res = (this->fo->beginWriteFile(this->parameter) ? "Upload failed" : "Upload successful");
        } else
        if (this->commandEquals(commandAndParameter.at(0), "cd")) { // Changes the current working directory on the server
            printf("Change of working dir to %s requested", this->parameter.c_str());
			// Test if dir exists
            if (!this->fo->changeDir(this->parameter)) {
                printf("Directory change to  %s successful!", this->parameter.c_str());
			}
            res = this->fo->getCurrentWorkingDir(false); // Return current directory on the server to the client (same as pwd)
        } else
        if (this->commandEquals(commandAndParameter.at(0), "rmdir")) {
            printf("Deletion of dir  %s requested ", this->parameter.c_str());
			if (this->fo->dirIsBelowServerRoot(this->parameter)) {
                printf("Attempt to delete directory beyond server root (prohibited)");
                res = "//"; // Return some garbage as error indication :)
            } else {
                this->directories.clear(); // Reuse directories to spare memory
                this->fo->clearListOfDeletedDirectories();
                this->files.clear(); // Reuse files to spare memory
                this->fo->clearListOfDeletedFiles();
                if (this->fo->deleteDirectory(this->parameter)) {
                    printf("Error when trying to delete directory %s ", this->parameter.c_str());
                }
                this->directories = this->fo->getListOfDeletedDirectories();
                this->files = this->fo->getListOfDeletedFiles();
                for (unsigned int j = 0; j < directories.size(); j++) {
                    res += directories.at(j) + "\n";
                }
                for (unsigned int i = 0; i < files.size(); i++) {
                    res += files.at(i) + "\n";
                }
            }
        } else
        if (this->commandEquals(commandAndParameter.at(0), "delete")) {
            printf("Deletion of file %s requested ", this->parameter.c_str());
			this->fo->clearListOfDeletedFiles();
            if (this->fo->deleteFile(this->parameter)) {
                res = "//";
            } else {
                std::vector<std::string> deletedFile = this->fo->getListOfDeletedFiles();
                if (deletedFile.size() > 0)
                    res = deletedFile.at(0);
            }
        } else
        if (this->commandEquals(commandAndParameter.at(0), "getsize")) {
            printf("Size of file %s requested ", this->parameter.c_str());
			std::vector<std::string> fileStats = this->fo->getStats(this->parameter, Status);
            res = fileStats.at(4); // file size or content count of directory
        } else
        if (this->commandEquals(commandAndParameter.at(0), "getaccessright")) {
            printf("Access rights of file %s requested ", this->parameter.c_str());
			std::vector<std::string> fileStats = this->fo->getStats(this->parameter, Status);
            res = fileStats.at(0); // unix file / directory permissions
        } else
        if (this->commandEquals(commandAndParameter.at(0), "getlastmodificationtime")) {
            printf("Last modification time of file %s requested ", this->parameter.c_str());
			std::vector<std::string> fileStats = this->fo->getStats(this->parameter, Status);
            res = fileStats.at(3); // unix file / directory permissions
        } else
        if (this->commandEquals(commandAndParameter.at(0), "getowner")) {
            printf("Owner of file %s requested ", this->parameter.c_str());
			std::vector<std::string> fileStats = this->fo->getStats(this->parameter, Status);
            res = fileStats.at(2); // owner
        } else
        if (this->commandEquals(commandAndParameter.at(0), "getgroup")) {
            printf("Group of file %s requested ", this->parameter.c_str());
			std::vector<std::string> fileStats = this->fo->getStats(this->parameter, Status);
            res = fileStats.at(1); // group
        } else
        if (this->commandEquals(commandAndParameter.at(0), "mkdir")) { // Creates a directory of the specified name in the current server working dir
            printf("Creating of dir %s requested ", this->parameter.c_str());
			res = (this->fo->createDirectory(this->parameter) ? "//" : this->parameter); // return "//" in case of failure
        } else
        if (this->commandEquals(commandAndParameter.at(0), "touch")) { // Creates an empty file of the specified name in the current server working dir
            printf("Creating of empty file %s requested", this->parameter.c_str());
			res = (this->fo->createFile(this->parameter) ? "//" : this->parameter);  // return "//" in case of failure
        } else {
        // Unknown / no command
            printf("Unknown command encountered!");
			commandAndParameter.clear();
            command = "";
            res = "ERROR: Unknown command!";
        }
    } else // end of command with one parameter
    // No command / enter
    if (!commandAndParameter.at(0).empty()) {
        printf("Unknown command encountered!");
        commandAndParameter.clear();
    }
    res += "\n";
    return res;
}

// Extracts the command and parameter (if existent) from the client call
std::vector<std::string> serverconnection::extractParameters(std::string command) {
    std::vector<std::string> res = std::vector<std::string>();
    std::size_t previouspos = 0;
    std::size_t pos;
    // First get the command by taking the string and walking from beginning to the first blank
    if ((pos = command.find(SEPARATOR, previouspos)) != std::string::npos) { // No empty string
        res.push_back(command.substr(int(previouspos),int(pos-previouspos))); // The command
//        printf("command %s!",res.back().c_str());
    }
    if (command.length() > (pos+1)) {
        //For telnet testing commandOffset = 3 because of the enter control sequence at the end of the telnet command (otherwise = 1)
        res.push_back(command.substr(int(pos+1),int(command.length()-(pos+(this->commandOffset))))); // The parameter (if existent)
//        res.push_back(command.substr(int(pos+1),int(command.length()-(pos+3)))); // The parameter (if existent)
//        printf("- Parameter: %s",res.back().c_str());
    }
    return res;
}

// Receives the incoming data and issues the apropraite commands and responds
void serverconnection::respondToQuery() {
    char buffer[BUFFER_SIZE];
    int bytes;
    bytes = recv(this->fd, buffer, sizeof(buffer), 0);
    // In non-blocking mode, bytes <= 0 does not mean a connection closure!
    if (bytes > 0) {
        std::string clientCommand = std::string(buffer, bytes);
		//        printf(" %s", command);
        if (this->uploadCommand) { // (Previous) upload command
		//        printf("Schreibe block ");
            /// Previous (upload) command continuation, store incoming data to the file
            printf("Part %d :", ++(this->receivedPart));
            this->fo->writeFileBlock(clientCommand);
        } else {
            // If not upload command issued, parse the incoming data for command and parameters
            std::string res = this->commandParser(clientCommand);
//            if (!this->downloadCommand) {
                this->sendToClient(res); // Send response to client if no binary file
//              this->downloadCommand = false;
//            }
        }
    } else { // no bytes incoming over this connection
        if (this->uploadCommand) { // If upload command was issued previously and no data is left to receive, close the file and connection
//            this->fo->closeWriteFile();
            this->uploadCommand = false;
            this->downloadCommand = false;
            this->closureRequested = true;
            this->receivedPart = 0;
        }
    }
}

// Sends the given string to the client using the current connection
void serverconnection::sendToClient(char* response, unsigned long length) {
    // Now we're sending the response
    unsigned int bytesSend = 0;
    while (bytesSend < length) {
        int ret = send(this->fd, response+bytesSend, length-bytesSend, 0);
        if (ret <= 0) {
            return;
        }
        bytesSend += ret;
    }
}

// Sends the given string to the client using the current connection
void serverconnection::sendToClient(std::string response) {
    // Now we're sending the response
    unsigned int bytesSend = 0;
    while (bytesSend < response.length()) {
        int ret = send(this->fd, response.c_str()+bytesSend, response.length()-bytesSend, 0);
        if (ret <= 0) {
            return;
        }
        bytesSend += ret;
    }
}

// Returns the file descriptor of the current connection
int serverconnection::getFD() {
    return this->fd;
}

// Returns whether the connection was requested to be closed (by client)
bool serverconnection::getCloseRequestStatus() {
    return this->closureRequested;
}

unsigned int serverconnection::getConnectionId() {
    return this->connectionId;
}
