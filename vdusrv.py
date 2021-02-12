import os, ssl, http.server, time, random, hashlib, base64, mimetypes, json
#Api key expiration time, seconds
KEY_EXPIRATION_TIME = 60
#Current list of users who can generate keys
Users = ["test@example.com", "john"]
#Active file tokens for request testing
FileTokens = {
    "b" : {"Path" : "C:\\lidl.png", "ETag": "2.0"},
    "c" : {"Path" : "C:\\lidl.png.gz", "ETag": "alpha1"},
    }
#Current valid api keys
ApiKeys = {}

def dump(r):
    s = json.dumps(r, indent=4, sort_keys=True)
    print(s)

def Log(msg):
    print(("[%s] " + str(msg)) % time.strftime('%H:%M:%S'))

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
            Log(self.headers.as_string())
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
                        allowMode += "GET"
                        f = open(fpath, "rb")
                        bfcontent = f.read()
                        f.close()
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
                        self.send_header("Content-Location", fpath)
                        self.send_header("Content-Length", fstat.st_size)
                        self.send_header("Content-MD5", base64.b64encode(hashlib.md5(bfcontent).digest()).decode("utf-8"))
                        #print(chardet.detect(bfcontent)["encoding"])
                        self.send_header("Content-Type", mimeType[0])
                        self.send_header("Date", self.date_time_string())
                        self.send_header("Last Modified", self.date_time_string(fstat.st_mtime))
                        self.send_header("Expires", self.date_time_string(time.time() + KEY_EXPIRATION_TIME))
                        self.send_header("ETag", finst["ETag"])
                        self.end_headers()
                        self.wfile.write(bfcontent)
                        Log("GET %s From:%s File:%s (200)" % (self.path, ApiKeys[apiKey]["User"], fpath))
                
    def do_POST(self):
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FileTokens
        
        contentLen = int(self.headers.get("Content-Length"))

        if (self.path == "/auth/key"):
            Log("\n" + self.headers.as_string())
            user = self.headers.get("From")
            Log(str(self.rfile.read(contentLen)))
            if (user not in Users):
                self.send_response_only(401)
                self.end_headers()
                Log("POST %s From:%s (401)" % (self.path, user))
            else:
                apiKey = GenerateRandomToken(ApiKeys)
                expires = time.time() + KEY_EXPIRATION_TIME
                dump(expires)
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
                    finst = FileTokens[fileToken]
                    fpath = finst["Path"]
                    self.send_response_only(200)
                    self.send_header("Date", self.date_time_string())
                    self.end_headers()
                    Log("POST %s From:%s File:%s (200)" % (self.path, ApiKeys[apiKey]["User"], fpath))
    
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



httpd = http.server.HTTPServer(("localhost", 4443), VDUHTTPRequestHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile=os.path.dirname(os.path.realpath(__file__)) + "\\server.pem", server_side=True)
httpd.serve_forever()