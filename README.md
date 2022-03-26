# VDUClient

![VDUClient UI](https://i.imgur.com/5drCMmt.png)

## About

This is a client for the VDU (Validated Data storage Unit) server. It allows users to store and retrieve documents from the server and creates a virtual disk containing a virtual file system where the documents are stored and handled automatically. It provides various features for version control, file sharing, and document management and customisation. All retrieved files are seamlessly available to the user inside the new virtual disk, and changes to them are transmitted to the server automatically without the need for manual intervention.

## Requirements

Microsoft Visual C++ Redistributable 2015-2022

WinFSP 1.8 or newer

Windows 7 SP1 or newer (Supports Windows 11)

## Installation

1.) Download and install latest WinFSP (<http://www.secfs.net/winfsp/rel/>) and the latest Visual C++ 2015-2022 Redistributable libraries (<https://aka.ms/vs/17/release/vc_redist.x64.exe> or <https://aka.ms/vs/17/release/vc_redist.x86.exe>)

1a.) For Windows 7 SP1 it might be required download and install <https://www.microsoft.com/en-us/download/details.aspx?id=46148>

2.) Run a Release version of `VDUClient.exe` of the architecture of your machine (Win32 or x64)

## Setting up development environment

Requires Visual Studio 2022, Windows 10 SDK, Build tools v143, MFC+ATL for build tools v143

1.) Install WinFsp with the Developer Option selected during installation

2.) Download VDUClient repository, launch `.sln` file

3.) Compile as Release, pick your architecture (Win32 or x64)

4.) Run `VDUClient.exe`

NOTE: If the include file `winfsp.hpp` wont be pre-installed in some cases, you need to manually download it from <https://raw.githubusercontent.com/winfsp/winfsp/master/inc/winfsp/winfsp.hpp> and copy into your `WinFsp\inc\winfsp` installation folder!

## Launch options

- `-silent`       Program window starts up hidden and minimized to tray
- `-testmode`     Program starts in test mode, where it reads actions and executes them. For more information see contents of 'test.py'
- `-insecure` Disables certificate validation - for debug/test purposes

## VDU URI protocol

The application registers a new URI protocol in the format of `vdu://{token}`, which can be used to trigger the running application to access a file recognizable by `token`.

Example: vdu://abcdef1234

## How to use

1.) Set server address with optional port

2.) Set username, and optionally set user certificate

3.) Login; ensure the login was successful

4.) Input the file token of the requested file or use hyperlinks to access files

5.) Modify the file using the VDU Virtual Disk

6.) When done with work on the file, delete it from the VDU Virtual Disk

# VDU Simulated server

## Requirements

Python 3.6.3 or newer

## Setting up the server

1.) Run `vdusrv.py` - Runs on all interfaces on port 4443 by default

2.) To access the simulated server, run `VDUClient.exe` with `-insecure` launch option, to disable certificate verification

Edit server settings at the top of the script if required.

## Predefined data

Users: `test@example.com`, `john`

File tokens: `a`,`b`,`c`,`d`,`e`,`f`

To test VDU protocol, use `test_links.html` webpage

# Testing

![VDU Tests](https://i.imgur.com/UOp5ogZ.png)

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
- `-accessnetfile` `[urlToken]` Accesses a file in the URL format
- `-deletefile [token]`       Invalidates token
- `-rename [token] [name]`    Renames file to 'name'
- `-logout`                   Logs out current user
- `-write [token] [text]`     Writes text at the beginning of a file
- `-read [token] [cmpText]`   Reads text of the length of 'cmpText' from the beginning of a file and compares them

All of the mentioned actions can be issued to a running instance of the VDU Client. This is true even when outside of testing mode.

# Credits

Client developed using Microsoft Visual Studio 2022 Community with valid Free Licence, using MFC/ATL Build tools v143 and Windows 10.0 SDK, at <https://visualstudio.microsoft.com/cs/vs/community/>

Server developed using Microsoft Visual Studio Code, with Python 3.9.4 environment, at <https://code.visualstudio.com/>

WinFsp - Windows File System Proxy, Copyright (C) Bill Zissimopoulos. GitHub page at <https://github.com/winfsp/winfsp>, Website at <https://winfsp.dev/>,

WinFsp Virtual File System usage based on passthrough-cpp by `billziss-gh` at <https://github.com/winfsp/winfsp/tree/master/tst/passthrough-cpp>

[Cloud Storage](https://icons8.com/icon/r8kHwiV6nVEd/cloud-storage) icon by [Icons8](https://icons8.com)

Function descriptions and help in various areas of Desktop Development using C++, Windows API and MFC/ATL API at <https://docs.microsoft.com/>

# License

This project is licensed under the GPLv3 licence, as it includes a modification of work of:

 `WinFsp - Windows File System Proxy, Copyright (C) Bill Zissimopoulos.`

GitHub page at <https://github.com/winfsp/winfsp>, Website at <https://winfsp.dev/>, with the knowledge of the original author.

@Copyright (C) 2015-2022 Bill Zissimopoulos
