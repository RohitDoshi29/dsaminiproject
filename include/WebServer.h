#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "ParcelManager.h"
#include <string>

class WebServer {
private:
    ParcelManager& manager;
    int port;
    std::string webDir;

    std::string handleApiRequest(const std::string& method, const std::string& path,
                                const std::string& query, const std::string& body,
                                std::string& contentType);
    std::string serveStaticFile(const std::string& filepath, std::string& contentType);

public:
    WebServer(ParcelManager& mgr, int portNum = 5000, std::string dir = "web");
    void start();
};

#endif // WEB_SERVER_H
