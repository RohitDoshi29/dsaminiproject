#!/usr/bin/env bash
set -e

WORKDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$WORKDIR"

PORT=5055

echo "======================================================================"
echo "          COURIER PARCEL WEB SERVER & API TEST SUITE                 "
echo "======================================================================"

# Start server in background
./bin/courier_tracker --web $PORT > /dev/null 2>&1 &
SERVER_PID=$!

cleanup() {
    if kill -0 $SERVER_PID 2>/dev/null; then
        kill $SERVER_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

# Wait up to 3 seconds for port to open
for i in {1..15}; do
    if curl -s "http://localhost:$PORT/" > /dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

echo "[1] Testing Static HTML delivery..."
HTML=$(curl -s "http://localhost:$PORT/")
if echo "$HTML" | grep -q "Courier Parcel Tracking & Dispatch"; then
    echo " -> [PASS] index.html served correctly."
else
    echo " -> [FAIL] index.html not served properly."
    exit 1
fi

echo "[2] Testing Static CSS & JS delivery..."
CSS=$(curl -s "http://localhost:$PORT/style.css")
if echo "$CSS" | grep -q -F "border-subtle"; then
    echo " -> [PASS] style.css served correctly."
else
    echo " -> [FAIL] style.css not served."
    exit 1
fi

JS=$(curl -s "http://localhost:$PORT/app.js")
if echo "$JS" | grep -q "performSearch"; then
    echo " -> [PASS] app.js served correctly."
else
    echo " -> [FAIL] app.js not served."
    exit 1
fi

echo "[3] Testing GET /api/parcels..."
PARCELS=$(curl -s "http://localhost:$PORT/api/parcels")
if echo "$PARCELS" | grep -q "CR101"; then
    echo " -> [PASS] /api/parcels returned valid parcel records."
else
    echo " -> [FAIL] /api/parcels failed."
    exit 1
fi

echo "[4] Testing GET /api/search?id=CR101 (Algorithm comparison)..."
SEARCH_RES=$(curl -s "http://localhost:$PORT/api/search?id=CR101")
if echo "$SEARCH_RES" | grep -q "sequential" && echo "$SEARCH_RES" | grep -q "hash"; then
    echo " -> [PASS] /api/search returned dual-algorithm comparison metrics."
else
    echo " -> [FAIL] /api/search failed."
    exit 1
fi

echo "[5] Testing POST /api/register..."
REG_RES=$(curl -s -X POST -H "Content-Type: application/json" \
    -d '{"trackingID":"CR999","sender":"Dev Tester","recipient":"Quality Assurance","destination":"Pune","registrationDate":"2026-10-20"}' \
    "http://localhost:$PORT/api/register")
if echo "$REG_RES" | grep -q '"success":true'; then
    echo " -> [PASS] /api/register registered new parcel CR999."
else
    echo " -> [FAIL] /api/register failed: $REG_RES"
    exit 1
fi

echo "[6] Testing POST /api/update-status..."
UPD_RES=$(curl -s -X POST -H "Content-Type: application/json" \
    -d '{"trackingID":"CR999","status":"Dispatched","dispatchDate":"2026-10-20"}' \
    "http://localhost:$PORT/api/update-status")
if echo "$UPD_RES" | grep -q '"success":true'; then
    echo " -> [PASS] /api/update-status updated status to Dispatched."
else
    echo " -> [FAIL] /api/update-status failed: $UPD_RES"
    exit 1
fi

echo "[7] Testing GET /api/sort?by=destination&order=asc..."
SORT_RES=$(curl -s "http://localhost:$PORT/api/sort?by=destination&order=asc")
if echo "$SORT_RES" | grep -q '"success":true'; then
    echo " -> [PASS] /api/sort sorted records by destination."
else
    echo " -> [FAIL] /api/sort failed."
    exit 1
fi

echo "[8] Testing GET /api/benchmark?n=1000&q=500..."
BENCH_RES=$(curl -s "http://localhost:$PORT/api/benchmark?n=1000&q=500")
if echo "$BENCH_RES" | grep -q "speedup"; then
    echo " -> [PASS] /api/benchmark executed empirical benchmark successfully."
else
    echo " -> [FAIL] /api/benchmark failed."
    exit 1
fi

# Clean up CR999 from parcels.txt
sed -i '/CR999/d' parcels.txt

echo "======================================================================"
echo "          ALL WEB & API TESTS PASSED SUCCESSFULLY!                    "
echo "======================================================================"
