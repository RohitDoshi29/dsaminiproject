#include "../include/WebServer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <algorithm>
#include <chrono>
#include <random>

namespace {

std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

std::string parcelToJson(const Parcel& p) {
    std::ostringstream ss;
    ss << "{"
       << "\"trackingID\":\"" << escapeJson(p.trackingID) << "\","
       << "\"sender\":\"" << escapeJson(p.sender) << "\","
       << "\"recipient\":\"" << escapeJson(p.recipient) << "\","
       << "\"destination\":\"" << escapeJson(p.destination) << "\","
       << "\"status\":\"" << escapeJson(p.status) << "\","
       << "\"registrationDate\":\"" << escapeJson(p.registrationDate) << "\","
       << "\"dispatchDate\":\"" << escapeJson(p.dispatchDate) << "\""
       << "}";
    return ss.str();
}

std::string parcelsToJsonArray(const std::vector<Parcel>& list) {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < list.size(); ++i) {
        ss << parcelToJson(list[i]);
        if (i + 1 < list.size()) ss << ",";
    }
    ss << "]";
    return ss.str();
}

std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%') {
            if (i + 2 < in.size()) {
                int hex = 0;
                std::istringstream iss(in.substr(i + 1, 2));
                if (iss >> std::hex >> hex) {
                    out += static_cast<char>(hex);
                    i += 2;
                } else {
                    out += '%';
                }
            } else {
                out += '%';
            }
        } else if (in[i] == '+') {
            out += ' ';
        } else {
            out += in[i];
        }
    }
    return out;
}

std::string getQueryParam(const std::string& query, const std::string& key) {
    std::string prefix = key + "=";
    size_t pos = 0;
    while ((pos = query.find(prefix, pos)) != std::string::npos) {
        if (pos == 0 || query[pos - 1] == '&') {
            size_t end = query.find('&', pos);
            if (end == std::string::npos) end = query.length();
            std::string val = query.substr(pos + prefix.length(), end - (pos + prefix.length()));
            return urlDecode(val);
        }
        pos += prefix.length();
    }
    return "";
}

std::string getJsonField(const std::string& json, const std::string& field) {
    std::string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + key.length());
    if (pos == std::string::npos) return "";

    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos) return "";

    if (json[pos] == '"') {
        size_t endQuote = json.find('"', pos + 1);
        if (endQuote != std::string::npos) {
            return json.substr(pos + 1, endQuote - (pos + 1));
        }
    } else {
        size_t endVal = json.find_first_of(",}\r\n", pos);
        if (endVal != std::string::npos) {
            return json.substr(pos, endVal - pos);
        }
    }
    return "";
}

} // namespace

WebServer::WebServer(ParcelManager& mgr, int portNum, std::string dir)
    : manager(mgr), port(portNum), webDir(std::move(dir)) {}

std::string WebServer::serveStaticFile(const std::string& filepath, std::string& contentType) {
    std::string fullPath = webDir + "/" + filepath;
    if (filepath == "index.html" || filepath.empty() || filepath == "/") {
        fullPath = webDir + "/index.html";
        contentType = "text/html; charset=utf-8";
    } else if (filepath.rfind(".css") != std::string::npos) {
        contentType = "text/css; charset=utf-8";
    } else if (filepath.rfind(".js") != std::string::npos) {
        contentType = "application/javascript; charset=utf-8";
    } else if (filepath.rfind(".json") != std::string::npos) {
        contentType = "application/json; charset=utf-8";
    } else {
        contentType = "text/plain; charset=utf-8";
    }

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string WebServer::handleApiRequest(const std::string& method, const std::string& path,
                                       const std::string& query, const std::string& body,
                                       std::string& contentType) {
    contentType = "application/json; charset=utf-8";

    if (path == "/api/parcels" && method == "GET") {
        return parcelsToJsonArray(manager.getAllParcels());
    }

    if (path == "/api/pending" && method == "GET") {
        return parcelsToJsonArray(manager.getPendingDispatches());
    }

    if (path == "/api/search" && method == "GET") {
        std::string id = getQueryParam(query, "id");
        if (id.empty()) {
            return "{\"error\":\"Missing 'id' parameter\"}";
        }

        size_t seqSteps = 0, hashSteps = 0;
        double seqTimeNs = 0.0, hashTimeNs = 0.0;
        int seqIdx = manager.sequentialSearch(id, seqSteps, seqTimeNs);
        int hashIdx = manager.hashSearch(id, hashSteps, hashTimeNs);

        std::ostringstream ss;
        ss << "{"
           << "\"trackingID\":\"" << escapeJson(id) << "\","
           << "\"found\":" << (hashIdx != -1 ? "true" : "false") << ",";

        if (hashIdx != -1) {
            const Parcel* p = manager.getParcelByIndex(hashIdx);
            ss << "\"parcel\":" << (p ? parcelToJson(*p) : "null") << ",";
        } else {
            ss << "\"parcel\":null,";
        }

        double speedup = (hashTimeNs > 0.0) ? (seqTimeNs / hashTimeNs) : 1.0;
        ss << "\"sequential\":{"
           << "\"found\":" << (seqIdx != -1 ? "true" : "false") << ","
           << "\"steps\":" << seqSteps << ","
           << "\"timeNs\":" << std::fixed << std::setprecision(1) << seqTimeNs << ","
           << "\"complexity\":\"O(N)\""
           << "},"
           << "\"hash\":{"
           << "\"found\":" << (hashIdx != -1 ? "true" : "false") << ","
           << "\"steps\":" << hashSteps << ","
           << "\"timeNs\":" << std::fixed << std::setprecision(1) << hashTimeNs << ","
           << "\"complexity\":\"O(1) avg\""
           << "},"
           << "\"speedup\":" << std::fixed << std::setprecision(2) << speedup
           << "}";
        return ss.str();
    }

    if (path == "/api/register" && method == "POST") {
        std::string id = getJsonField(body, "trackingID");
        std::string sender = getJsonField(body, "sender");
        std::string recipient = getJsonField(body, "recipient");
        std::string destination = getJsonField(body, "destination");
        std::string regDate = getJsonField(body, "registrationDate");

        if (id.empty() || sender.empty() || recipient.empty() || destination.empty()) {
            // Also try URL encoded body
            if (id.empty()) id = getQueryParam(body, "trackingID");
            if (sender.empty()) sender = getQueryParam(body, "sender");
            if (recipient.empty()) recipient = getQueryParam(body, "recipient");
            if (destination.empty()) destination = getQueryParam(body, "destination");
            if (regDate.empty()) regDate = getQueryParam(body, "registrationDate");
        }

        if (regDate.empty()) regDate = ParcelManager::getCurrentDate();

        if (id.empty()) {
            return "{\"success\":false,\"error\":\"Tracking ID cannot be empty.\"}";
        }
        if (manager.contains(id)) {
            return "{\"success\":false,\"error\":\"Tracking ID '" + escapeJson(id) + "' already exists! Duplicate rejected.\"}";
        }

        Parcel p(id, sender, recipient, destination, ParcelStatus::REGISTERED, regDate, "-");
        bool ok = manager.registerParcel(p);
        if (ok) {
            return "{\"success\":true,\"message\":\"Parcel registered successfully.\",\"parcel\":" + parcelToJson(p) + "}";
        } else {
            return "{\"success\":false,\"error\":\"Failed to save parcel.\"}";
        }
    }

    if (path == "/api/update-status" && method == "POST") {
        std::string id = getJsonField(body, "trackingID");
        std::string status = getJsonField(body, "status");
        std::string dispDate = getJsonField(body, "dispatchDate");

        if (id.empty()) id = getQueryParam(body, "trackingID");
        if (status.empty()) status = getQueryParam(body, "status");
        if (dispDate.empty()) dispDate = getQueryParam(body, "dispatchDate");

        if (id.empty() || !manager.contains(id)) {
            return "{\"success\":false,\"error\":\"Parcel not found.\"}";
        }

        bool ok = manager.updateStatus(id, status, dispDate);
        if (ok) {
            size_t dummySteps = 0;
            double dummyTime = 0.0;
            int idx = manager.hashSearch(id, dummySteps, dummyTime);
            const Parcel* p = manager.getParcelByIndex(idx);
            return "{\"success\":true,\"message\":\"Status updated successfully.\",\"parcel\":" + (p ? parcelToJson(*p) : "null") + "}";
        } else {
            return "{\"success\":false,\"error\":\"Update failed.\"}";
        }
    }

    if (path == "/api/sort" && method == "GET") {
        std::string by = getQueryParam(query, "by");
        std::string order = getQueryParam(query, "order");
        bool ascending = (order != "desc");

        if (by == "destination") {
            manager.sortByDestination(ascending);
        } else {
            manager.sortByTrackingID(ascending);
        }

        return "{\"success\":true,\"sortedBy\":\"" + escapeJson(by) + "\",\"ascending\":" + (ascending ? "true" : "false") +
               ",\"parcels\":" + parcelsToJsonArray(manager.getAllParcels()) + "}";
    }

    if (path == "/api/benchmark" && method == "GET") {
        std::string nStr = getQueryParam(query, "n");
        std::string qStr = getQueryParam(query, "q");
        size_t n = 10000;
        size_t q = 5000;
        if (!nStr.empty()) n = std::max<size_t>(100, std::stoul(nStr));
        if (!qStr.empty()) q = std::max<size_t>(100, std::stoul(qStr));

        // Generate synthetic benchmark
        std::vector<Parcel> benchParcels;
        benchParcels.reserve(n);
        std::unordered_map<std::string, size_t> benchIndex;
        benchIndex.reserve(n);

        std::vector<std::string> cities = {"Mumbai", "Delhi", "Bangalore", "Pune", "Chennai", "Kolkata"};
        for (size_t i = 0; i < n; ++i) {
            std::string id = "B" + std::to_string(100000 + i);
            Parcel p(id, "S" + std::to_string(i), "R" + std::to_string(i),
                     cities[i % cities.size()], "Registered", "2026-10-01", "-");
            benchParcels.push_back(p);
            benchIndex[id] = i;
        }

        std::mt19937 rng(42);
        std::uniform_int_distribution<size_t> dist(0, n - 1);
        std::vector<std::string> queries;
        queries.reserve(q);
        for (size_t i = 0; i < q; ++i) {
            queries.push_back(benchParcels[dist(rng)].trackingID);
        }

        // Sequential search benchmark
        auto s1 = std::chrono::high_resolution_clock::now();
        size_t seqFound = 0;
        for (const auto& target : queries) {
            for (size_t i = 0; i < benchParcels.size(); ++i) {
                if (benchParcels[i].trackingID == target) {
                    seqFound++;
                    break;
                }
            }
        }
        auto s2 = std::chrono::high_resolution_clock::now();
        double seqMs = std::chrono::duration<double, std::milli>(s2 - s1).count();

        // Hash search benchmark
        auto h1 = std::chrono::high_resolution_clock::now();
        size_t hashFound = 0;
        for (const auto& target : queries) {
            if (benchIndex.find(target) != benchIndex.end()) {
                hashFound++;
            }
        }
        auto h2 = std::chrono::high_resolution_clock::now();
        double hashMs = std::chrono::duration<double, std::milli>(h2 - h1).count();

        double speedup = (hashMs > 0.0) ? (seqMs / hashMs) : 1.0;
        double seqAvgUs = (seqMs * 1000.0) / q;
        double hashAvgUs = (hashMs * 1000.0) / q;

        std::ostringstream ss;
        ss << "{"
           << "\"n\":" << n << ","
           << "\"queries\":" << q << ","
           << "\"seqDurationMs\":" << std::fixed << std::setprecision(2) << seqMs << ","
           << "\"seqAvgUs\":" << std::fixed << std::setprecision(4) << seqAvgUs << ","
           << "\"hashDurationMs\":" << std::fixed << std::setprecision(2) << hashMs << ","
           << "\"hashAvgUs\":" << std::fixed << std::setprecision(4) << hashAvgUs << ","
           << "\"speedup\":" << std::fixed << std::setprecision(1) << speedup
           << "}";
        return ss.str();
    }

    return "{\"error\":\"Unknown endpoint\"}";
}

void WebServer::start() {
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[Error] Could not create server socket.\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[Error] Could not bind to port " << port << ". Try specifying another port.\n";
        close(server_fd);
        return;
    }

    if (listen(server_fd, 20) < 0) {
        std::cerr << "[Error] Listen failed on port " << port << ".\n";
        close(server_fd);
        return;
    }

    std::cout << "\n========================================================================\n";
    std::cout << "  COURIER PARCEL TRACKING & DISPATCH - NATIVE C++ WEB SERVER            \n";
    std::cout << "========================================================================\n";
    std::cout << "  Server running at : http://localhost:" << port << "/\n";
    std::cout << "  API Endpoints     : /api/parcels, /api/search, /api/benchmark, etc.   \n";
    std::cout << "  Persistent DB     : parcels.txt\n";
    std::cout << "  Press Ctrl+C to stop the server.\n";
    std::cout << "========================================================================\n\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            continue;
        }

        char buffer[8192];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            close(client_sock);
            continue;
        }

        std::string request(buffer, bytes_read);
        std::istringstream reqStream(request);
        std::string method, fullUri, proto;
        reqStream >> method >> fullUri >> proto;

        // Parse path and query
        std::string path = fullUri;
        std::string query;
        size_t qPos = fullUri.find('?');
        if (qPos != std::string::npos) {
            path = fullUri.substr(0, qPos);
            query = fullUri.substr(qPos + 1);
        }

        // Extract body if POST
        std::string body;
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            body = request.substr(headerEnd + 4);
        }

        std::string responseBody;
        std::string contentType = "text/html; charset=utf-8";
        int statusCode = 200;

        // Handle CORS preflight
        if (method == "OPTIONS") {
            std::string preflight =
                "HTTP/1.1 204 No Content\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n"
                "Content-Length: 0\r\n\r\n";
            ssize_t w = write(client_sock, preflight.c_str(), preflight.size());
            (void)w;
            close(client_sock);
            continue;
        }

        if (path.rfind("/api/", 0) == 0) {
            responseBody = handleApiRequest(method, path, query, body, contentType);
        } else {
            // Serve static web files
            std::string filename = (path == "/" || path.empty()) ? "index.html" : path.substr(1);
            if (filename.rfind("web/", 0) == 0) filename = filename.substr(4);
            responseBody = serveStaticFile(filename, contentType);
            if (responseBody.empty()) {
                statusCode = 404;
                responseBody = "<h1>404 Not Found</h1><p>The requested file was not found.</p>";
                contentType = "text/html; charset=utf-8";
            }
        }

        std::ostringstream response;
        response << "HTTP/1.1 " << statusCode << " " << (statusCode == 200 ? "OK" : "Not Found") << "\r\n"
                 << "Content-Type: " << contentType << "\r\n"
                 << "Content-Length: " << responseBody.size() << "\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Connection: close\r\n\r\n"
                 << responseBody;

        std::string respStr = response.str();
        ssize_t w = write(client_sock, respStr.c_str(), respStr.size());
        (void)w;
        close(client_sock);
    }

    close(server_fd);
}
