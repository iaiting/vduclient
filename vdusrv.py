import os, ssl, http.server, time, random, hashlib, base64, mimetypes, json
thispath = os.path.dirname(os.path.realpath(__file__))

#Server fake response delay, seconds
FAKE_RESPONSE_DELAY = 0
#File chunk read delay, seconds
FILE_CHUNK_READ_DELAY = 0.0000001
#Api key expiration time, seconds
KEY_EXPIRATION_TIME = 120
#Probability that file request will time out (for testing)
TIMEOUT_PROBABILITY = 0.01


#Current list of users who can generate keys, 
Users = ["test@example.com", "john"]
#Active file tokens for request testing
FileTokens = {
    "a" : {"Path" : "C:\\Browse.VC.db-shm", "ETag": "1"},
    "b" : {"Path" : "C:\\lidl.txt", "ETag": "1"},
    "c" : {"Path" : "C:\\lidl.txt.gz", "ETag": "1"},
    "d" : {"Path" : "C:\\chromium.7z", "ETag": "1"},
    "e" : {"Path" : "C:\\119842878_378697899829823_2582828538790603002_n.mp4", "ETag": "1"}, 
    }
#Current valid api keys
ApiKeys = {}


def dump(r):
    s = json.dumps(r, indent=4, sort_keys=True)
    print(s)

def Log(msg):
    print(("[%s] " + str(msg)) % time.strftime('%H:%M:%S'))

def FileMD5(fpath):
    with open(fpath, "rb") as f:
        file_hash = hashlib.md5()
        while chunk := f.read(8192):
            file_hash.update(chunk)
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
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FAKE_RESPONSE_DELAY, FileTokens
        
        if (FAKE_RESPONSE_DELAY > 0):
            time.sleep(FAKE_RESPONSE_DELAY)

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
                        allowMode += "GET"
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
                        self.send_header("Expires", self.date_time_string(time.time() + KEY_EXPIRATION_TIME))
                        self.send_header("ETag", finst["ETag"])
                        self.end_headers()
                        Log("GET %s From:%s File:%s (200)" % (self.path, ApiKeys[apiKey]["User"], fpath))
                        with open(fpath, "rb") as f:
                            while chunk := f.read(8192 * 2):
                                self.wfile.write(chunk)
                                time.sleep(FILE_CHUNK_READ_DELAY)
                
    def do_POST(self):
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FAKE_RESPONSE_DELAY, FileTokens
        
        if (FAKE_RESPONSE_DELAY > 0):
            time.sleep(FAKE_RESPONSE_DELAY)
        
        contentLen = int(self.headers.get("Content-Length"))

        if (self.path == "/auth/key"):
            #Log("\n" + self.headers.as_string())
            user = self.headers.get("From")
            Log(str(self.rfile.read(contentLen)))
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
                    finst = FileTokens[fileToken]
                    fpath = finst["Path"]
                    self.send_response_only(200)
                    self.send_header("Date", self.date_time_string())
                    self.end_headers()
                    Log("POST %s From:%s File:%s (200)" % (self.path, ApiKeys[apiKey]["User"], fpath))
    
    def do_DELETE(self):
        global ApiKeys, Users, KEY_EXPIRATION_TIME, FAKE_RESPONSE_DELAY, FileTokens

        if (FAKE_RESPONSE_DELAY > 0):
            time.sleep(FAKE_RESPONSE_DELAY)
        
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
httpd.socket = ssl.wrap_socket(httpd.socket, server_side=True, 
certfile=thispath + "\\server.crt",
keyfile=thispath + "\\server.key",
ca_certs=thispath + "\\ca.crt", cert_reqs=ssl.CERT_NONE)
httpd.serve_forever()