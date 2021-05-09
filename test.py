#
# @author xferan00
# @file test.py
#

import os, time, subprocess
thispath = os.path.dirname(os.path.realpath(__file__))

#===============================================
#Settings
VDUCLIENT = thispath + "\\Release\\x64\\VDUClient.exe" 
VDUSERVER = thispath + "\\vdusrv.py"
LOCAL_SERVER_ADDRESS = "127.0.0.1:4443"
PADDING = (20 * "=")
#===============================================
# Action list
# 
# -server [ip]              Set server ip to 'ip'
# -user [login]             Log in as 'login'
# -accessfile [token]       Accesses a file
# -deletefile [token]       Invalidates token
# -rename [token] [name]    Renames file to 'name'
# -logout                   Logs out current user
# -write [token] [text]     Writes `text` at the beginning of a file
# -read [token] [cmpText]   Reads text of the length of `cmpText` from the beginning of a file and compares them

def Log(msg):
    print(("[%s] " + str(msg)) % time.strftime('%H:%M:%S'))

EXIT_SUCCESS = 0
EXIT_FAILURE = 1
#Array of tests
#Each test is the name, the instructions of test and expected exit code
Tests = [
    ["server_bad", "-server 0.0.0.0:4443 -user john", EXIT_FAILURE],
    ["login_ok", "-user john", EXIT_SUCCESS], #Simple login test, can we login under this name?
    ["login_bad", "-user alex", EXIT_FAILURE], #This user does not exist
    ["logout_ok", "-user john -logout", EXIT_SUCCESS],
    ["logout_bad", "-logout", EXIT_FAILURE],
    ["nologin", "-user \"\"", EXIT_FAILURE],
    ["del_ne", "-deletefile x", EXIT_FAILURE], #Not logged in,
    ["file", "-user john -accessfile a -deletefile a -logout", EXIT_SUCCESS],#Access and delele file a 
    ["nofile", "-user john -accessfile 012a -deletefile 012a", EXIT_FAILURE], #File doesnt exist, last action skipped
    ["thetwotime", "-user john -accessfile d -deletefile d -accessfile d -deletefile d -logout", EXIT_SUCCESS],
    ["tworeqs", "-user john -accessfile b -accessfile b", EXIT_FAILURE], #File already exists, fails
    ["allfiles", "-user john -accessfile a -accessfile b -accessfile c -accessfile d -accessfile e -accessfile f -logout", EXIT_SUCCESS],
    ["rename_bf", "-user john -accessfile a -rename a test.txt -deletefile a -accessfile a -rename a plain.txt -deletefile a -logout", EXIT_SUCCESS],
    ["rename_ne", "-user john -rename c test", EXIT_FAILURE], #Rename non existent file
    ["write_ok", "-user john -accessfile a -write a Testing_Writing_Works -deletefile a -logout", EXIT_SUCCESS],
    ["write_ne", "-user john -write a Testing_Not_Working -deletefile a", EXIT_FAILURE],
    ["write_multi", "-user john -accessfile a -write a Testing_Writing_Works -deletefile a -accessfile a -write a Another_One -deletefile a -logout", EXIT_SUCCESS],
    ["read_ok", "-user john -accessfile a -write a Apple -deletefile a -accessfile a -read a Apple -deletefile a -logout", EXIT_SUCCESS],
    ["read_bad", "-user john -accessfile a -write a Pear -deletefile a -accessfile a -read a Citron -deletefile a", EXIT_FAILURE],
    ["read_two", "-user john -accessfile a -write a Citron_is_healthy -deletefile a -accessfile a -read a Citron -write a Banana -deletefile a -accessfile a -read a Banana_is_healthy -logout", EXIT_SUCCESS],
]

#Add base actions to set test mode and set our local server
VDUCLIENT += " -insecure -testmode -server %s " % (LOCAL_SERVER_ADDRESS)
VDUSERVER = "python " + VDUSERVER

successfulTestCount = 0
for test in Tests:
    testName = test[0]
    testInstructions = test[1]
    expectedCode = test[2]

    pserver = subprocess.Popen(VDUSERVER, stdout=subprocess.PIPE)

    p = subprocess.Popen(VDUCLIENT + testInstructions)
    p.wait()

    pserver.terminate()
    while pserver.poll() == None:
        None
                
    if (p.returncode == expectedCode):
        Log("[Test]  OK  [%s] %d" % (testName, p.returncode))
        successfulTestCount = successfulTestCount + 1
    else:
        Log("[Test] FAIL [%s] %d (expected %d)" + PADDING % (testName, p.returncode, expectedCode))
        exit()
    

Log(PADDING)
Log("[Test] Passed %d/%d tests" % (successfulTestCount, len(Tests)))
os.system("pause")