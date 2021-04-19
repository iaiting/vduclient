import os, sys, time, json, subprocess
thispath = os.path.dirname(os.path.realpath(__file__))
vduclient = thispath + "\\x64\\Release\\VDUClient.exe " 

#===============================================
#Settings
LOG_SERVER = False #Whether or not to log server messages in output
#===============================================
# Action list
# 
# -server [ip]              Set server ip to 'ip'
# -user [login]             Log in as 'login'
# -accessfile [token]       Accesses a file
# -deletefile [token]       Invalidates token
# -rename [token] [name]    Renames file to 'name'
# -logout                   Logs out current user
# -write [token] [text]     Writes text to a file
# 

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
    ["file", "-user john -accessfile a -deletefile a", EXIT_SUCCESS],#Access and delele file a 
    ["nofile", "-user john -accessfile 012a -deletefile 012a", EXIT_FAILURE], #File doesnt exist, last action skipped
    ["thetwotime", "-user john -accessfile d -deletefile d -accessfile d -deletefile d", EXIT_SUCCESS],
    ["tworeqs", "-user john -accessfile b -accessfile b", EXIT_FAILURE], #File already exists, fails
    ["allfiles", "-user john -accessfile a -accessfile b -accessfile c -accessfile d -accessfile e -accessfile f", EXIT_SUCCESS],
    ["rename_bf", "-user john -accessfile a -rename a test.txt -deletefile a -accessfile a -rename a plain.txt -deletefile a", EXIT_SUCCESS],
    ["rename_ne", "-user john -rename c test", EXIT_FAILURE], #Rename non existent file
    ["write_ok", "-user john -accessfile a -write a Testing_Writing_Works -deletefile a", EXIT_SUCCESS],
    ["write_ne", "-user john -write a Testing_Not_Working -deletefile a", EXIT_FAILURE],
    ["write_multi", "-user john -accessfile a -write a Testing_Writing_Works -deletefile a -accessfile a -write a Another_One -deletefile a", EXIT_SUCCESS]
]
pserver = None
if (LOG_SERVER):
    pserver = subprocess.Popen(["python", thispath + "\\vdusrv.py"])
else:
    pserver = subprocess.Popen(["python", thispath + "\\vdusrv.py"], stdout = subprocess.PIPE, stderr = subprocess.PIPE)

#Add base actions to set test mode and set our local server
vduclient += "-insecure -testmode -server 127.0.0.1:4443 "

for test in Tests:
    testName = test[0]
    testInstructions = test[1]
    expectedCode = test[2]

    p = subprocess.Popen(vduclient + testInstructions)
    p.wait()
    
    if (p.returncode == expectedCode):
        Log("✔️  [%s] %d" % (testName, p.returncode))
    else:
        Log("❌ [%s] %d (expected %d)" % (testName, p.returncode, expectedCode))

pserver.terminate()
