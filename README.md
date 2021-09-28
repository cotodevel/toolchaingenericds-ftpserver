![ToolchainGenericDS](img/TGDS-Logo.png)

Branch: TGDS1.64-ARM7Sound - plays ADPCM BG music, but while a transfer takes place ARM7 dies

This is the ToolchainGenericDS Woopsi FTP Server project:

1.	Compile Toolchain:
To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds :
Then simply extract the project somewhere.

2.	Compile this project: 
Open msys, through msys commands head to the directory your extracted this project.
Then write:
make clean <enter>
make <enter>

After compiling, run the example in NDS. 

Project Specific description:

Finally a TGDS FTP Server! AUTH is not supported. It means only old insecure FTP Server mode.
Settings to connect to FTP Server:
user: anonymous
pass:
port: 21

Sending files to FTP Server is not so slow! I mean I can upload a 800K file in about 20 seconds. On the other hand, retrieving files from FTP Server is painfully slow.
This has actually sped up most DS development I am doing. Since I can now just use FTP to transfer binaries to DS!

Launching .nds files over FTP:
Simply send a .nds file over FTP, and it will be automatically launched.

Changelog:
Alpha 0.2
- Add ToolchainGenericDS-multiboot (https://bitbucket.org/Coto88/toolchaingenericds-multiboot) loader code (ability to boot .NDS, homebrew files)

Alpha 0.1:
- Add FTP Server.

Bugs/Notes:
- Navigating between directories might cause invalid directory listing, but it works otherwise.
- Opening/closing data sockets in DSWIFI is very unstable. That means there could be hangs between directory listing and/or receiving or sending files. Run again the binary if this happens.
- Currently it works with WinSCP. On filezilla I have no idea why it doesn't work, we'll see what I can do about it.



Coto