# VDUClient

![VDUClient UI](https://i.imgur.com/GTul0K3.png)

## Requirements
WinFSP 1.8+

## Installation

1.) Download and install latest WinFSP (http://www.secfs.net/winfsp/rel/)

2.) Grab a release version of VDUClient and run (Debug version requires Visual Studio debug libraries)

## Setting up developing environment
Requires Visual Studio 2019, Build tools v142, MFC+ATL

1.) Install WinFSP with Developer Option (selectable during installation)

2.) Download VDUClient solution, launch .sln file

3.) Compile as release/debug x64

4.) Run `VDUClient.exe`

NOTE: If winfsp.hpp wont be pre-installed, you need to manually download it from https://raw.githubusercontent.com/billziss-gh/winfsp/master/inc/winfsp/winfsp.hpp and copy into your `WinFsp\inc\winfsp` folder!

# VDU Example Dev-Server

## Requirements
Python3

## Setting up the server
Make sure to `pip install` required libraries

1.) Edit server settings at the top of the file to your liking
1.) Run `vdusrv.py` - Runs on all interfaces on port 4443 by default
