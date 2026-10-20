# Courier Parcel Tracking & Dispatch System
**Data Structures & Algorithms (DSA) Mini-Project with Native Web UI**  
**Presented by:** Rohit Doshi  
**Backend:** C++17 (Vectors, Hash Index `unordered_map`, MergeSort, POSIX Sockets)  
**Frontend:** Modern Accessible Web Interface (HTML5, Responsive Tables, Container Queries, Dialogs)  
**Storage:** Persistent Pipe-Delimited File Handling (`parcels.txt`)

---

## 📌 1. Project Context & Objectives

Courier offices handle high volumes of parcels daily. Traditional manual registers suffer from:
1. **Scattered Records:** Manual entries are error-prone and hard to maintain.
2. **Slow Tracking:** Linear scanning of a long ledger causes search latency to scale linearly ($\mathcal{O}(N)$) with volume.
3. **Unclear Lifecycle Status:** Staff need instant visibility into which parcels are awaiting dispatch versus those in transit or delivered.

### Core Objectives
- **Parcel Registration:** Register parcels with unique tracking IDs, sender/recipient info, destination, and booking dates.
- **Fast Retrieval & Live Comparison:** Retrieve parcel details in $\mathcal{O}(1)$ average time using a hash index, comparing its performance against sequential search ($\mathcal{O}(N)$) with microsecond/nanosecond precision.
- **Status Lifecycle Tracking:** Update parcel stages (`Registered` $\to$ `Dispatched` $\to$ `In Transit` $\to$ `Delivered` / `Cancelled`) with automated dispatch dates.
- **Pending Dispatches:** Filter parcels awaiting dispatch (`status == "Registered"`).
- **Sorting:** Organize parcels by **Destination** (alphabetical delivery routes) or **Tracking ID** using stable **MergeSort** ($\mathcal{O}(N \log N)$).
- **Persistent Storage:** Automatically save and load all records to a local text file (`parcels.txt`) across program restarts.
- **Empirical Benchmarking:** Run large-scale search benchmarks ($N = 1,000$ to $50,000+$) proving the theoretical complexity differences between Sequential Search and Hash Lookup.
- **Modern Web Interface:** A web frontend directly powered by the C++ engine over HTTP without external frameworks.

---

## 🏗️ 2. Dual-Mode Architecture (CLI & Native Web Server)

```
+---------------------------------------------------------------------------------+
|                                 USER INTERFACES                                 |
|     1. Interactive CLI Console Menu   OR    2. Modern Responsive Web Browser     |
+------------------------+---------------------------------------+----------------+
                         |                                       |
                         | (Direct C++ Function Calls)           | (HTTP / REST API via POSIX Sockets)
                         v                                       v
+---------------------------------------------------------------------------------+
|                                C++ BACKEND CORE                                 |
|                       WebServer (POSIX Sockets / HTTP 1.1)                      |
|                       ParcelManager (Business Logic & Validation)               |
+----------------------------------------+----------------------------------------+
                                         |
                     +-------------------+-------------------+
                     |                                       |
                     v                                       v
+-----------------------------------------+ +-------------------------------------+
|             RECORD STORAGE              | |             HASH INDEX              |
|          std::vector<Parcel>            | |    std::unordered_map<ID, size_t>   |
|   Contiguous memory for all records     | |   Maps Tracking ID to Vector Index  |
+--------------------+--------------------+ +-------------------------------------+
                     |
                     v
+-----------------------------------------+
|           PERSISTENT STORAGE            |
|               parcels.txt               |
|      Pipe-delimited records format      |
+-----------------------------------------+
```

---

## 🌐 3. Web Interface Features

The web frontend located in `/web` provides an intuitive experience:
1. **Live Metrics Bar:** Instant count of Total Parcels, Pending Dispatches, In Transit, and Delivered items.
2. **Interactive Search & Algorithm Comparison:**
   - Real-time side-by-side comparison between **Sequential Search ($\mathcal{O}(N)$)** and **Hash-Based Lookup ($\mathcal{O}(1)$)**.
   - Displays number of checks/steps, elapsed execution time in nanoseconds/microseconds, and speedup ratio.
   - Visual **Lifecycle Stepper** showing stage progression (`Registered ➔ Dispatched ➔ In Transit ➔ Delivered`).
3. **Responsive Parcel Registry Table:**
   - Adheres to web standards: sticky headers (`position: sticky; inset-block-start: 0`), container query scaling, and keyboard accessibility (`role="region"`, `tabindex="0"`).
   - Instant sorting by Destination or Tracking ID using the C++ MergeSort engine.
   - Filtering by status (e.g. view only Pending Dispatches).
4. **Accessible Modal Dialogs:**
   - Registration and status update forms built with HTML5 `<dialog closedby="any">` and backdrop blur.
5. **Empirical Benchmark Runner Modal:**
   - Select dataset size ($N = 1,000$, $10,000$, or $50,000$) and run thousands of queries inside the C++ backend with live progress visualization.

---

## ⚡ 4. Algorithmic Complexity Analysis

| Operation | Algorithm / Mechanism | Typical Time | Worst-Case Time | Space Complexity | Implementation Detail |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **Sequential Search** | Linear Scan over `vector<Parcel>` | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ | Iterates indices 0 to $N-1$ until match. |
| **Hash-Based Lookup** | `std::unordered_map<ID, size_t>` | $\mathcal{O}(1)$ avg | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | Instant bucket computation; $\mathcal{O}(N)$ only on pathological collisions. |
| **Sorting** | Custom stable MergeSort | $\mathcal{O}(N \log N)$ | $\mathcal{O}(N \log N)$ | $\mathcal{O}(N)$ | Divide-and-conquer; rebuilds hash index after permutation. |
| **Pending Filter** | Vector scan (`status == Registered`) | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(K)$ | Inspects each record to extract pending queue. |
| **File Persistence** | Stream I/O (`parcels.txt`) | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ auxiliary | Pipe-delimited serialization & deserialization. |

---

## 🚀 5. How to Run

### Option 1: Start the Web Server (Recommended)
You can launch the web server directly using `make`, the C++ binary, or Python:

```bash
# Using Make (starts on port 5000)
make web

# Or using the C++ binary directly on any custom port
./bin/courier_tracker --web 5000

# Or using the Python launcher
python3 server.py 5000
```
Open your browser and navigate to: **`http://localhost:5000/`**

---

### Option 2: Run the Terminal CLI Menu
```bash
# Compile and run interactive CLI
make run
```
Menu option `9` also allows launching the web server directly from within the CLI!

---

### Option 3: Compile the Single-File Version
If your professor or examiner prefers a single `.cpp` file:
```bash
g++ -std=c++17 -O2 courier_tracker.cpp -o bin/courier_tracker

# Run CLI
./bin/courier_tracker

# Or run Web Server
./bin/courier_tracker --web 5000
```

---

### Run All Automated Verification Tests
```bash
make test
```
This runs both:
- `tests/test_demo.sh`: Validates the 6-step live demo walkthrough from Slide 11.
- `tests/test_web.sh`: Validates all HTTP endpoints, JSON API, and static asset delivery.

---

## 📡 6. REST API Endpoints

The native C++ web server exposes the following REST endpoints:

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/` | Serves the web dashboard (`web/index.html`) |
| `GET` | `/api/parcels` | Returns JSON array of all registered parcels |
| `GET` | `/api/pending` | Returns JSON array of parcels awaiting dispatch |
| `GET` | `/api/search?id=CR101` | Executes sequential & hash search, returns times, step counts, and parcel |
| `POST`| `/api/register` | Registers a new parcel, enforces unique ID, saves to `parcels.txt` |
| `POST`| `/api/update-status` | Updates delivery status and dispatch date, saves to `parcels.txt` |
| `GET` | `/api/sort?by=destination&order=asc` | Sorts records via C++ MergeSort and returns ordered records |
| `GET` | `/api/benchmark?n=10000&q=5000` | Generates $N$ records, executes $Q$ queries, returns speedup metrics |

---

## 📂 7. Project Structure

```
dsaminiproject/
├── include/
│   ├── Parcel.h              # Parcel entity definition & constants
│   ├── ParcelManager.h       # Manager class declarations & algorithms
│   └── WebServer.h           # POSIX socket HTTP server header
├── src/
│   ├── ParcelManager.cpp     # File I/O, hash indexing, MergeSort & benchmarking
│   ├── WebServer.cpp         # Native C++ HTTP server & REST API
│   └── main.cpp              # CLI menu & CLI/Web dispatcher
├── web/
│   ├── index.html            # Modern responsive web dashboard
│   ├── style.css             # Responsive table, dialogs, design tokens
│   └── app.js                # Frontend controller communicating with C++ API
├── courier_tracker.cpp       # Standalone all-in-one C++ file
├── server.py                 # Optional Python launcher
├── parcels.txt               # Persistent text database
├── Makefile                  # Build targets (all, run, web, single, test, clean)
├── tests/
│   ├── test_demo.sh          # Slide 11 walkthrough test suite
│   └── test_web.sh           # Web and API test suite
└── README.md                 # Project documentation
```

---
*Developed for College Data Structures & Algorithms Mini-Project.*
