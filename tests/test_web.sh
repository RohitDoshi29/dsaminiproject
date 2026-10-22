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

echo "[3] Testing 404 Not Found response..."
STATUS_404=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:$PORT/nonexistent.html")
if [ "$STATUS_404" = "404" ]; then
    echo " -> [PASS] 404 returned for nonexistent file."
else
    echo " -> [FAIL] Expected 404 but got $STATUS_404."
    exit 1
fi

echo "[4] Testing OPTIONS CORS preflight..."
CORS_HEADERS=$(curl -s -I -X OPTIONS "http://localhost:$PORT/api/register")
if echo "$CORS_HEADERS" | grep -q "Access-Control-Allow-Origin"; then
    echo " -> [PASS] CORS headers returned for OPTIONS preflight."
else
    echo " -> [FAIL] CORS headers missing."
    exit 1
fi

echo "[5] Testing GET /api/parcels..."
PARCELS=$(curl -s "http://localhost:$PORT/api/parcels")
if echo "$PARCELS" | grep -q "CR101"; then
    echo " -> [PASS] /api/parcels returned valid parcel records."
else
    echo " -> [FAIL] /api/parcels failed."
    exit 1
fi

echo "[6] Testing GET /api/pending..."
PENDING=$(curl -s "http://localhost:$PORT/api/pending")
if echo "$PENDING" | grep -q "Registered"; then
    echo " -> [PASS] /api/pending returned pending records."
else
    echo " -> [FAIL] /api/pending failed."
    exit 1
fi

echo "[7] Testing GET /api/search?id=CR101 (Found case)..."
SEARCH_RES=$(curl -s "http://localhost:$PORT/api/search?id=CR101")
if echo "$SEARCH_RES" | grep -q "sequential" && echo "$SEARCH_RES" | grep -q '"found":true'; then
    echo " -> [PASS] /api/search returned dual-algorithm comparison for CR101."
else
    echo " -> [FAIL] /api/search failed: $SEARCH_RES"
    exit 1
fi

echo "[8] Testing GET /api/search?id=NONEXISTENT (Not found case)..."
SEARCH_NOTFOUND=$(curl -s "http://localhost:$PORT/api/search?id=NONEXISTENT")
if echo "$SEARCH_NOTFOUND" | grep -q '"found":false'; then
    echo " -> [PASS] /api/search handled non-existent ID gracefully."
else
    echo " -> [FAIL] /api/search non-existent test failed: $SEARCH_NOTFOUND"
    exit 1
fi

echo "[9] Testing POST /api/register (Success case)..."
REG_RES=$(curl -s -X POST -H "Content-Type: application/json" \
    -d '{"trackingID":"CR999","sender":"Dev Tester","recipient":"Quality Assurance","destination":"Pune","registrationDate":"2026-10-20"}' \
    "http://localhost:$PORT/api/register")
if echo "$REG_RES" | grep -q '"success":true'; then
    echo " -> [PASS] /api/register registered new parcel CR999."
else
    echo " -> [FAIL] /api/register failed: $REG_RES"
    exit 1
fi

echo "[10] Testing POST /api/register (Duplicate rejection)..."
REG_DUP=$(curl -s -X POST -H "Content-Type: application/json" \
    -d '{"trackingID":"CR999","sender":"Dev Tester","recipient":"Quality Assurance","destination":"Pune","registrationDate":"2026-10-20"}' \
    "http://localhost:$PORT/api/register")
if echo "$REG_DUP" | grep -q '"success":false'; then
    echo " -> [PASS] /api/register correctly rejected duplicate tracking ID."
else
    echo " -> [FAIL] Duplicate registration was not rejected: $REG_DUP"
    exit 1
fi

echo "[11] Testing POST /api/update-status (Success case)..."
UPD_RES=$(curl -s -X POST -H "Content-Type: application/json" \
    -d '{"trackingID":"CR999","status":"Dispatched","dispatchDate":"2026-10-20"}' \
    "http://localhost:$PORT/api/update-status")
if echo "$UPD_RES" | grep -q '"success":true'; then
    echo " -> [PASS] /api/update-status updated status to Dispatched."
else
    echo " -> [FAIL] /api/update-status failed: $UPD_RES"
    exit 1
fi

echo "[12] Testing POST /api/update-status (Non-existent parcel)..."
UPD_ERR=$(curl -s -X POST -H "Content-Type: application/json" \
    -d '{"trackingID":"UNKNOWN_ID","status":"Dispatched","dispatchDate":"2026-10-20"}' \
    "http://localhost:$PORT/api/update-status")
if echo "$UPD_ERR" | grep -q '"success":false'; then
    echo " -> [PASS] /api/update-status rejected unknown parcel update."
else
    echo " -> [FAIL] Unknown parcel update was not rejected: $UPD_ERR"
    exit 1
fi

echo "[13] Testing GET /api/sort?by=destination&order=asc & desc..."
SORT_ASC=$(curl -s "http://localhost:$PORT/api/sort?by=destination&order=asc")
SORT_DESC=$(curl -s "http://localhost:$PORT/api/sort?by=destination&order=desc")
if echo "$SORT_ASC" | grep -q '"success":true' && echo "$SORT_DESC" | grep -q '"success":true'; then
    echo " -> [PASS] /api/sort executed destination sorting (asc & desc)."
else
    echo " -> [FAIL] /api/sort failed."
    exit 1
fi

echo "[14] Testing GET /api/sort?by=trackingID&order=asc & desc..."
SORT_ID_ASC=$(curl -s "http://localhost:$PORT/api/sort?by=trackingID&order=asc")
SORT_ID_DESC=$(curl -s "http://localhost:$PORT/api/sort?by=trackingID&order=desc")
if echo "$SORT_ID_ASC" | grep -q '"success":true' && echo "$SORT_ID_DESC" | grep -q '"success":true'; then
    echo " -> [PASS] /api/sort executed tracking ID sorting (asc & desc)."
else
    echo " -> [FAIL] /api/sort failed."
    exit 1
fi

echo "[15] Testing GET /api/benchmark?n=1000&q=500..."
BENCH_RES=$(curl -s "http://localhost:$PORT/api/benchmark?n=1000&q=500")
if echo "$BENCH_RES" | grep -q "speedup" && echo "$BENCH_RES" | grep -q "seqDurationMs"; then
    echo " -> [PASS] /api/benchmark executed empirical benchmark successfully."
else
    echo " -> [FAIL] /api/benchmark failed: $BENCH_RES"
    exit 1
fi

# Clean up CR999 from parcels.txt
sed -i '/CR999/d' parcels.txt

echo "======================================================================"
echo "          ALL 15 WEB & API ENDPOINTS VERIFIED AND WORKING!            "
echo "======================================================================"
