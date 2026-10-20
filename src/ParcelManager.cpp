#include "../include/ParcelManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <random>

ParcelManager::ParcelManager(std::string storageFile)
    : filename(std::move(storageFile)) {
    loadRecords();
}

std::string ParcelManager::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string ParcelManager::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    localtime_r(&now_c, &tm_buf);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);
    return std::string(buf);
}

void ParcelManager::rebuildIndex() {
    trackingIndex.clear();
    for (size_t i = 0; i < parcels.size(); ++i) {
        trackingIndex[parcels[i].trackingID] = i;
    }
}

bool ParcelManager::loadRecords() {
    parcels.clear();
    trackingIndex.clear();

    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue; // Skip comments and empty lines
        }

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

bool ParcelManager::saveRecords() const {
    std::ofstream outFile(filename, std::ios::trunc);
    if (!outFile.is_open()) {
        std::cerr << "[Error] Unable to open file " << filename << " for saving records.\n";
        return false;
    }

    for (const auto& p : parcels) {
        outFile << p.toFileLine() << "\n";
    }

    outFile.close();
    return true;
}

bool ParcelManager::contains(const std::string& trackingID) const {
    return trackingIndex.find(trackingID) != trackingIndex.end();
}

size_t ParcelManager::count() const {
    return parcels.size();
}

const std::vector<Parcel>& ParcelManager::getAllParcels() const {
    return parcels;
}

const Parcel* ParcelManager::getParcelByIndex(size_t index) const {
    if (index < parcels.size()) {
        return &parcels[index];
    }
    return nullptr;
}

bool ParcelManager::registerParcel(const Parcel& parcel) {
    if (parcel.trackingID.empty()) {
        return false;
    }
    if (contains(parcel.trackingID)) {
        return false; // Duplicate tracking ID rejected
    }

    parcels.push_back(parcel);
    trackingIndex[parcel.trackingID] = parcels.size() - 1;
    saveRecords();
    return true;
}

bool ParcelManager::updateStatus(const std::string& trackingID, const std::string& newStatus, const std::string& dispatchDate) {
    auto it = trackingIndex.find(trackingID);
    if (it == trackingIndex.end()) {
        return false;
    }

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

std::vector<Parcel> ParcelManager::getPendingDispatches() const {
    std::vector<Parcel> pending;
    for (const auto& p : parcels) {
        if (p.status == ParcelStatus::REGISTERED) {
            pending.push_back(p);
        }
    }
    return pending;
}

// Sequential / Linear Search: O(n)
int ParcelManager::sequentialSearch(const std::string& trackingID, size_t& steps, double& timeNanoseconds) const {
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

// Hash-based Lookup: Average O(1)
int ParcelManager::hashSearch(const std::string& trackingID, size_t& steps, double& timeNanoseconds) const {
    steps = 1;
    auto start = std::chrono::high_resolution_clock::now();

    auto it = trackingIndex.find(trackingID);
    int foundIndex = (it != trackingIndex.end()) ? static_cast<int>(it->second) : -1;

    auto end = std::chrono::high_resolution_clock::now();
    timeNanoseconds = std::chrono::duration<double, std::nano>(end - start).count();
    return foundIndex;
}

// Merge routine for custom MergeSort (O(N log N))
void ParcelManager::merge(int left, int mid, int right, bool byDestination) {
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

// Recursive MergeSort (O(N log N))
void ParcelManager::mergeSort(int left, int right, bool byDestination) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(left, mid, byDestination);
        mergeSort(mid + 1, right, byDestination);
        merge(left, mid, right, byDestination);
    }
}

void ParcelManager::sortByTrackingID(bool ascending) {
    if (parcels.size() <= 1) return;
    mergeSort(0, static_cast<int>(parcels.size() - 1), false);
    if (!ascending) {
        std::reverse(parcels.begin(), parcels.end());
    }
    rebuildIndex();
    saveRecords();
}

void ParcelManager::sortByDestination(bool ascending) {
    if (parcels.size() <= 1) return;
    mergeSort(0, static_cast<int>(parcels.size() - 1), true);
    if (!ascending) {
        std::reverse(parcels.begin(), parcels.end());
    }
    rebuildIndex();
    saveRecords();
}

void ParcelManager::compareSearchForID(const std::string& trackingID) const {
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

void ParcelManager::runBenchmark(size_t datasetSize, size_t queryCount) const {
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

    // Pick random target IDs from dataset
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

void ParcelManager::displayTable(const std::vector<Parcel>& records, const std::string& title) {
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

void ParcelManager::displayParcelCard(const Parcel& p) {
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
