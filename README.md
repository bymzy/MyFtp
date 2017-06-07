# MyFtp
This is a simple Ftp-like program on linux with python client.

## Target
Sometimes it is annoying to scp files from one machine to another if you use scp(cause you need password :)). So why not make one simple program to handles files between machines.

## Features
- get files from server side (supported!)
- put files to server side (supported!)
- del files on server side (supported!)
- list files on server side (supported!)
- mkdir on server side (not finished yet)
- deldir on server side (not finished yet)
- chdir on server side (not finished yet, this is useful in interactive mode!) 


## TODO
- DONE cmdline support     
- DONE logfile support
- DONE daemon
- DONE server ip, port can be assigned from cmdline
- DONE server work dir can be assigned from cmdline

## Dependencies
- openssl-devel
- libcrypto

## Something more
My job needs to copy file from CenOS to windows, i found i stupid user XFTP to transfer file, so i make this program and write some other scripts to copy automatic.

