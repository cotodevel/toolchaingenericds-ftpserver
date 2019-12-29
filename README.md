This is the ToolchainGenericDS FTP Server project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.
Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make
After compiling, run the example in NDS.

Project Specific description: 

Finally a TGDS FTP Server! AUTH is not supported. It means only old insecure FTP Server mode.
Settings to connect to FTP Server:
user: anonymous
pass:
port: 21

Current version :
Alpha 0.1

Sending files to FTP Server is not so slow! I mean I can upload a 800K file in about 20 seconds. On the other hand, retrieving files from FTP Server is painfully slow.
This has actually sped up most DS development I am doing. Since I can now just use FTP to transfer binaries to DS!

Bugs/Notes:
- It only lists the root directory. Usually the TGDS FS calls work fine in other projects, but here it behaves weirdly. Other FTP operations are working fine, though.
- Currently it works with WinSCP. On filezilla I have no idea why it doesn't work, we'll see what I can do about it.
- Opening/closing data sockets in DSWIFI is very unstable. That means there could be hangs between directory listing and/or receiving or sending files. Run again the binary if this happens.


Enjoy.

Coto
