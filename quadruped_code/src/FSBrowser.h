#ifndef _FS_BROWSER_H_
#define _FS_BROWSER_H_

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "utils.h"

#define FILESYSTEM          SPIFFS
#define FORMAT_FILESYSTEM   false   // You only need to format the filesystem once

#if FILESYSTEM == FFat
    #include <FFat.h>
#endif

#if FILESYSTEM == SPIFFS
    #include <SPIFFS.h>
#endif

class FSBrowser;

static FSBrowser *_pThis;

class FSBrowser {
private:    
    WebServer   _server;
    File        _fsUploadFile;   //holds the current upload
    String      _hostname;

public:
    //format bytes
    String formatBytes(size_t bytes) {
        if (bytes < 1024) {
            return String(bytes) + "B";
        } else if (bytes < (1024 * 1024)) {
            return String(bytes / 1024.0) + "KB";
        } else if (bytes < (1024 * 1024 * 1024)) {
            return String(bytes / 1024.0 / 1024.0) + "MB";
        } else {
            return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
        }
    }

    String getContentType(String filename) {
        if (_server.hasArg("download")) {
            return "application/octet-stream";
        } else if (filename.endsWith(".htm")) {
            return "text/html";
        } else if (filename.endsWith(".html")) {
            return "text/html";
        } else if (filename.endsWith(".css")) {
            return "text/css";
        } else if (filename.endsWith(".js")) {
            return "application/javascript";
        } else if (filename.endsWith(".png")) {
            return "image/png";
        } else if (filename.endsWith(".gif")) {
            return "image/gif";
        } else if (filename.endsWith(".jpg")) {
            return "image/jpeg";
        } else if (filename.endsWith(".ico")) {
            return "image/x-icon";
        } else if (filename.endsWith(".xml")) {
            return "text/xml";
        } else if (filename.endsWith(".pdf")) {
            return "application/x-pdf";
        } else if (filename.endsWith(".zip")) {
            return "application/x-zip";
        } else if (filename.endsWith(".gz")) {
            return "application/x-gzip";
        }
        return "text/plain";
    }

    bool exists(String path) {
        bool yes = false;
        File file = FILESYSTEM.open(path, "r");
        if (!file.isDirectory()) {
            yes = true;
        }
        file.close();
        return yes;
    }

    bool handleFileRead(String path) {
        LOG("handleFileRead: %s\n", path.c_str());
        if (path.endsWith("/")) {
            path += "index.html";
        }
        String contentType = getContentType(path);
        String pathWithGz = path + ".gz";
        if (exists(pathWithGz) || exists(path)) {
            if (exists(pathWithGz)) {
                path += ".gz";
            }
            File file = FILESYSTEM.open(path, "r");
            _server.streamFile(file, contentType);
            file.close();
            return true;
        }
        return false;
    }

    void handleFileUpload() {
        if (_server.uri() != "/edit") {
            return;
        }
        HTTPUpload& upload = _server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            String filename = upload.filename;
            if (!filename.startsWith("/")) {
                filename = "/" + filename;
            }
            LOG("handleFileUpload Name: %s\n", filename.c_str());
            _fsUploadFile = FILESYSTEM.open(filename, "w");
            filename = String();
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            //LOG("handleFileUpload Data: %ld\n", upload.currentSize);
            if (_fsUploadFile) {
                _fsUploadFile.write(upload.buf, upload.currentSize);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (_fsUploadFile) {
                _fsUploadFile.close();
            }
            LOG("handleFileUpload Size: :%d\n", upload.totalSize);
        }
    }

    void handleFileDelete() {
        if (_server.args() == 0) {
            return _server.send(500, "text/plain", "BAD ARGS");
        }
        String path = _server.arg(0);
        LOG("handleFileDelete: %s\n", path.c_str());
        if (path == "/") {
            return _server.send(500, "text/plain", "BAD PATH");
        }
        if (!exists(path)) {
            return _server.send(404, "text/plain", "FileNotFound");
        }
        FILESYSTEM.remove(path);
        _server.send(200, "text/plain", "");
        path = String();
    }

    void handleFileCreate() {
        if (_server.args() == 0) {
            return _server.send(500, "text/plain", "BAD ARGS");
        }
        String path = _server.arg(0);
        LOG("handleFileCreate: %s\n", path.c_str());
        if (path == "/") {
            return _server.send(500, "text/plain", "BAD PATH");
        }
        if (exists(path)) {
            return _server.send(500, "text/plain", "FILE EXISTS");
        }
        File file = FILESYSTEM.open(path, "w");
        if (file) {
            file.close();
        } else {
            return _server.send(500, "text/plain", "CREATE FAILED");
        }
        _server.send(200, "text/plain", "");
        path = String();
    }

    void handleFileList() {
        if (!_server.hasArg("dir")) {
            _server.send(500, "text/plain", "BAD ARGS");
            return;
        }

        String path = _server.arg("dir");
        LOG("handleFileList: %s\n", path.c_str());

        File root = FILESYSTEM.open(path);
        path = String();

        String output = "[";
        if (root.isDirectory()) {
            File file = root.openNextFile();
            while (file) {
                if (output != "[") {
                    output += ',';
                }
                output += "{\"type\":\"";
                output += (file.isDirectory()) ? "dir" : "file";
                output += "\",\"name\":\"";
                output += String(file.name()).substring(1);
                output += "\"}";
                file = root.openNextFile();
            }
        }
        output += "]";
        _server.send(200, "text/json", output);
    }

    FSBrowser(String hostname) {
        _hostname = hostname;
        _pThis = this;
    }

    void setup(char *ssid, char *password) {
        //if (FORMAT_FILESYSTEM) FILESYSTEM.format();
        FILESYSTEM.begin();
        {
            File root = FILESYSTEM.open("/");
            File file = root.openNextFile();
            while (file) {
                String fileName = file.name();
                size_t fileSize = file.size();
                LOG("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
                file = root.openNextFile();
            }
            LOG("\n");
        }

        //WIFI INIT
        LOG("Connecting to %s\n", ssid);
        if (String(WiFi.SSID()) != String(ssid)) {
            WiFi.mode(WIFI_STA);
            WiFi.setHostname(_hostname.c_str());
            WiFi.begin(ssid, password);
        }

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            LOG(".");
        }
        LOG("\nConnected! IP address: %s\n", WiFi.localIP().toString().c_str());

        MDNS.begin(_hostname.c_str());
        MDNS.addService("http", "tcp", 80);
        LOG("Open http://%s.local/edit to see the file browser\n", _hostname.c_str());

        //SERVER INIT
        //list directory
        _server.on("/list", HTTP_GET, []() {
            _pThis->handleFileList();
        });

        //load editor
        _server.on("/edit", HTTP_GET, []() {
            if (!_pThis->handleFileRead("/edit.html")) {
                _pThis->_server.send(404, "text/plain", "FileNotFound");
            }
        });

        //create file
        _server.on("/edit", HTTP_PUT, []() {
            _pThis->handleFileCreate();
        });

        //delete file
        _server.on("/edit", HTTP_DELETE, []() {
            _pThis->handleFileDelete();
        });
        //first callback is called after the request has ended with all parsed arguments
        //second callback handles file uploads at that location
        
        _server.on("/edit", HTTP_POST, []() {
            _pThis->_server.send(200, "text/plain", "");
        }, []() {
            _pThis->handleFileUpload();
        });

        //called when the url is not defined here
        //use it to load content from FILESYSTEM
        _server.onNotFound([]() {
            if (!_pThis->handleFileRead(_pThis->_server.uri())) {
                _pThis->_server.send(404, "text/plain", "FileNotFound");
            }
            });

        //get heap status, analog input value and all GPIO statuses in one json call
        _server.on("/all", HTTP_GET, []() {
            String json = "{";
            json += "\"heap\":" + String(ESP.getFreeHeap());
            json += ", \"analog\":" + String(analogRead(A0));
            json += ", \"gpio\":" + String((uint32_t)(0));
            json += "}";
            _pThis->_server.send(200, "text/json", json);
            json = String();
            });

        _server.begin();
        LOG("HTTP server started !!\n");
    }

    void loop(void) {
        _server.handleClient();
    }

    void close(void) {
        MDNS.end();
        _server.close();
        WiFi.disconnect(true, true);
        LOG("HTTP server closed !!\n");
    }
};

#endif