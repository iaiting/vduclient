# VDUClient

![VDUClient UI](https://i.imgur.com/wLT9izo.png)

## Requirements
Microsoft Visual C++ Redistributable 2015-2019
WinFSP 1.8+

## Installation

1.) Download and install latest WinFSP (http://www.secfs.net/winfsp/rel/) and Visual C++ (https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0)

2.) Grab a release version of VDUClient and run (Debug version requires Visual Studio debug libraries)

## Setting up developing environment
Requires Visual Studio 2019, Build tools v142, MFC+ATL

1.) Install WinFSP with Developer Option (selectable during installation)

2.) Download VDUClient solution, launch .sln file

3.) Compile as release/debug x64

4.) Run `VDUClient.exe`

NOTE: If winfsp.hpp wont be pre-installed, you need to manually download it from https://raw.githubusercontent.com/billziss-gh/winfsp/master/inc/winfsp/winfsp.hpp and copy into your `WinFsp\inc\winfsp` folder!

## Launch options
- `-silent`       Program window starts up hidden and minimized to tray
- `-testmode`     Program starts in test mode, where it reads actions and executes them. For more information see `test.py`

# VDU Example Dev-Server

## Requirements
Python3

## Setting up the server
Make sure to `pip install` required libraries

1.) Edit server settings at the top of the file to your liking
1.) Run `vdusrv.py` - Runs on all interfaces on port 4443 by default

# Testing

![VDU Tests](https://i.imgur.com/poxPXVv.png)

## Requirements
Python3

## Setting up for testing
Make sure to `pip install` required libraries

1.) Edit test settings at the top of the file to your liking
2.) Run `test.py` - Checkmarks mean passed, Crosses mean did not pass

## Adding custom tests
In order to add custom tests, get familiar with the Action list, create your test and then simply add it to the list of the tests
#### Action list
- `-server [ip]`              Set server ip to 'ip'
- `-user [login]`             Log in as 'login'
- `-accessfile [token]`       Accesses a file
- `-deletefile [token]`       Invalidates token
- `-rename [token] [name]`    Renames file to 'name'
- `-logout`                   Logs out current user
- `-write [token] [text]`     Writes text to a file

# Credits
Client developed using Microsoft Visual Studio 2019 Community with valid Free Licence, using MFC/ATL Build tools v142 and Windows 10.0 SDK, at https://visualstudio.microsoft.com/cs/vs/community/

Server developed using Microsoft Visual Studio Code, with Python3.9 environment, at https://code.visualstudio.com/

WinFSP API usage in courtesy of `billziss-gh`, GitHub page at https://github.com/billziss-gh/winfsp, Website at www.secfs.net/winfsp/
WinFSP FileSystem usage based on passthrough-cpp by `billziss-gh` at https://github.com/billziss-gh/winfsp/tree/master/tst/passthrough-cpp

Cloud Storage icon by https://icons8.com at https://icons8.com/icons/set/cloud-storage

Function descriptions and help in various areas of Desktop Development using C++, Windows API and MFC/ATL API at https://docs.microsoft.com/
