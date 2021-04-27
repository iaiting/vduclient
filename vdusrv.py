import os, ssl, http.server, time, random, hashlib, base64, mimetypes
thispath = os.path.dirname(os.path.realpath(__file__))

#File chunk read delay, seconds
FILE_CHUNK_READ_DELAY = 0
#Api key / file token expiration time, seconds
KEY_EXPIRATION_TIME = 120
#Probability that file request will time out (for testing)
TIMEOUT_PROBABILITY = 0

#Current list of users who can generate keys, 
Users = ["test@example.com", "john"]
#Active file tokens for request testing
#You can add this with a program or manually, just for testing
FileTokens = {
    "a" : {"Path" : thispath + "\\TestFiles\\plain.txt", "ETag": "1", "Expires":0},
    "b" : {"Path" : thispath + "\\TestFiles\\compressed.zip", "ETag": "1", "Expires":0},
    "c" : {"Path" : thispath + "\\TestFiles\\image.png", "ETag": "1", "Expires":0},
    "d" : {"Path" : thispath + "\\TestFiles\\hugefile.bin", "ETag": "1", "Expires":0},
    "e" : {"Path" : thispath + "\\TestFiles\\rand.py", "ETag": "1", "Expires":0}, 
    "f" : {"Path" : thispath + "\\TestFiles\\document.docx", "ETag": "1", "Expires":0}, 
    }
#Current valid api keys, will be generated on user login
ApiKeys = {}

def Log(msg):
    print(("[%s] [SERVER] " + str(msg)) % time.strftime('%H:%M:%S'))

def FileMD5(fpath):
    with open(fpath, "rb") as f:
        file_hash = hashlib.md5()
        while True:
            chunk = f.read(8192)
            if not chunk:
                break
            file_hash.update(chunk)
        f.close()
    return file_hash.digest()

def GenerateRandomToken(duplicateCheckDict = None):
    token = ""
    while True:
        token = token.join(random.choice("abcdefghijklmnopqrstuvwxyz0123456789") for i in range(64))
        if (duplicateCheckDict == None or token not in duplicateCheckDict):
            break
    return token

class VDUHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FileTokens

        if (self.path == "/ping"):
            self.send_response_only(204)
            self.send_header("Date", self.date_time_string())
            self.send_header("X-Client-Ip", self.client_address[0])
            self.end_headers()
            Log("GET %s from %s (204)" % (self.path, self.client_address[0]))
        elif (self.path == "/auth/key"):
            apiKey = self.headers.get("X-Api-Key")
            if (apiKey not in ApiKeys):
                self.send_response_only(401)
                self.end_headers()
                Log("GET %s (401)" % (self.path))
            else:
                newApiKey = GenerateRandomToken(ApiKeys)
                expires = time.time() + KEY_EXPIRATION_TIME
                ApiKeys[newApiKey] = {"Expires": expires, "User": ApiKeys[apiKey]["User"]}
                ApiKeys.pop(apiKey)
                self.send_response_only(200)
                self.send_header("X-Api-Key", newApiKey)
                self.send_header("Date", self.date_time_string())
                self.send_header("Expires", self.date_time_string(expires))
                self.end_headers()
                Log("GET %s From:%s (200)" % (self.path, ApiKeys[newApiKey]["User"]))
        elif (self.path.startswith("/file/")):
            apiKey = self.headers.get("X-Api-Key")
            if (apiKey not in ApiKeys):
                self.send_response_only(401)
                self.end_headers()
                Log("GET %s (401)" % (self.path))
            else:
                fileToken = self.path.split("/file/")[1]
                if (fileToken not in FileTokens):
                    self.send_response_only(404)
                    self.end_headers()
                    Log("GET %s From:%s (404)" % (self.path, ApiKeys[apiKey]["User"]))
                else:
                    finst = FileTokens[fileToken]
                    fpath = finst["Path"]
                    allowMode = ""
                    if (not os.access(fpath, os.R_OK)):
                        self.send_response_only(405)
                        self.end_headers()
                        Log("GET %s From:%s File:%s (405)" % (self.path, ApiKeys[apiKey]["User"], fpath))
                    else:
                        fstat = os.stat(fpath)
                        allowMode = ""
                        if (os.access(fpath, os.R_OK)):
                            allowMode += "GET"

                        if (os.access(fpath, os.W_OK)):
                            allowMode += " POST"
                        
                        mimeType = mimetypes.guess_type(fpath)
                        self.send_response_only(200)
                        self.send_header("Allow", allowMode)
                        self.send_header("Content-Encoding", mimeType[1])
                        filedirpath, filename = os.path.split(fpath)
                        filedirpath
                        self.send_header("Content-Location", filename)
                        self.send_header("Content-Length", fstat.st_size)
                        self.send_header("Content-MD5", base64.b64encode(FileMD5(fpath)).decode("utf-8"))
                        self.send_header("Content-Type", mimeType[0])
                        self.send_header("Date", self.date_time_string())
                        self.send_header("Last-Modified", self.date_time_string(fstat.st_mtime))
                        finst["Expires"] = time.time() + KEY_EXPIRATION_TIME
                        self.send_header("Expires", self.date_time_string(finst["Expires"]))
                        self.send_header("ETag", finst["ETag"])
                        self.end_headers()
                        Log("GET %s From:%s File:%s (200)" % (self.path, ApiKeys[apiKey]["User"], fpath))
                        with open(fpath, "rb") as f:
                            while True:
                                chunk = f.read(8192)
                                if not chunk:
                                    break
                                self.wfile.write(chunk)
                                if (FILE_CHUNK_READ_DELAY):
                                    time.sleep(FILE_CHUNK_READ_DELAY)
                            f.close()
                
    def do_POST(self):
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FileTokens
        
        contentLen = int(self.headers.get("Content-Length"))

        if (self.path == "/auth/key"):
            #Log("\n" + self.headers.as_string())
            user = self.headers.get("From")
            #Log(str(self.rfile.read(contentLen)))
            if (user not in Users):
                self.send_response_only(401)
                self.end_headers()
                Log("POST %s From:%s (401)" % (self.path, user))
            else:
                apiKey = GenerateRandomToken(ApiKeys)
                expires = time.time() + KEY_EXPIRATION_TIME
                ApiKeys[apiKey] = {"Expires": expires, "User": user}
                self.send_response_only(201)
                self.send_header("X-Api-Key", apiKey)
                self.send_header("Date", self.date_time_string())
                self.send_header("Expires", self.date_time_string(expires))
                self.end_headers()
                Log("POST %s From:%s (201)" % (self.path, user))
        elif (self.path.startswith("/file/")):
            apiKey = self.headers.get("X-Api-Key")
            if (apiKey not in ApiKeys):
                self.send_response_only(401)
                self.end_headers()
                Log("POST %s (401)" % (self.path))
            else:
                fileToken = self.path.split("/file/")[1]
                if (fileToken not in FileTokens):
                    self.send_response_only(404)
                    self.end_headers()
                    Log("POST %s From:%s (404)" % (self.path, ApiKeys[apiKey]["User"]))
                else:
                    if (random.random() <= TIMEOUT_PROBABILITY):
                        self.send_response_only(408)
                        self.end_headers()
                        Log("POST %s From:%s (408)" % (self.path, ApiKeys[apiKey]["User"]))
                        return

                    finst = FileTokens[fileToken]
                    fpath = finst["Path"]
                    filedirpath, filename = os.path.split(fpath)

                    allowMode = ""
                    if (os.access(fpath, os.R_OK)):
                        allowMode += "GET"

                    if (os.access(fpath, os.W_OK)):
                        allowMode += " POST"
                    else:
                        self.send_response_only(405)
                        self.end_headers()
                        Log("POST %s From:%s (405)" % (self.path, ApiKeys[apiKey]["User"]))
                        return

                    #Needs renaming?
                    newFileName = self.headers.get("Content-Location")
                    if (newFileName != filename):
                        os.rename(fpath, filedirpath + "\\" + newFileName)
                        fpath = filedirpath + "\\" + newFileName
                        finst["Path"] = fpath

                    #Make sure md5 hash is matching
                    calculatedMD5 = base64.b64encode(FileMD5(fpath)).decode("utf-8")
                    receivedMD5 = self.headers.get("Content-MD5")
                    if (calculatedMD5 != receivedMD5):
                        #Write new contents
                        try:
                            with open(fpath, "wb") as f:
                                f.write(self.rfile.read(contentLen))
                                f.close()
                        except:
                            self.send_response_only(409)
                            self.end_headers()
                            Log("POST %s From:%s (409)" % (self.path, ApiKeys[apiKey]["User"]))
                            return
                    else:
                        #Not reading rfile with content can cause abnormal termination of connection
                        self.rfile.read(contentLen)

                    #Size should be matching as well
                    fstat = os.stat(fpath)
                    newSize = int(self.headers.get("Content-Length"))
                    if (newSize != fstat.st_size):
                        Log("Length mismatch!!")

                    #Mime type check
                    mimeType = mimetypes.guess_type(fpath)
                    newEncoding = self.headers.get("Content-Encoding")
                    newType = self.headers.get("Content-Type")

                    if (str(mimeType[1]) != newEncoding):
                        Log("Encoding mismatch!!")
                    if (str(mimeType[0]) != newType):
                        Log("Type mismatch!!!")

                    #Increase version
                    finst["ETag"] = str(int(finst["ETag"]) + 1)

                    #A case for sending 205 response, if send after expiration date
                    if (time.time() > finst["Expires"]):
                        self.send_response_only(205)
                        self.end_headers()
                        Log("POST %s From:%s File:%s (205)" % (self.path, ApiKeys[apiKey]["User"], fpath))
                        return
                    
                    self.send_response_only(201)
                    self.send_header("Allow", allowMode)
                    self.send_header("Date", self.date_time_string())

                    #New expiration time
                    finst["Expires"] = time.time() + KEY_EXPIRATION_TIME

                    self.send_header("Expires", finst["Expires"])
                    self.send_header("ETag", finst["ETag"])
                    self.end_headers()
                    Log("POST %s From:%s File:%s (201)" % (self.path, ApiKeys[apiKey]["User"], fpath))
                    return
    
    def do_DELETE(self):
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FileTokens
        
        if (self.path == "/auth/key"):
            apiKey = self.headers.get("X-Api-Key")
            if (apiKey not in ApiKeys):
                self.send_response_only(401)
                self.end_headers()
                Log("DELETE %s (401)" % (self.path))
            else:
                self.send_response_only(204)
                self.send_header("Date", self.date_time_string())
                self.end_headers()
                Log("DELETE %s From:%s (204)" % (self.path, ApiKeys[apiKey]["User"]))
                ApiKeys.pop(apiKey)
        elif (self.path.startswith("/file/")):
            apiKey = self.headers.get("X-Api-Key")
            if (apiKey not in ApiKeys):
                self.send_response_only(401)
                self.end_headers()
                Log("DELETE %s (401)" % (self.path))
            else:
                fileToken = self.path.split("/file/")[1]
                if (fileToken not in FileTokens):
                    self.send_response_only(404)
                    self.end_headers()
                    Log("DELETE %s From:%s (404)" % (self.path, ApiKeys[apiKey]["User"]))
                else:
                    if (random.random() <= TIMEOUT_PROBABILITY):
                        self.send_response_only(408)
                        self.end_headers()
                        Log("DELETE %s From:%s (408)" % (self.path, ApiKeys[apiKey]["User"]))
                    else:
                        self.send_response_only(204)
                        self.end_headers()
                        Log("DELETE %s From:%s (204)" % (self.path, ApiKeys[apiKey]["User"]))


httpd = http.server.HTTPServer(("0.0.0.0", 4443), VDUHTTPRequestHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, server_side=True, certfile=thispath + "\\server_.pem")
httpd.serve_forever()