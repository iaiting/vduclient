# VDUClient

![VDUClient UI](https://i.imgur.com/wLT9izo.png)

## Requirements

Microsoft Visual C++ Redistributable 2015-2019

WinFSP 1.8 or newer

Windows 7 SP1 and newer

## Installation

1.) Download and install latest WinFSP (http://www.secfs.net/winfsp/rel/) and Visual C++ 15-19 Redistributable libraries (https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0)

1a.) For Windows 7 SP1, download and install https://www.microsoft.com/en-us/download/details.aspx?id=46148

2.) Grab a release version of `VDUClient.exe` and run (Debug version requires Visual Studio debug libraries)

## Setting up developing environment

Requires Visual Studio 2019, Windows 10 SDK, Build tools v142, MFC+ATL for build tools v142

1.) Install WinFsP with the Developer Option selected during installation

2.) Download VDUClient repository, launch `.sln` file

3.) Compile as Release/x64

4.) Run `VDUClient.exe`

NOTE: If the include file `winfsp.hpp` wont be pre-installed in some cases, you need to manually download it from https://raw.githubusercontent.com/billziss-gh/winfsp/master/inc/winfsp/winfsp.hpp and copy into your `WinFsp\inc\winfsp` installation folder!

## Launch options

- `-silent`       Program window starts up hidden and minimized to tray
- `-testmode`     Program starts in test mode, where it reads actions and executes them. For more information see contents of 'test.py'
- `-insecure` Disables certificate validation - for debug/test purposes

# VDU Mock Server

## Requirements

Python3.

## Setting up the server

1.) Edit server settings at the top of the file to your liking

1.) Run `vdusrv.py` - Runs on all interfaces on port 4443 by default

# Testing

![VDU Tests](https://i.imgur.com/poxPXVv.png)

## Requirements

Python 3.6.3 or newer

## Setting up for testing

1.) Make sure to not have any instance of VDU Client or VDU server running, before running the test.

2.) Run `test.py` in the root directory

Test is set up to be ran from that directory. To change path to client executable and server script, you can edit the top of the file.
To add new tests or edit the settings of the testing suite, modify the file

## Adding custom tests

In order to add custom tests, get familiar with the Action list, create your test and then simply add it to the list of the tests

#### Action list

- `-server [ip]`              Set server ip to 'ip'
- `-user [login]`             Log in as 'login'
- `-accessfile [token]`       Accesses a file
- `-deletefile [token]`       Invalidates token
- `-rename [token] [name]`    Renames file to 'name'
- `-logout`                   Logs out current user
- `-write [token] [text]`     Writes text at the beginning of a file
- `-read [token] [cmpText]`   Reads text of the length of 'cmpText' from the beginning of a file and compares them 
# Credits

Client developed using Microsoft Visual Studio 2019 Community with valid Free Licence, using MFC/ATL Build tools v142 and Windows 10.0 SDK, at https://visualstudio.microsoft.com/cs/vs/community/

Server developed using Microsoft Visual Studio Code, with Python 3.9.4 environment, at https://code.visualstudio.com/

WinFsP API usage in courtesy of `billziss-gh`, GitHub page at https://github.com/billziss-gh/winfsp, Website at www.secfs.net/winfsp/

WinFSP Virtual File System usage based on passthrough-cpp by `billziss-gh` at https://github.com/billziss-gh/winfsp/tree/master/tst/passthrough-cpp

Cloud Storage icon by https://icons8.com at https://icons8.com/icon/r8kHwiV6nVEd/cloud-storage

Function descriptions and help in various areas of Desktop Development using C++, Windows API and MFC/ATL API at https://docs.microsoft.com/
