#include "../include/ParcelManager.h"
#include <iostream>
#include <string>
#include <limits>

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
            // Check for illegal pipe character
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
    std::cout << " 9. Exit\n";
    std::cout << "------------------------------------------------------------------------\n";
    std::cout << "Enter your choice (1-9): ";
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
    if (regDate.empty()) {
        regDate = defaultDate;
    }

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

int main() {
    ParcelManager manager("parcels.txt");
    printHeader();
    std::cout << " Loaded " << manager.count() << " parcel records from persistent storage (parcels.txt).\n";

    bool running = true;
    while (running) {
        printMenu();
        int choice = 0;
        if (!(std::cin >> choice)) {
            clearInputBuffer();
            std::cout << " [!] Please enter a valid number from 1 to 9.\n";
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
            case 9:
                manager.saveRecords();
                std::cout << "\nAll records saved to 'parcels.txt'. Exiting Courier Parcel Manager. Goodbye!\n\n";
                running = false;
                break;
            default:
                std::cout << " [!] Invalid option. Please choose between 1 and 9.\n";
                break;
        }
    }

    return 0;
}
