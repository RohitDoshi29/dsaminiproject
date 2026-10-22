#include "../include/Parcel.h"
#include "../include/ParcelManager.h"
#include <iostream>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <regex>

void testParcelModel() {
    std::cout << "[TEST] Parcel Model & Status Validation... " << std::flush;
    
    // Default constructor
    Parcel p1;
    assert(p1.trackingID == "");
    assert(p1.status == ParcelStatus::REGISTERED);
    assert(p1.dispatchDate == "-");

    // Parameterized constructor
    Parcel p2("CR999", "Alice", "Bob", "Delhi", ParcelStatus::DISPATCHED, "2026-10-01", "2026-10-02");
    assert(p2.trackingID == "CR999");
    assert(p2.sender == "Alice");
    assert(p2.recipient == "Bob");
    assert(p2.destination == "Delhi");
    assert(p2.status == ParcelStatus::DISPATCHED);
    assert(p2.registrationDate == "2026-10-01");
    assert(p2.dispatchDate == "2026-10-02");

    // toFileLine
    std::string line = p2.toFileLine();
    assert(line == "CR999 | Alice | Bob | Delhi | Dispatched | 2026-10-01 | 2026-10-02");

    // Status Validation
    assert(ParcelStatus::isValidStatus(ParcelStatus::REGISTERED) == true);
    assert(ParcelStatus::isValidStatus(ParcelStatus::DISPATCHED) == true);
    assert(ParcelStatus::isValidStatus(ParcelStatus::IN_TRANSIT) == true);
    assert(ParcelStatus::isValidStatus(ParcelStatus::DELIVERED) == true);
    assert(ParcelStatus::isValidStatus(ParcelStatus::CANCELLED) == true);
    assert(ParcelStatus::isValidStatus("UnknownStatus") == false);
    assert(ParcelStatus::isValidStatus("") == false);

    std::cout << "PASSED\n";
}

void testTrimAndDate() {
    std::cout << "[TEST] String Trim & Date Helpers... " << std::flush;

    assert(ParcelManager::trim("  hello  ") == "hello");
    assert(ParcelManager::trim("\t\r\nworld\n") == "world");
    assert(ParcelManager::trim("   ") == "");
    assert(ParcelManager::trim("already_clean") == "already_clean");
    assert(ParcelManager::trim(" a b c ") == "a b c");

    std::string date = ParcelManager::getCurrentDate();
    assert(date.length() == 10);
    assert(date[4] == '-' && date[7] == '-');

    std::cout << "PASSED\n";
}

void testRegistrationAndDuplicates() {
    std::cout << "[TEST] Registration & Duplicate Rejection... " << std::flush;
    
    std::string testFile = "tests/temp_test.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    assert(pm.count() == 0);

    // Empty ID rejection
    Parcel emptyId("", "A", "B", "C", "Registered", "2026-10-01", "-");
    assert(pm.registerParcel(emptyId) == false);
    assert(pm.count() == 0);

    // Valid registration
    Parcel p1("TEST01", "Sender1", "Receiver1", "Mumbai", ParcelStatus::REGISTERED, "2026-10-01", "-");
    assert(pm.registerParcel(p1) == true);
    assert(pm.count() == 1);
    assert(pm.contains("TEST01") == true);
    assert(pm.contains("TEST99") == false);

    // Duplicate rejection
    Parcel p1Duplicate("TEST01", "Sender2", "Receiver2", "Delhi", ParcelStatus::REGISTERED, "2026-10-02", "-");
    assert(pm.registerParcel(p1Duplicate) == false);
    assert(pm.count() == 1);

    // Second valid parcel
    Parcel p2("TEST02", "Sender2", "Receiver2", "Pune", ParcelStatus::REGISTERED, "2026-10-02", "-");
    assert(pm.registerParcel(p2) == true);
    assert(pm.count() == 2);

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testSearchAlgorithms() {
    std::cout << "[TEST] Sequential vs Hash Search... " << std::flush;

    std::string testFile = "tests/temp_search.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    for (int i = 1; i <= 20; ++i) {
        std::string id = "ID_" + std::to_string(i);
        pm.registerParcel(Parcel(id, "S", "R", "City", ParcelStatus::REGISTERED, "2026-10-01", "-"));
    }
    assert(pm.count() == 20);

    // Search for 5th parcel (ID_5)
    size_t seqSteps = 0, hashSteps = 0;
    double seqTime = 0.0, hashTime = 0.0;
    int seqIdx = pm.sequentialSearch("ID_5", seqSteps, seqTime);
    int hashIdx = pm.hashSearch("ID_5", hashSteps, hashTime);

    assert(seqIdx == 4);
    assert(hashIdx == 4);
    assert(seqSteps == 5);
    assert(hashSteps == 1);

    // Search for non-existent parcel
    seqIdx = pm.sequentialSearch("NON_EXISTENT", seqSteps, seqTime);
    hashIdx = pm.hashSearch("NON_EXISTENT", hashSteps, hashTime);
    assert(seqIdx == -1);
    assert(hashIdx == -1);
    assert(seqSteps == 20); // Checked all 20 elements

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testStatusUpdates() {
    std::cout << "[TEST] Lifecycle Status Updates & Auto-Dispatch Dates... " << std::flush;

    std::string testFile = "tests/temp_status.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    pm.registerParcel(Parcel("CR101", "Sender", "Recipient", "Mumbai", ParcelStatus::REGISTERED, "2026-10-01", "-"));

    // Update non-existent returns false
    assert(pm.updateStatus("UNKNOWN", ParcelStatus::DISPATCHED) == false);

    // Update to Dispatched without date -> should auto-fill dispatch date with current date
    assert(pm.updateStatus("CR101", ParcelStatus::DISPATCHED) == true);
    size_t steps = 0; double t = 0;
    int idx = pm.hashSearch("CR101", steps, t);
    const Parcel* p = pm.getParcelByIndex(idx);
    assert(p != nullptr);
    assert(p->status == ParcelStatus::DISPATCHED);
    assert(p->dispatchDate == ParcelManager::getCurrentDate());

    // Update to Delivered with explicit date
    assert(pm.updateStatus("CR101", ParcelStatus::DELIVERED, "2026-10-25") == true);
    p = pm.getParcelByIndex(idx);
    assert(p->status == ParcelStatus::DELIVERED);
    assert(p->dispatchDate == "2026-10-25");

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testPendingDispatches() {
    std::cout << "[TEST] Pending Dispatches Filter... " << std::flush;

    std::string testFile = "tests/temp_pending.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    pm.registerParcel(Parcel("P1", "S", "R", "D", ParcelStatus::REGISTERED, "2026-10-01", "-"));
    pm.registerParcel(Parcel("P2", "S", "R", "D", ParcelStatus::DISPATCHED, "2026-10-01", "2026-10-02"));
    pm.registerParcel(Parcel("P3", "S", "R", "D", ParcelStatus::REGISTERED, "2026-10-01", "-"));
    pm.registerParcel(Parcel("P4", "S", "R", "D", ParcelStatus::DELIVERED, "2026-10-01", "2026-10-03"));

    auto pending = pm.getPendingDispatches();
    assert(pending.size() == 2);
    assert(pending[0].trackingID == "P1");
    assert(pending[1].trackingID == "P3");

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testSortingAlgorithms() {
    std::cout << "[TEST] MergeSort by Destination & Tracking ID... " << std::flush;

    std::string testFile = "tests/temp_sort.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    pm.registerParcel(Parcel("CR103", "S", "R", "Mumbai", "Registered", "2026-10-01", "-"));
    pm.registerParcel(Parcel("CR101", "S", "R", "Delhi", "Registered", "2026-10-01", "-"));
    pm.registerParcel(Parcel("CR102", "S", "R", "Bangalore", "Registered", "2026-10-01", "-"));
    pm.registerParcel(Parcel("CR104", "S", "R", "Chennai", "Registered", "2026-10-01", "-"));

    // 1. Sort by Destination Ascending
    pm.sortByDestination(true);
    auto& parcels = pm.getAllParcels();
    assert(parcels[0].destination == "Bangalore");
    assert(parcels[1].destination == "Chennai");
    assert(parcels[2].destination == "Delhi");
    assert(parcels[3].destination == "Mumbai");

    // Verify Hash Index rebuilt after permutation
    size_t steps = 0; double t = 0;
    assert(pm.hashSearch("CR102", steps, t) == 0); // Bangalore is at 0
    assert(pm.hashSearch("CR104", steps, t) == 1); // Chennai is at 1
    assert(pm.hashSearch("CR101", steps, t) == 2); // Delhi is at 2
    assert(pm.hashSearch("CR103", steps, t) == 3); // Mumbai is at 3

    // 2. Sort by Destination Descending
    pm.sortByDestination(false);
    assert(parcels[0].destination == "Mumbai");
    assert(parcels[1].destination == "Delhi");
    assert(parcels[2].destination == "Chennai");
    assert(parcels[3].destination == "Bangalore");

    // 3. Sort by Tracking ID Ascending
    pm.sortByTrackingID(true);
    assert(parcels[0].trackingID == "CR101");
    assert(parcels[1].trackingID == "CR102");
    assert(parcels[2].trackingID == "CR103");
    assert(parcels[3].trackingID == "CR104");

    // 4. Sort by Tracking ID Descending
    pm.sortByTrackingID(false);
    assert(parcels[0].trackingID == "CR104");
    assert(parcels[1].trackingID == "CR103");
    assert(parcels[2].trackingID == "CR102");
    assert(parcels[3].trackingID == "CR101");

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testPersistence() {
    std::cout << "[TEST] File Persistence (Save & Reload)... " << std::flush;

    std::string testFile = "tests/temp_persist.txt";
    std::remove(testFile.c_str());

    {
        ParcelManager pm(testFile);
        pm.registerParcel(Parcel("CR101", "Sender One", "Recipient One", "City A", "Registered", "2026-10-10", "-"));
        pm.registerParcel(Parcel("CR102", "Sender Two", "Recipient Two", "City B", "Dispatched", "2026-10-11", "2026-10-12"));
        assert(pm.count() == 2);
    }

    // Now reload in a completely new instance
    {
        ParcelManager pm(testFile);
        assert(pm.count() == 2);
        assert(pm.contains("CR101") == true);
        assert(pm.contains("CR102") == true);

        size_t steps = 0; double t = 0;
        int idx = pm.hashSearch("CR102", steps, t);
        const Parcel* p = pm.getParcelByIndex(idx);
        assert(p != nullptr);
        assert(p->sender == "Sender Two");
        assert(p->recipient == "Recipient Two");
        assert(p->destination == "City B");
        assert(p->status == "Dispatched");
        assert(p->registrationDate == "2026-10-11");
        assert(p->dispatchDate == "2026-10-12");
    }

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testBoundaryConditions() {
    std::cout << "[TEST] Boundary & Index Bounds... " << std::flush;

    std::string testFile = "tests/temp_bounds.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    assert(pm.getParcelByIndex(0) == nullptr);
    assert(pm.getParcelByIndex(100) == nullptr);

    pm.registerParcel(Parcel("C1", "S", "R", "D", "Registered", "2026-10-01", "-"));
    assert(pm.getParcelByIndex(0) != nullptr);
    assert(pm.getParcelByIndex(1) == nullptr);

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

void testBenchmarkExecution() {
    std::cout << "[TEST] Empirical Benchmark Runner (Small Run)... " << std::flush;
    std::string testFile = "tests/temp_bench.txt";
    std::remove(testFile.c_str());

    ParcelManager pm(testFile);
    // Run small benchmark of 200 parcels and 100 queries
    pm.runBenchmark(200, 100);

    std::remove(testFile.c_str());
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "======================================================================\n";
    std::cout << "          PARCEL MANAGER C++ UNIT & INTEGRATION TEST SUITE             \n";
    std::cout << "======================================================================\n";

    testParcelModel();
    testTrimAndDate();
    testRegistrationAndDuplicates();
    testSearchAlgorithms();
    testStatusUpdates();
    testPendingDispatches();
    testSortingAlgorithms();
    testPersistence();
    testBoundaryConditions();
    testBenchmarkExecution();

    std::cout << "======================================================================\n";
    std::cout << "          ALL C++ UNIT & ALGORITHM TESTS PASSED SUCCESSFULLY!          \n";
    std::cout << "======================================================================\n";
    return 0;
}

