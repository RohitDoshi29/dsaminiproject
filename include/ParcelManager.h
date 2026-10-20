#ifndef PARCEL_MANAGER_H
#define PARCEL_MANAGER_H

#include "Parcel.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>

class ParcelManager {
private:
    std::vector<Parcel> parcels;
    std::unordered_map<std::string, size_t> trackingIndex;
    std::string filename;

    // Custom sorting algorithms
    void mergeSort(int left, int right, bool byDestination);
    void merge(int left, int mid, int right, bool byDestination);

public:
    explicit ParcelManager(std::string storageFile = "parcels.txt");

    // File Persistence
    bool loadRecords();
    bool saveRecords() const;
    void rebuildIndex();

    // Core Operations
    bool registerParcel(const Parcel& parcel);
    bool updateStatus(const std::string& trackingID, const std::string& newStatus, const std::string& dispatchDate = "");
    bool contains(const std::string& trackingID) const;
    size_t count() const;
    const std::vector<Parcel>& getAllParcels() const;
    std::vector<Parcel> getPendingDispatches() const;
    const Parcel* getParcelByIndex(size_t index) const;

    // Search Algorithms
    // Returns index in parcels vector or -1 if not found
    int sequentialSearch(const std::string& trackingID, size_t& steps, double& timeNanoseconds) const;
    int hashSearch(const std::string& trackingID, size_t& steps, double& timeNanoseconds) const;

    // Sorting
    void sortByTrackingID(bool ascending = true);
    void sortByDestination(bool ascending = true);

    // Benchmarking & Comparison
    void compareSearchForID(const std::string& trackingID) const;
    void runBenchmark(size_t datasetSize, size_t queryCount) const;

    // Formatting & Display
    static void displayTable(const std::vector<Parcel>& records, const std::string& title = "PARCEL RECORDS");
    static void displayParcelCard(const Parcel& p);
    static std::string trim(const std::string& str);
    static std::string getCurrentDate();
};

#endif // PARCEL_MANAGER_H
