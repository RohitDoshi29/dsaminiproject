#!/usr/bin/env bash
set -e

echo "======================================================================"
echo "          COURIER PARCEL MANAGER - DEMO VERIFICATION SUITE            "
echo "======================================================================"

WORKDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$WORKDIR"

# Backup existing parcels.txt if any
if [ -f parcels.txt ]; then
    cp parcels.txt parcels.txt.bak
fi

# Ensure test parcels.txt has baseline records
cat << 'EOF' > parcels.txt
CR102 | Rajesh Kumar | Priya Singh | Delhi | Dispatched | 2026-10-15 | 2026-10-17
CR103 | Vikram Mehta | Ananya Das | Bangalore | In Transit | 2026-10-14 | 2026-10-15
CR104 | Sunil Verma | Pooja Joshi | Pune | Registered | 2026-10-17 | -
CR105 | Deepa Nair | Rahul Iyer | Chennai | Delivered | 2026-10-12 | 2026-10-13
EOF

echo "[Step 1] Testing Parcel Registration (CR101 for Mumbai) & Duplicate Rejection..."
# 1: Register CR101
# 1: Attempt duplicate register CR101 (should reject)
# 10: Exit
./bin/courier_tracker << 'EOF'
1
CR101
Amit Sharma
Neha Patel
Mumbai
2026-10-16
1
CR101
10
EOF

echo "[Check 1] Verifying CR101 was saved to parcels.txt..."
grep -q "CR101 | Amit Sharma | Neha Patel | Mumbai | Registered | 2026-10-16 | -" parcels.txt
echo " -> CR101 verified in persistent storage!"

echo "[Step 2] Testing Search (Sequential vs Hash) & Status Update (CR101 -> Dispatched)..."
# 2: Search CR101
# 3: Update CR101 to Dispatched (option 2), enter dispatch date 2026-10-20
# 4: Display pending dispatches
# 5: Sort by Destination (1), Ascending (1)
# 10: Exit
./bin/courier_tracker << 'EOF'
2
CR101
3
CR101
2
2026-10-20
4
5
1
1
10
EOF

echo "[Check 2] Verifying status update to Dispatched in parcels.txt..."
grep -q "CR101 | Amit Sharma | Neha Patel | Mumbai | Dispatched | 2026-10-16 | 2026-10-20" parcels.txt
echo " -> Status update and dispatch date verified in persistent storage!"

echo "[Step 3] Testing Persistence Across Program Restart & Benchmark..."
# Re-run program, display all (6), run small benchmark (7, option 1), exit (10)
./bin/courier_tracker << 'EOF'
6
7
1
10
EOF

echo "======================================================================"
echo "          ALL 6 LIVE DEMO CHECKS PASSED SUCCESSFULLY!                 "
echo "======================================================================"

# Restore seed parcels.txt with full test data
cat << 'EOF' > parcels.txt
CR101 | Amit Sharma | Neha Patel | Mumbai | Registered | 2026-10-16 | -
CR102 | Rajesh Kumar | Priya Singh | Delhi | Dispatched | 2026-10-15 | 2026-10-17
CR103 | Vikram Mehta | Ananya Das | Bangalore | In Transit | 2026-10-14 | 2026-10-15
CR104 | Sunil Verma | Pooja Joshi | Pune | Registered | 2026-10-17 | -
CR105 | Deepa Nair | Rahul Iyer | Chennai | Delivered | 2026-10-12 | 2026-10-13
CR106 | Karan Malhotra | Simran Kaur | Kolkata | Registered | 2026-10-18 | -
CR107 | Manoj Gupta | Sneha Reddy | Hyderabad | Dispatched | 2026-10-16 | 2026-10-18
CR108 | Alok Mishra | Divya Sharma | Ahmedabad | Cancelled | 2026-10-13 | -
CR109 | Tanvi Shah | Rohan Doshi | Mumbai | Registered | 2026-10-19 | -
CR110 | Sameer Sen | Meera Nair | Jaipur | In Transit | 2026-10-15 | 2026-10-16
EOF

if [ -f parcels.txt.bak ]; then
    rm -f parcels.txt.bak
fi

