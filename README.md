# Courier Parcel Tracking & Dispatch System
**Data Structures & Algorithms (DSA) Mini-Project**  
**Presented by:** Rohit Doshi  
**Language:** C++17 | **Data Structures:** Vectors, Hash Index (`unordered_map`), MergeSort, File I/O  

---

## 📌 1. Project Context & Objectives

Courier offices handle high volumes of parcels daily. Traditional register-based approaches suffer from:
1. **Scattered Records:** Manual entries make parcel information error-prone and hard to maintain.
2. **Slow Tracking:** Linear scanning of a long ledger causes search latency to scale linearly ($O(N)$) with volume.
3. **Unclear Lifecycle Status:** Staff need instant visibility into which parcels are awaiting dispatch versus those in transit or delivered.

### Core Objectives
- **Parcel Registration:** Register parcels with unique tracking IDs, sender/recipient info, destination, and booking dates.
- **Fast Retrieval:** Retrieve parcel details in $O(1)$ average time using a hash index, comparing its performance against sequential search ($O(N)$).
- **Status Lifecycle Tracking:** Update parcel stages (`Registered` $\to$ `Dispatched` $\to$ `In Transit` $\to$ `Delivered` / `Cancelled`) with automated dispatch dates.
- **Pending Dispatches:** Filter parcels awaiting dispatch (`status == "Registered"`).
- **Sorting:** Organize parcels by **Destination** (alphabetical delivery routes) or **Tracking ID** using stable **MergeSort** ($O(N \log N)$).
- **Persistent Storage:** Automatically save and load all records to a local text file (`parcels.txt`) across program restarts.
- **Empirical Benchmarking:** Run large-scale search benchmarks ($N = 1,000$ to $50,000+$) proving the theoretical complexity differences between Sequential Search and Hash Lookup.

---

## 🏗️ 2. Data Structures & Architecture

```
+-------------------------------------------------------------+
|                      USER INTERFACE                         |
|            Interactive Console Menu (CLI / C++)             |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                      PARCEL MANAGER                         |
|           Business Logic, Validation, MergeSort             |
+--------------------+-------------------+--------------------+
                     |                   |
                     v                   v
+-----------------------------+ +-----------------------------+
|       RECORD STORAGE        | |         HASH INDEX          |
|    std::vector<Parcel>      | |  std::unordered_map<ID,idx> |
|  Full in-memory records     | |  Maps ID to vector index    |
+--------------+--------------+ +-----------------------------+
               |
               v
+-----------------------------+
|     PERSISTENT STORAGE      |
|         parcels.txt         |
|  Pipe-delimited text format |
+-----------------------------+
```

### The Parcel Data Model
```cpp
struct Parcel {
    std::string trackingID;      // Unique parcel identifier (e.g. CR101)
    std::string sender;          // Sender name
    std::string recipient;       // Recipient name
    std::string destination;     // Delivery city (e.g. Mumbai)
    std::string status;          // "Registered", "Dispatched", "In Transit", "Delivered", "Cancelled"
    std::string registrationDate;// Booking date (YYYY-MM-DD)
    std::string dispatchDate;    // Dispatch date (YYYY-MM-DD or "-")
};
```

### File Delimited Format (`parcels.txt`)
```
trackingID | sender | recipient | destination | status | registrationDate | dispatchDate
```
*Example:*
```
CR101 | Amit Sharma | Neha Patel | Mumbai | Registered | 2026-10-16 | -
CR102 | Rajesh Kumar | Priya Singh | Delhi | Dispatched | 2026-10-15 | 2026-10-17
```

---

## ⚡ 3. Algorithm Analysis: Sequential Search vs Hash Lookup

| Operation | Algorithm / Mechanism | Typical Time | Worst-Case Time | Space Complexity | Why? |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **Sequential Search** | Linear Scan over `vector<Parcel>` | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ | Must inspect elements one-by-one from index 0 to $N-1$. |
| **Hash-Based Lookup** | `std::unordered_map<ID, size_t>` | $\mathcal{O}(1)$ avg | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | Computes hash bucket directly; worst-case occurs only on extreme hash collisions. |
| **Sorting** | Custom stable MergeSort | $\mathcal{O}(N \log N)$ | $\mathcal{O}(N \log N)$ | $\mathcal{O}(N)$ | Divide-and-conquer; rebuilds hash index after permutation. |
| **Pending Filter** | Vector scan (`status == Registered`) | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(K)$ | Visits each record once to filter pending parcels. |
| **File Persistence** | Stream I/O (`parcels.txt`) | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ auxiliary | Line-by-line serialization and deserialization. |

### Theoretical Comparison
1. **Sequential Search:** Checks up to $N$ records before locating the target or determining absence. Average checks: $N / 2$.
2. **Hash-Based Lookup:** Directly computes the hash of `trackingID` to find the vector index in constant time. In practice, hash lookup is **25x to 2,500x faster** as dataset size scales.

---

## 🚀 4. Compilation & Execution

### Prerequisites
- GCC / G++ supporting C++17
- Linux / macOS / Windows (MinGW or WSL)

### Option A: Using the Makefile (Modular Build)
```bash
# Build the project
make

# Run the interactive program
make run

# Run the automated demo test suite
make test

# Clean build artifacts
make clean
```

### Option B: Compiling the Standalone Single File
If your teacher or lab examiner prefers a single `.cpp` file:
```bash
# Compile
g++ -std=c++17 -O2 courier_tracker.cpp -o bin/courier_tracker

# Run
./bin/courier_tracker
```

---

## 📋 5. Live Demonstration Plan (Slide 11 Walkthrough)

Follow these steps for a 2–3 minute examiner walkthrough:

| Step | Action | Menu Choice | Details to Enter | Verification Check |
| :---: | :--- | :---: | :--- | :--- |
| **01** | **Register** | `1` | ID: `CR101`, Sender: `Amit Sharma`, Recipient: `Neha Patel`, Dest: `Mumbai` | Enforces unique ID; rejects duplicate if re-entered. |
| **02** | **Search** | `2` | ID: `CR101` | Displays side-by-side search comparison (Sequential vs Hash) + Parcel Card. |
| **03** | **Update** | `3` | ID: `CR101`, Status: `2` (Dispatched), Date: `2026-10-20` | Updates status and persists dispatch date. |
| **04** | **Filter** | `4` | View Pending Dispatches | Displays all parcels where `status == "Registered"`. |
| **05** | **Sort** | `5` | Criterion: `1` (Destination), Order: `1` (Ascending) | Records ordered alphabetically by destination city. |
| **06** | **Restart** | `9` then `./bin/courier_tracker` | Choose `6` (Display All) | Verifies all updates survived shutdown via `parcels.txt`. |

---

## 📊 6. Empirical Benchmark Results

Running the built-in benchmark runner (Option `7` in the menu) yields empirical proof:

```
========================================================================================
                   EMPIRICAL BENCHMARK RESULTS (N = 10,000, Queries = 5,000)             
========================================================================================
Algorithm                Total Time (ms)Avg Time/Query    Theoretical    Items Located  
----------------------------------------------------------------------------------------
Sequential Search        124.50         24.9000 us        O(N)           5000           
Hash-Based Lookup        0.22           0.0440 us         O(1) average   5000           
========================================================================================
 SPEEDUP FACTOR: Hash Lookup is 565.9x FASTER than Sequential Search on 10,000 records!
========================================================================================
```

---

## 📂 7. Project Structure

```
dsaminiproject/
├── include/
│   ├── Parcel.h              # Parcel entity definition & constants
│   └── ParcelManager.h       # ParcelManager declarations & API
├── src/
│   ├── ParcelManager.cpp     # File I/O, hash indexing, MergeSort & benchmarking
│   └── main.cpp              # Interactive menu-driven console UI
├── courier_tracker.cpp       # Standalone all-in-one source file
├── parcels.txt               # Persistent database file
├── Makefile                  # Automated build configurations
├── tests/
│   └── test_demo.sh          # Automated test suite for the 6-step walkthrough
└── README.md                 # Project documentation
```

---
*Developed for College Data Structures & Algorithms Mini-Project.*
