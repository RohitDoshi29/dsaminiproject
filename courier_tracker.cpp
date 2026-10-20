/**
 * ============================================================================
 * CP DATA STRUCTURES PROJECT: Courier Parcel Tracking & Dispatch
 * Author: Rohit Doshi
 * Language: C++17
 * Architecture: std::vector<Parcel> + std::unordered_map<string, size_t>
 * Persistence: File handling with pipe-delimited text format (parcels.txt)
 * Algorithms: Sequential Search O(N) vs Hash-Based Lookup O(1), MergeSort O(N log N)
 * Web Server: Built-in Native POSIX Socket HTTP Server & REST API
 * ============================================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <random>
#include <limits>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

namespace ParcelStatus {
    const std::string REGISTERED = "Registered";
    const std::string DISPATCHED = "Dispatched";
    const std::string IN_TRANSIT = "In Transit";
    const std::string DELIVERED  = "Delivered";
    const std::string CANCELLED  = "Cancelled";

    inline bool isValidStatus(const std::string& status) {
        return (status == REGISTERED || status == DISPATCHED ||
                status == IN_TRANSIT  || status == DELIVERED  ||
                status == CANCELLED);
    }
}

struct Parcel {
    std::string trackingID;
    std::string sender;
    std::string recipient;
    std::string destination;
    std::string status;          // "Registered", "Dispatched", "In Transit", "Delivered", "Cancelled"
    std::string registrationDate;// YYYY-MM-DD
    std::string dispatchDate;    // YYYY-MM-DD or "-"

    Parcel()
        : trackingID(""), sender(""), recipient(""), destination(""),
          status(ParcelStatus::REGISTERED), registrationDate(""), dispatchDate("-") {}

    Parcel(std::string id, std::string snd, std::string rcv, std::string dest,
           std::string stat, std::string regDate, std::string dispDate = "-")
        : trackingID(std::move(id)), sender(std::move(snd)), recipient(std::move(rcv)),
          destination(std::move(dest)), status(std::move(stat)),
          registrationDate(std::move(regDate)), dispatchDate(std::move(dispDate)) {
        if (dispatchDate.empty()) dispatchDate = "-";
    }

    std::string toFileLine() const {
        return trackingID + " | " + sender + " | " + recipient + " | " +
               destination + " | " + status + " | " + registrationDate + " | " +
               (dispatchDate.empty() ? "-" : dispatchDate);
    }
};

class ParcelManager {
private:
    std::vector<Parcel> parcels;
    std::unordered_map<std::string, size_t> trackingIndex;
    std::string filename;

    void merge(int left, int mid, int right, bool byDestination) {
        int n1 = mid - left + 1;
        int n2 = right - mid;

        std::vector<Parcel> L(parcels.begin() + left, parcels.begin() + mid + 1);
        std::vector<Parcel> R(parcels.begin() + mid + 1, parcels.begin() + right + 1);

        int i = 0, j = 0, k = left;
        while (i < n1 && j < n2) {
            bool condition = false;
            if (byDestination) {
                if (L[i].destination == R[j].destination) {
                    condition = (L[i].trackingID <= R[j].trackingID);
                } else {
                    condition = (L[i].destination <= R[j].destination);
                }
            } else {
                condition = (L[i].trackingID <= R[j].trackingID);
            }

            if (condition) {
                parcels[k++] = L[i++];
            } else {
                parcels[k++] = R[j++];
            }
        }

        while (i < n1) parcels[k++] = L[i++];
        while (j < n2) parcels[k++] = R[j++];
    }

    void mergeSort(int left, int right, bool byDestination) {
        if (left < right) {
            int mid = left + (right - left) / 2;
            mergeSort(left, mid, byDestination);
            mergeSort(mid + 1, right, byDestination);
            merge(left, mid, right, byDestination);
        }
    }

public:
    explicit ParcelManager(std::string storageFile = "parcels.txt")
        : filename(std::move(storageFile)) {
        loadRecords();
    }

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string getCurrentDate() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
        localtime_r(&now_c, &tm_buf);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);
        return std::string(buf);
    }

    void rebuildIndex() {
        trackingIndex.clear();
        for (size_t i = 0; i < parcels.size(); ++i) {
            trackingIndex[parcels[i].trackingID] = i;
        }
    }

    bool loadRecords() {
        parcels.clear();
        trackingIndex.clear();

        std::ifstream inFile(filename);
        if (!inFile.is_open()) return false;

        std::string line;
        while (std::getline(inFile, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> fields;

            while (std::getline(ss, token, '|')) {
                fields.push_back(trim(token));
            }

            if (fields.size() >= 7) {
                Parcel p(fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6]);
                parcels.push_back(p);
                trackingIndex[p.trackingID] = parcels.size() - 1;
            }
        }

        inFile.close();
        return true;
    }

    bool saveRecords() const {
        std::ofstream outFile(filename, std::ios::trunc);
        if (!outFile.is_open()) return false;

        for (const auto& p : parcels) {
            outFile << p.toFileLine() << "\n";
        }

        outFile.close();
        return true;
    }

    bool contains(const std::string& trackingID) const {
        return trackingIndex.find(trackingID) != trackingIndex.end();
    }

    size_t count() const {
        return parcels.size();
    }

    const std::vector<Parcel>& getAllParcels() const {
        return parcels;
    }

    const Parcel* getParcelByIndex(size_t index) const {
        if (index < parcels.size()) return &parcels[index];
        return nullptr;
    }

    bool registerParcel(const Parcel& parcel) {
        if (parcel.trackingID.empty()) return false;
        if (contains(parcel.trackingID)) return false;

        parcels.push_back(parcel);
        trackingIndex[parcel.trackingID] = parcels.size() - 1;
        saveRecords();
        return true;
    }

    bool updateStatus(const std::string& trackingID, const std::string& newStatus, const std::string& dispatchDate = "") {
        auto it = trackingIndex.find(trackingID);
        if (it == trackingIndex.end()) return false;

        size_t idx = it->second;
        parcels[idx].status = newStatus;

        if (!dispatchDate.empty() && dispatchDate != "-") {
            parcels[idx].dispatchDate = dispatchDate;
        } else if (newStatus == ParcelStatus::DISPATCHED ||
                   newStatus == ParcelStatus::IN_TRANSIT ||
                   newStatus == ParcelStatus::DELIVERED) {
            if (parcels[idx].dispatchDate == "-" || parcels[idx].dispatchDate.empty()) {
                parcels[idx].dispatchDate = getCurrentDate();
            }
        }

        saveRecords();
        return true;
    }

    std::vector<Parcel> getPendingDispatches() const {
        std::vector<Parcel> pending;
        for (const auto& p : parcels) {
            if (p.status == ParcelStatus::REGISTERED) {
                pending.push_back(p);
            }
        }
        return pending;
    }

    int sequentialSearch(const std::string& trackingID, size_t& steps, double& timeNanoseconds) const {
        steps = 0;
        auto start = std::chrono::high_resolution_clock::now();

        int foundIndex = -1;
        for (size_t i = 0; i < parcels.size(); ++i) {
            steps++;
            if (parcels[i].trackingID == trackingID) {
                foundIndex = static_cast<int>(i);
                break;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        timeNanoseconds = std::chrono::duration<double, std::nano>(end - start).count();
        return foundIndex;
    }

    int hashSearch(const std::string& trackingID, size_t& steps, double& timeNanoseconds) const {
        steps = 1;
        auto start = std::chrono::high_resolution_clock::now();

        auto it = trackingIndex.find(trackingID);
        int foundIndex = (it != trackingIndex.end()) ? static_cast<int>(it->second) : -1;

        auto end = std::chrono::high_resolution_clock::now();
        timeNanoseconds = std::chrono::duration<double, std::nano>(end - start).count();
        return foundIndex;
    }

    void sortByTrackingID(bool ascending = true) {
        if (parcels.size() <= 1) return;
        mergeSort(0, static_cast<int>(parcels.size() - 1), false);
        if (!ascending) std::reverse(parcels.begin(), parcels.end());
        rebuildIndex();
        saveRecords();
    }

    void sortByDestination(bool ascending = true) {
        if (parcels.size() <= 1) return;
        mergeSort(0, static_cast<int>(parcels.size() - 1), true);
        if (!ascending) std::reverse(parcels.begin(), parcels.end());
        rebuildIndex();
        saveRecords();
    }

    static void displayParcelCard(const Parcel& p) {
        std::cout << "\n+--------------------------------------------------------------+\n";
        std::cout << "|                      PARCEL DETAILS CARD                     |\n";
        std::cout << "+--------------------------------------------------------------+\n";
        std::cout << "| Tracking ID       : " << std::left << std::setw(40) << p.trackingID << " |\n";
        std::cout << "| Sender            : " << std::left << std::setw(40) << p.sender << " |\n";
        std::cout << "| Recipient         : " << std::left << std::setw(40) << p.recipient << " |\n";
        std::cout << "| Destination       : " << std::left << std::setw(40) << p.destination << " |\n";
        std::cout << "| Current Status    : " << std::left << std::setw(40) << p.status << " |\n";
        std::cout << "| Registration Date : " << std::left << std::setw(40) << p.registrationDate << " |\n";
        std::cout << "| Dispatch Date     : " << std::left << std::setw(40) << p.dispatchDate << " |\n";
        std::cout << "+--------------------------------------------------------------+\n";
    }

    static void displayTable(const std::vector<Parcel>& records, const std::string& title = "PARCEL RECORDS") {
        std::cout << "\n========================================================================================================\n";
        std::cout << " " << title << " (Total: " << records.size() << " records)\n";
        std::cout << "========================================================================================================\n";
        std::cout << std::left << std::setw(12) << "Tracking ID"
                  << std::setw(18) << "Sender"
                  << std::setw(18) << "Recipient"
                  << std::setw(16) << "Destination"
                  << std::setw(14) << "Status"
                  << std::setw(14) << "Reg Date"
                  << std::setw(14) << "Dispatch Date" << "\n";
        std::cout << "--------------------------------------------------------------------------------------------------------\n";

        if (records.empty()) {
            std::cout << " [No records to display]\n";
        } else {
            for (const auto& p : records) {
                std::cout << std::left << std::setw(12) << p.trackingID
                          << std::setw(18) << (p.sender.length() > 16 ? p.sender.substr(0, 14) + ".." : p.sender)
                          << std::setw(18) << (p.recipient.length() > 16 ? p.recipient.substr(0, 14) + ".." : p.recipient)
                          << std::setw(16) << (p.destination.length() > 14 ? p.destination.substr(0, 12) + ".." : p.destination)
                          << std::setw(14) << p.status
                          << std::setw(14) << p.registrationDate
                          << std::setw(14) << p.dispatchDate << "\n";
            }
        }
        std::cout << "========================================================================================================\n";
    }

    void compareSearchForID(const std::string& trackingID) const {
        size_t seqSteps = 0, hashSteps = 0;
        double seqTime = 0.0, hashTime = 0.0;

        int seqIdx = sequentialSearch(trackingID, seqSteps, seqTime);
        int hashIdx = hashSearch(trackingID, hashSteps, hashTime);

        std::cout << "\n========================================================================\n";
        std::cout << "               SEARCH ALGORITHM PERFORMANCE COMPARISON                  \n";
        std::cout << "========================================================================\n";
        std::cout << " Target Tracking ID : " << trackingID << "\n";
        std::cout << " Total Records (N)  : " << parcels.size() << "\n";
        std::cout << "------------------------------------------------------------------------\n";
        std::cout << std::left << std::setw(22) << "Algorithm"
                  << std::setw(15) << "Result"
                  << std::setw(16) << "Steps/Checks"
                  << std::setw(16) << "Time (ns)"
                  << std::setw(12) << "Complexity" << "\n";
        std::cout << "------------------------------------------------------------------------\n";

        std::string seqRes = (seqIdx != -1) ? ("Found at #" + std::to_string(seqIdx + 1)) : "Not Found";
        std::string hashRes = (hashIdx != -1) ? ("Found at #" + std::to_string(hashIdx + 1)) : "Not Found";

        std::cout << std::left << std::setw(22) << "Sequential Search"
                  << std::setw(15) << seqRes
                  << std::setw(16) << seqSteps
                  << std::setw(16) << std::fixed << std::setprecision(1) << seqTime
                  << std::setw(12) << "O(N)" << "\n";

        std::cout << std::left << std::setw(22) << "Hash-Based Lookup"
                  << std::setw(15) << hashRes
                  << std::setw(16) << hashSteps
                  << std::setw(16) << std::fixed << std::setprecision(1) << hashTime
                  << std::setw(12) << "O(1) avg" << "\n";
        std::cout << "========================================================================\n";

        if (hashIdx != -1) {
            displayParcelCard(parcels[hashIdx]);
        } else {
            std::cout << " [!] Parcel with Tracking ID '" << trackingID << "' does not exist in registry.\n";
        }
    }

    void runBenchmark(size_t datasetSize, size_t queryCount) const {
        std::cout << "\n>>> Generating synthetic benchmark dataset of " << datasetSize << " parcels...\n";

        std::vector<Parcel> benchParcels;
        benchParcels.reserve(datasetSize);
        std::unordered_map<std::string, size_t> benchIndex;
        benchIndex.reserve(datasetSize);

        std::vector<std::string> cities = {"Mumbai", "Delhi", "Bangalore", "Pune", "Chennai",
                                           "Kolkata", "Hyderabad", "Ahmedabad", "Jaipur", "Surat"};
        std::vector<std::string> statuses = {ParcelStatus::REGISTERED, ParcelStatus::DISPATCHED,
                                             ParcelStatus::IN_TRANSIT, ParcelStatus::DELIVERED};

        for (size_t i = 0; i < datasetSize; ++i) {
            std::string id = "BENCH" + std::to_string(100000 + i);
            Parcel p(id, "Sender_" + std::to_string(i), "Recipient_" + std::to_string(i),
                     cities[i % cities.size()], statuses[i % statuses.size()], "2026-10-01", "-");
            benchParcels.push_back(p);
            benchIndex[id] = i;
        }

        std::mt19937 rng(42);
        std::uniform_int_distribution<size_t> dist(0, datasetSize - 1);
        std::vector<std::string> queries;
        queries.reserve(queryCount);
        for (size_t i = 0; i < queryCount; ++i) {
            queries.push_back(benchParcels[dist(rng)].trackingID);
        }

        std::cout << ">>> Running " << queryCount << " queries using Sequential Search (Linear Scan)..." << std::flush;
        auto seqStart = std::chrono::high_resolution_clock::now();
        size_t seqFoundCount = 0;
        for (const auto& q : queries) {
            for (size_t i = 0; i < benchParcels.size(); ++i) {
                if (benchParcels[i].trackingID == q) {
                    seqFoundCount++;
                    break;
                }
            }
        }
        auto seqEnd = std::chrono::high_resolution_clock::now();
        double seqDurationMs = std::chrono::duration<double, std::milli>(seqEnd - seqStart).count();
        std::cout << " Done!\n";

        std::cout << ">>> Running " << queryCount << " queries using Hash-Based Lookup (unordered_map)..." << std::flush;
        auto hashStart = std::chrono::high_resolution_clock::now();
        size_t hashFoundCount = 0;
        for (const auto& q : queries) {
            auto it = benchIndex.find(q);
            if (it != benchIndex.end()) {
                hashFoundCount++;
            }
        }
        auto hashEnd = std::chrono::high_resolution_clock::now();
        double hashDurationMs = std::chrono::duration<double, std::milli>(hashEnd - hashStart).count();
        std::cout << " Done!\n";

        double speedup = (hashDurationMs > 0.0) ? (seqDurationMs / hashDurationMs) : 0.0;
        double seqAvgUs = (seqDurationMs * 1000.0) / queryCount;
        double hashAvgUs = (hashDurationMs * 1000.0) / queryCount;

        std::cout << "\n========================================================================================\n";
        std::cout << "                   EMPIRICAL BENCHMARK RESULTS (N = " << datasetSize << ", Queries = " << queryCount << ")             \n";
        std::cout << "========================================================================================\n";
        std::cout << std::left << std::setw(25) << "Algorithm"
                  << std::setw(15) << "Total Time (ms)"
                  << std::setw(18) << "Avg Time/Query"
                  << std::setw(15) << "Theoretical"
                  << std::setw(15) << "Items Located" << "\n";
        std::cout << "----------------------------------------------------------------------------------------\n";
        std::cout << std::left << std::setw(25) << "Sequential Search"
                  << std::setw(15) << std::fixed << std::setprecision(2) << seqDurationMs
                  << (std::to_string(seqAvgUs).substr(0, 6) + " us")
                  << std::setw(12) << " "
                  << std::setw(15) << "O(N)"
                  << std::setw(15) << seqFoundCount << "\n";
        std::cout << std::left << std::setw(25) << "Hash-Based Lookup"
                  << std::setw(15) << std::fixed << std::setprecision(2) << hashDurationMs
                  << (std::to_string(hashAvgUs).substr(0, 6) + " us")
                  << std::setw(12) << " "
                  << std::setw(15) << "O(1) average"
                  << std::setw(15) << hashFoundCount << "\n";
        std::cout << "========================================================================================\n";
        std::cout << " SPEEDUP FACTOR: Hash Lookup is " << std::fixed << std::setprecision(1)
                  << speedup << "x FASTER than Sequential Search on " << datasetSize << " records!\n";
        std::cout << "========================================================================================\n\n";
    }
};

// Web Server Implementation
class WebServer {
private:
    ParcelManager& manager;
    int port;
    std::string webDir;

    static std::string escapeJson(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            if (c == '"') o << "\\\"";
            else if (c == '\\') o << "\\\\";
            else if (c == '\n') o << "\\n";
            else if (c == '\r') o << "\\r";
            else if (c == '\t') o << "\\t";
            else o << c;
        }
        return o.str();
    }

    static std::string parcelToJson(const Parcel& p) {
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

    static std::string parcelsToJsonArray(const std::vector<Parcel>& list) {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < list.size(); ++i) {
            ss << parcelToJson(list[i]);
            if (i + 1 < list.size()) ss << ",";
        }
        ss << "]";
        return ss.str();
    }

    static std::string urlDecode(const std::string& in) {
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

    static std::string getQueryParam(const std::string& query, const std::string& key) {
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

    static std::string getJsonField(const std::string& json, const std::string& field) {
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

    std::string serveStaticFile(const std::string& filepath, std::string& contentType) {
        std::string fullPath = webDir + "/" + filepath;
        if (filepath == "index.html" || filepath.empty() || filepath == "/") {
            fullPath = webDir + "/index.html";
            contentType = "text/html; charset=utf-8";
        } else if (filepath.rfind(".css") != std::string::npos) {
            contentType = "text/css; charset=utf-8";
        } else if (filepath.rfind(".js") != std::string::npos) {
            contentType = "application/javascript; charset=utf-8";
        } else {
            contentType = "text/plain; charset=utf-8";
        }

        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) return "";

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::string handleApiRequest(const std::string& method, const std::string& path,
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
            if (id.empty()) return "{\"error\":\"Missing 'id' parameter\"}";

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

            if (id.empty()) id = getQueryParam(body, "trackingID");
            if (sender.empty()) sender = getQueryParam(body, "sender");
            if (recipient.empty()) recipient = getQueryParam(body, "recipient");
            if (destination.empty()) destination = getQueryParam(body, "destination");
            if (regDate.empty()) regDate = getQueryParam(body, "registrationDate");

            if (regDate.empty()) regDate = ParcelManager::getCurrentDate();
            if (id.empty()) return "{\"success\":false,\"error\":\"Tracking ID cannot be empty.\"}";
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

            if (id.empty() || !manager.contains(id)) return "{\"success\":false,\"error\":\"Parcel not found.\"}";

            bool ok = manager.updateStatus(id, status, dispDate);
            if (ok) {
                size_t dummySteps = 0; double dummyTime = 0.0;
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
            for (size_t i = 0; i < q; ++i) queries.push_back(benchParcels[dist(rng)].trackingID);

            auto s1 = std::chrono::high_resolution_clock::now();
            size_t seqFound = 0;
            for (const auto& target : queries) {
                for (size_t i = 0; i < benchParcels.size(); ++i) {
                    if (benchParcels[i].trackingID == target) { seqFound++; break; }
                }
            }
            auto s2 = std::chrono::high_resolution_clock::now();
            double seqMs = std::chrono::duration<double, std::milli>(s2 - s1).count();

            auto h1 = std::chrono::high_resolution_clock::now();
            size_t hashFound = 0;
            for (const auto& target : queries) {
                if (benchIndex.find(target) != benchIndex.end()) hashFound++;
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

public:
    WebServer(ParcelManager& mgr, int portNum = 5000, std::string dir = "web")
        : manager(mgr), port(portNum), webDir(std::move(dir)) {}

    void start() {
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
            std::cerr << "[Error] Could not bind to port " << port << ". Try another port.\n";
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
            if (client_sock < 0) continue;

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

            std::string path = fullUri;
            std::string query;
            size_t qPos = fullUri.find('?');
            if (qPos != std::string::npos) {
                path = fullUri.substr(0, qPos);
                query = fullUri.substr(qPos + 1);
            }

            std::string body;
            size_t headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                body = request.substr(headerEnd + 4);
            }

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

            std::string responseBody;
            std::string contentType = "text/html; charset=utf-8";
            int statusCode = 200;

            if (path.rfind("/api/", 0) == 0) {
                responseBody = handleApiRequest(method, path, query, body, contentType);
            } else {
                std::string filename = (path == "/" || path.empty()) ? "index.html" : path.substr(1);
                if (filename.rfind("web/", 0) == 0) filename = filename.substr(4);
                responseBody = serveStaticFile(filename, contentType);
                if (responseBody.empty()) {
                    statusCode = 404;
                    responseBody = "<h1>404 Not Found</h1>";
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
};

void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string promptString(const std::string& prompt, bool allowEmpty = false) {
    std::string input;
    while (true) {
        std::cout << prompt;
        std::getline(std::cin, input);
        input = ParcelManager::trim(input);
        if (allowEmpty || !input.empty()) {
            if (input.find('|') != std::string::npos) {
                std::cout << " [!] Error: The pipe character '|' is reserved as a file delimiter. Please re-enter.\n";
                continue;
            }
            return input;
        }
        std::cout << " [!] Input cannot be empty. Please try again.\n";
    }
}

void printHeader() {
    std::cout << "\n========================================================================\n";
    std::cout << "               COURIER PARCEL TRACKING AND DISPATCH                     \n";
    std::cout << "       CP Data Structures Project | Vector + Hash-Based Lookup          \n";
    std::cout << "========================================================================\n";
}

void printMenu() {
    std::cout << "\n[ MAIN MENU ]\n";
    std::cout << " 1. Register New Parcel\n";
    std::cout << " 2. Search Parcel by Tracking ID (Sequential vs Hash Search)\n";
    std::cout << " 3. Update Parcel Status\n";
    std::cout << " 4. Display Pending Dispatches (status = Registered)\n";
    std::cout << " 5. Sort Parcels (by Destination or Tracking ID)\n";
    std::cout << " 6. Display All Records\n";
    std::cout << " 7. Run Search Benchmark (Empirical Complexity Analysis)\n";
    std::cout << " 8. Save & Reload Records\n";
    std::cout << " 9. Launch Web Server Mode (http://localhost:5000)\n";
    std::cout << " 10. Exit\n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << "Enter your choice (1-10): ";
}

void handleRegister(ParcelManager& manager) {
    std::cout << "\n--- [1] REGISTER NEW PARCEL ---\n";
    std::string id = promptString("Enter Unique Tracking ID (e.g. CR101): ");

    if (manager.contains(id)) {
        std::cout << " [!] Error: Tracking ID '" << id << "' already exists! Duplicate rejected.\n";
        return;
    }

    std::string sender = promptString("Enter Sender Name: ");
    std::string recipient = promptString("Enter Recipient Name: ");
    std::string destination = promptString("Enter Destination City: ");

    std::string defaultDate = ParcelManager::getCurrentDate();
    std::string regPrompt = "Enter Registration Date (YYYY-MM-DD) [Default: " + defaultDate + "]: ";
    std::string regDate = promptString(regPrompt, true);
    if (regDate.empty()) regDate = defaultDate;

    Parcel p(id, sender, recipient, destination, ParcelStatus::REGISTERED, regDate, "-");
    if (manager.registerParcel(p)) {
        std::cout << " [OK] Parcel '" << id << "' registered successfully and saved to registry!\n";
        ParcelManager::displayParcelCard(p);
    } else {
        std::cout << " [!] Failed to register parcel.\n";
    }
}

void handleSearch(const ParcelManager& manager) {
    std::cout << "\n--- [2] SEARCH PARCEL BY TRACKING ID ---\n";
    std::string id = promptString("Enter Tracking ID to search: ");
    manager.compareSearchForID(id);
}

void handleUpdateStatus(ParcelManager& manager) {
    std::cout << "\n--- [3] UPDATE PARCEL STATUS ---\n";
    std::string id = promptString("Enter Tracking ID to update: ");

    if (!manager.contains(id)) {
        std::cout << " [!] Error: Parcel '" << id << "' not found in registry.\n";
        return;
    }

    size_t dummySteps = 0;
    double dummyTime = 0.0;
    int idx = manager.hashSearch(id, dummySteps, dummyTime);
    const Parcel* current = manager.getParcelByIndex(idx);

    std::cout << "\nCurrent details for " << id << ":\n";
    std::cout << " Current Status: " << current->status << "\n";
    std::cout << " Dispatch Date : " << current->dispatchDate << "\n";

    std::cout << "\nSelect New Status:\n";
    std::cout << " 1. " << ParcelStatus::REGISTERED << "\n";
    std::cout << " 2. " << ParcelStatus::DISPATCHED << "\n";
    std::cout << " 3. " << ParcelStatus::IN_TRANSIT << "\n";
    std::cout << " 4. " << ParcelStatus::DELIVERED << "\n";
    std::cout << " 5. " << ParcelStatus::CANCELLED << "\n";
    std::cout << "Choice (1-5): ";

    int choice = 0;
    if (!(std::cin >> choice)) {
        clearInputBuffer();
        std::cout << " [!] Invalid input.\n";
        return;
    }
    clearInputBuffer();

    std::string newStatus;
    switch (choice) {
        case 1: newStatus = ParcelStatus::REGISTERED; break;
        case 2: newStatus = ParcelStatus::DISPATCHED; break;
        case 3: newStatus = ParcelStatus::IN_TRANSIT; break;
        case 4: newStatus = ParcelStatus::DELIVERED; break;
        case 5: newStatus = ParcelStatus::CANCELLED; break;
        default:
            std::cout << " [!] Invalid status selection.\n";
            return;
    }

    std::string dispatchDate = current->dispatchDate;
    if (newStatus == ParcelStatus::DISPATCHED || newStatus == ParcelStatus::IN_TRANSIT || newStatus == ParcelStatus::DELIVERED) {
        std::string today = ParcelManager::getCurrentDate();
        std::string dispPrompt = "Enter Dispatch Date (YYYY-MM-DD) [Default: " + (dispatchDate == "-" ? today : dispatchDate) + "]: ";
        std::string inputDate = promptString(dispPrompt, true);
        if (!inputDate.empty()) {
            dispatchDate = inputDate;
        } else if (dispatchDate == "-") {
            dispatchDate = today;
        }
    }

    if (manager.updateStatus(id, newStatus, dispatchDate)) {
        std::cout << " [OK] Status of parcel '" << id << "' updated to '" << newStatus << "' successfully!\n";
        int updatedIdx = manager.hashSearch(id, dummySteps, dummyTime);
        ParcelManager::displayParcelCard(*manager.getParcelByIndex(updatedIdx));
    } else {
        std::cout << " [!] Failed to update status.\n";
    }
}

void handlePending(const ParcelManager& manager) {
    std::cout << "\n--- [4] PENDING DISPATCHES ---\n";
    auto pending = manager.getPendingDispatches();
    ParcelManager::displayTable(pending, "PARCELS AWAITING DISPATCH (Status: Registered)");
}

void handleSort(ParcelManager& manager) {
    std::cout << "\n--- [5] SORT PARCEL RECORDS ---\n";
    std::cout << "Sort criteria:\n";
    std::cout << " 1. Sort by Destination (Alphabetical)\n";
    std::cout << " 2. Sort by Tracking ID\n";
    std::cout << "Choice (1-2): ";

    int sortChoice = 0;
    if (!(std::cin >> sortChoice)) {
        clearInputBuffer();
        std::cout << " [!] Invalid input.\n";
        return;
    }
    clearInputBuffer();

    std::cout << "Order:\n";
    std::cout << " 1. Ascending (A-Z / Min-Max)\n";
    std::cout << " 2. Descending (Z-A / Max-Min)\n";
    std::cout << "Choice (1-2): ";

    int orderChoice = 0;
    if (!(std::cin >> orderChoice)) {
        clearInputBuffer();
        std::cout << " [!] Invalid input.\n";
        return;
    }
    clearInputBuffer();

    bool ascending = (orderChoice != 2);

    if (sortChoice == 1) {
        manager.sortByDestination(ascending);
        std::cout << " [OK] Records sorted by Destination (" << (ascending ? "Ascending" : "Descending") << ") using MergeSort (O(N log N))!\n";
    } else if (sortChoice == 2) {
        manager.sortByTrackingID(ascending);
        std::cout << " [OK] Records sorted by Tracking ID (" << (ascending ? "Ascending" : "Descending") << ") using MergeSort (O(N log N))!\n";
    } else {
        std::cout << " [!] Invalid sort option.\n";
        return;
    }

    ParcelManager::displayTable(manager.getAllParcels(), "SORTED PARCEL RECORDS");
}

void handleDisplayAll(const ParcelManager& manager) {
    ParcelManager::displayTable(manager.getAllParcels(), "ALL PARCEL RECORDS IN REGISTRY");
}

void handleBenchmark(const ParcelManager& manager) {
    std::cout << "\n--- [7] SEARCH PERFORMANCE BENCHMARK ---\n";
    std::cout << "Select dataset size for empirical comparison:\n";
    std::cout << " 1. Small    (N = 1,000 records,   1,000 queries)\n";
    std::cout << " 2. Medium   (N = 10,000 records,  5,000 queries) [Recommended]\n";
    std::cout << " 3. Large    (N = 50,000 records,  10,000 queries)\n";
    std::cout << " 4. Custom size\n";
    std::cout << "Choice (1-4): ";

    int choice = 0;
    if (!(std::cin >> choice)) {
        clearInputBuffer();
        std::cout << " [!] Invalid input.\n";
        return;
    }
    clearInputBuffer();

    size_t n = 10000;
    size_t q = 5000;

    if (choice == 1) {
        n = 1000; q = 1000;
    } else if (choice == 2) {
        n = 10000; q = 5000;
    } else if (choice == 3) {
        n = 50000; q = 10000;
    } else if (choice == 4) {
        std::cout << "Enter custom record count (N): ";
        if (!(std::cin >> n)) { clearInputBuffer(); return; }
        std::cout << "Enter query count: ";
        if (!(std::cin >> q)) { clearInputBuffer(); return; }
        clearInputBuffer();
    } else {
        std::cout << " [!] Invalid choice.\n";
        return;
    }

    manager.runBenchmark(n, q);
}

void handleReload(ParcelManager& manager) {
    std::cout << "\n--- [8] SAVE & RELOAD RECORDS ---\n";
    manager.saveRecords();
    bool ok = manager.loadRecords();
    if (ok) {
        std::cout << " [OK] Records successfully synchronized and reloaded from disk ("
                  << manager.count() << " parcels currently loaded).\n";
    } else {
        std::cout << " [!] Error reloading records from file.\n";
    }
}

int main(int argc, char* argv[]) {
    ParcelManager manager("parcels.txt");
    int port = 5000;
    bool webMode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--web" || arg == "-w" || arg == "--serve") {
            webMode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                port = std::stoi(argv[++i]);
            }
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
            webMode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: ./courier_tracker [OPTIONS]\n"
                      << "  --web, -w [port]    Launch native C++ web server (default: 5000)\n"
                      << "  --port [port]       Specify web server port\n"
                      << "  --cli               Launch interactive CLI menu\n"
                      << "  --help, -h          Show this help message\n";
            return 0;
        }
    }

    if (webMode) {
        WebServer server(manager, port, "web");
        server.start();
        return 0;
    }

    printHeader();
    std::cout << " Loaded " << manager.count() << " parcel records from persistent storage (parcels.txt).\n";

    bool running = true;
    while (running) {
        printMenu();
        int choice = 0;
        if (!(std::cin >> choice)) {
            clearInputBuffer();
            std::cout << " [!] Please enter a valid number from 1 to 10.\n";
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: handleRegister(manager); break;
            case 2: handleSearch(manager); break;
            case 3: handleUpdateStatus(manager); break;
            case 4: handlePending(manager); break;
            case 5: handleSort(manager); break;
            case 6: handleDisplayAll(manager); break;
            case 7: handleBenchmark(manager); break;
            case 8: handleReload(manager); break;
            case 9: {
                WebServer server(manager, port, "web");
                server.start();
                break;
            }
            case 10:
                manager.saveRecords();
                std::cout << "\nAll records saved to 'parcels.txt'. Exiting Courier Parcel Manager. Goodbye!\n\n";
                running = false;
                break;
            default:
                std::cout << " [!] Invalid option. Please choose between 1 and 10.\n";
                break;
        }
    }

    return 0;
}
