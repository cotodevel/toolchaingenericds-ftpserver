This is the Toolchain Generic FTP project:

It is based on the Open Source FTP library from: https://github.com/kingk85/uFTP and added the TGDS layer so it runs on the NintendoDS.

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the example in NDS.

Project Specific description: 
currently in development.


//todos:

umask -> change file/dir permissions in linux

-

readlink ->

readlink -f will return 0 for a non-existent file in an existing directory whereas realpath returns 1. However, readlink -e will behave like realpath and return 1 for a non-existent file.

readlink -f
$ readlink -f non-existent-file
/home/user/non-existent-file
$ echo $?
0
readlink -e
$ readlink -e non-existent-file
$ echo $?
1

-

-> getrlimit

-

-> setsid

-

-> sigaction

-

-> lstat

-

-> getgrgid

-

-> getgrnam

Coto
