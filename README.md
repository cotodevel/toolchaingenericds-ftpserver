![ToolchainGenericDS](img/TGDS-Logo.png)

NTR/TWL SDK: TGDS1.65

master: Development branch. Use TGDS1.65: branch for stable features.
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


____Remoteboot____
Also, it's recommended to use the remoteboot feature. It allows to send the current TGDS Project over wifi removing the necessity
to take out the SD card repeteadly and thus, causing it to wear out and to break the SD slot of your unit.

Usage:
- Make sure the wifi settings in the NintendoDS are properly set up, so you're already able to connect to internet from it.

- Get a copy of ToolchainGenericDS-multiboot: https://bitbucket.org/Coto88/ToolchainGenericDS-multiboot/get/TGDS1.65.zip
Follow the instructions there and get either the TWL or NTR version. Make sure you update the computer IP address used to build TGDS Projects, 
in the file: toolchaingenericds-multiboot-config.txt of said repository before moving it into SD card.

For example if you're running NTR mode (say, a DS Lite), you'll need ToolchainGenericDS-multiboot.nds, tgds_multiboot_payload_ntr.bin
and toolchaingenericds-multiboot-config.txt (update here, the computer's IP you use to build TGDS Projects) then move all of them to root SD card directory.

- Build the TGDS Project as you'd normally would, and run these commands from the shell.
<make clean>
<make>

- Then if you're on NTR mode:
<remoteboot ntr_mode computer_ip_address>

- Or if you're on TWL mode:
<remoteboot twl_mode computer_ip_address>

- And finally boot ToolchainGenericDS-multiboot, and press (X), wait a few seconds and TGDS Project should boot remotely.
  After that, everytime you want to remoteboot a TGDS Project, repeat the last 2 steps. ;-)




/release folder has the latest binary precompiled for your convenience.

Latest stable release: https://bitbucket.org/Coto88/ToolchainGenericDS-FTPServer/get/TGDS1.65.zip

Bugs/Notes:
- Navigating between directories might cause invalid directory listing, but it works otherwise.
- Opening/closing data sockets in DSWIFI is very unstable. That means there could be hangs between directory listing and/or receiving or sending files. Run again the binary if this happens.
- Currently it works with WinSCP. On filezilla I have no idea why it doesn't work, we'll see what I can do about it.


Coto