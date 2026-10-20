// Courier Parcel Tracking & Dispatch System - Frontend Controller
let currentParcels = [];
let sortDirections = {
  destination: 'asc',
  trackingID: 'asc'
};
let activeSearchedParcelId = null;

document.addEventListener('DOMContentLoaded', () => {
  setupDialogBackdropFallbacks();
  setupSearchForm();
  setDefaultDates();
  loadParcels();
});

// Modern Web Guidance: Light-dismiss fallback for dialogs without native closedBy support
function setupDialogBackdropFallbacks() {
  const dialogs = document.querySelectorAll('dialog');
  dialogs.forEach(dialog => {
    if (!('closedBy' in HTMLDialogElement.prototype)) {
      dialog.addEventListener('click', (event) => {
        if (event.target !== dialog) return;
        const rect = dialog.getBoundingClientRect();
        const isDialogContent = (
          rect.top <= event.clientY &&
          event.clientY <= rect.top + rect.height &&
          rect.left <= event.clientX &&
          event.clientX <= rect.left + rect.width
        );
        if (!isDialogContent) {
          dialog.close();
        }
      });
    }
  });
}

function setDefaultDates() {
  const today = new Date().toISOString().split('T')[0];
  const regDateInput = document.getElementById('regDate');
  const dispDateInput = document.getElementById('updateDispatchDate');
  if (regDateInput) regDateInput.value = today;
  if (dispDateInput) dispDateInput.value = today;
}

function setupSearchForm() {
  const form = document.getElementById('searchForm');
  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const id = document.getElementById('searchInput').value.trim();
    if (id) {
      performSearch(id);
    }
  });
}

function quickSearch(id) {
  document.getElementById('searchInput').value = id;
  performSearch(id);
}

// Fetch all parcels from C++ Backend API
async function loadParcels() {
  try {
    const res = await fetch('/api/parcels');
    if (!res.ok) throw new Error('API request failed');
    currentParcels = await res.json();
    updateMetrics(currentParcels);
    renderTable(currentParcels);
    document.getElementById('backendStatus').textContent = 'C++ Engine Active';
    document.getElementById('recordCountBadge').textContent = `${currentParcels.length} Parcels`;
  } catch (err) {
    console.error('Failed to load parcels:', err);
    document.getElementById('backendStatus').textContent = 'Offline / Error';
  }
}

// Update Top Dashboard Metrics
function updateMetrics(parcels) {
  const total = parcels.length;
  const pending = parcels.filter(p => p.status === 'Registered').length;
  const transit = parcels.filter(p => p.status === 'In Transit' || p.status === 'Dispatched').length;
  const delivered = parcels.filter(p => p.status === 'Delivered').length;

  document.getElementById('statTotal').textContent = total;
  document.getElementById('statPending').textContent = pending;
  document.getElementById('statTransit').textContent = transit;
  document.getElementById('statDelivered').textContent = delivered;
}

// Render Table Rows
function renderTable(parcels) {
  const tbody = document.getElementById('parcelsTableBody');
  tbody.innerHTML = '';

  if (parcels.length === 0) {
    tbody.innerHTML = `<tr><td colspan="8" style="text-align:center; padding: 2rem; color: var(--text-dim);">No parcel records found matching filter.</td></tr>`;
    return;
  }

  parcels.forEach(p => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td><strong style="font-family: var(--font-mono); color: var(--primary);">${escapeHtml(p.trackingID)}</strong></td>
      <td>${escapeHtml(p.sender)}</td>
      <td>${escapeHtml(p.recipient)}</td>
      <td>${escapeHtml(p.destination)}</td>
      <td><span class="badge badge-${escapeHtml(p.status.replace(/\s+/g, '-'))}">${escapeHtml(p.status)}</span></td>
      <td>${escapeHtml(p.registrationDate)}</td>
      <td>${escapeHtml(p.dispatchDate)}</td>
      <td style="text-align: right;">
        <button type="button" class="btn btn-outline-sm" onclick="openUpdateModal('${escapeHtml(p.trackingID)}')">Update</button>
      </td>
    `;
    tbody.appendChild(tr);
  });
}

// Filter Table by Status
function filterTable() {
  const filter = document.getElementById('statusFilter').value;
  if (filter === 'ALL') {
    renderTable(currentParcels);
  } else {
    const filtered = currentParcels.filter(p => p.status === filter);
    renderTable(filtered);
  }
}

// Sort Parcels using C++ MergeSort API
async function sortParcels(criteria) {
  const currentOrder = sortDirections[criteria];
  const nextOrder = currentOrder === 'asc' ? 'desc' : 'asc';
  sortDirections[criteria] = nextOrder;

  const destIcon = document.getElementById('sortDestIcon');
  const idIcon = document.getElementById('sortIdIcon');

  if (criteria === 'destination') {
    destIcon.textContent = nextOrder === 'asc' ? '↑' : '↓';
    idIcon.textContent = '⇅';
  } else {
    idIcon.textContent = nextOrder === 'asc' ? '↑' : '↓';
    destIcon.textContent = '⇅';
  }

  try {
    const res = await fetch(`/api/sort?by=${criteria}&order=${nextOrder}`);
    const data = await res.json();
    if (data.success) {
      currentParcels = data.parcels;
      filterTable();
    }
  } catch (err) {
    console.error('Sort failed:', err);
  }
}

// Execute Search & Algorithm Comparison
async function performSearch(trackingID) {
  try {
    const res = await fetch(`/api/search?id=${encodeURIComponent(trackingID)}`);
    const data = await res.json();

    const container = document.getElementById('searchResultContainer');
    container.classList.remove('hidden');

    document.getElementById('resTrackingId').textContent = trackingID;
    document.getElementById('resSeqSteps').textContent = `${data.sequential.steps} comparisons`;
    document.getElementById('resSeqTime').textContent = formatTime(data.sequential.timeNs);

    document.getElementById('resHashSteps').textContent = `${data.hash.steps} bucket step`;
    document.getElementById('resHashTime').textContent = formatTime(data.hash.timeNs);

    const speedup = data.speedup || 1.0;
    document.getElementById('speedupVal').textContent = `${speedup.toFixed(1)}x`;

    const parcelCard = document.getElementById('parcelCard');
    if (data.found && data.parcel) {
      parcelCard.classList.remove('hidden');
      renderParcelCard(data.parcel);
      activeSearchedParcelId = data.parcel.trackingID;
    } else {
      parcelCard.classList.add('hidden');
      alert(`Parcel with Tracking ID '${trackingID}' was not found in the registry.`);
    }

    container.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  } catch (err) {
    console.error('Search error:', err);
  }
}

function formatTime(ns) {
  if (ns < 1000) return `${ns.toFixed(0)} ns`;
  if (ns < 1000000) return `${(ns / 1000).toFixed(2)} µs`;
  return `${(ns / 1000000).toFixed(2)} ms`;
}

// Render Parcel Card & Update Lifecycle Stepper
function renderParcelCard(p) {
  document.getElementById('cardTrackingId').textContent = p.trackingID;
  const statusBadge = document.getElementById('cardStatusBadge');
  statusBadge.textContent = p.status;
  statusBadge.className = `badge status-badge badge-${p.status.replace(/\s+/g, '-')}`;

  document.getElementById('cardSender').textContent = p.sender;
  document.getElementById('cardRecipient').textContent = p.recipient;
  document.getElementById('cardDestination').textContent = p.destination;
  document.getElementById('cardRegDate').textContent = p.registrationDate;
  document.getElementById('cardDispDate').textContent = p.dispatchDate;

  // Update Stepper
  const steps = ['Registered', 'Dispatched', 'In Transit', 'Delivered'];
  const currentIdx = steps.indexOf(p.status);

  const stepElements = document.querySelectorAll('.stepper .step');
  const lineElements = document.querySelectorAll('.stepper .line');

  stepElements.forEach((el, idx) => {
    el.classList.remove('active', 'completed');
    if (currentIdx !== -1) {
      if (idx < currentIdx) {
        el.classList.add('completed');
      } else if (idx === currentIdx) {
        el.classList.add('active');
      }
    }
  });

  lineElements.forEach((line, idx) => {
    line.classList.remove('active');
    if (currentIdx > idx) {
      line.classList.add('active');
    }
  });
}

function openUpdateModalForCurrent() {
  if (activeSearchedParcelId) {
    openUpdateModal(activeSearchedParcelId);
  }
}

// Modal Handlers
function openRegisterModal() {
  document.getElementById('registerForm').reset();
  setDefaultDates();
  document.getElementById('regError').classList.add('hidden');
  const modal = document.getElementById('registerModal');
  modal.showModal();
}

async function handleRegisterSubmit(event) {
  event.preventDefault();
  const id = document.getElementById('regTrackingId').value.trim();
  const sender = document.getElementById('regSender').value.trim();
  const recipient = document.getElementById('regRecipient').value.trim();
  const destination = document.getElementById('regDestination').value.trim();
  const regDate = document.getElementById('regDate').value || new Date().toISOString().split('T')[0];

  const errEl = document.getElementById('regError');
  errEl.classList.add('hidden');

  try {
    const res = await fetch('/api/register', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        trackingID: id,
        sender,
        recipient,
        destination,
        registrationDate: regDate
      })
    });

    const data = await res.json();
    if (data.success) {
      closeModal('registerModal');
      await loadParcels();
      performSearch(id);
    } else {
      errEl.textContent = data.error || 'Registration failed.';
      errEl.classList.remove('hidden');
    }
  } catch (err) {
    errEl.textContent = 'Server communication error.';
    errEl.classList.remove('hidden');
  }
}

function openUpdateModal(trackingID) {
  const parcel = currentParcels.find(p => p.trackingID === trackingID);
  if (!parcel) return;

  document.getElementById('updateTrackingId').value = parcel.trackingID;
  document.getElementById('updateDisplayId').textContent = parcel.trackingID;
  document.getElementById('updateDisplayDest').textContent = parcel.destination;
  document.getElementById('updateStatusSelect').value = parcel.status;

  const today = new Date().toISOString().split('T')[0];
  document.getElementById('updateDispatchDate').value = (parcel.dispatchDate && parcel.dispatchDate !== '-') ? parcel.dispatchDate : today;
  document.getElementById('updateError').classList.add('hidden');

  const modal = document.getElementById('updateModal');
  modal.showModal();
}

function handleStatusChangeInModal() {
  const status = document.getElementById('updateStatusSelect').value;
  const dispDateInput = document.getElementById('updateDispatchDate');
  if (status === 'Dispatched' || status === 'In Transit' || status === 'Delivered') {
    if (!dispDateInput.value) {
      dispDateInput.value = new Date().toISOString().split('T')[0];
    }
  }
}

async function handleUpdateSubmit(event) {
  event.preventDefault();
  const id = document.getElementById('updateTrackingId').value;
  const status = document.getElementById('updateStatusSelect').value;
  const dispatchDate = document.getElementById('updateDispatchDate').value;

  const errEl = document.getElementById('updateError');
  errEl.classList.add('hidden');

  try {
    const res = await fetch('/api/update-status', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        trackingID: id,
        status,
        dispatchDate: dispatchDate || '-'
      })
    });

    const data = await res.json();
    if (data.success) {
      closeModal('updateModal');
      await loadParcels();
      if (activeSearchedParcelId === id) {
        performSearch(id);
      }
    } else {
      errEl.textContent = data.error || 'Failed to update status.';
      errEl.classList.remove('hidden');
    }
  } catch (err) {
    errEl.textContent = 'Server communication error.';
    errEl.classList.remove('hidden');
  }
}

function openBenchmarkModal() {
  document.getElementById('benchResults').classList.add('hidden');
  document.getElementById('benchLoading').classList.add('hidden');
  const modal = document.getElementById('benchmarkModal');
  modal.showModal();
}

async function executeBenchmark() {
  const n = document.getElementById('benchSizeSelect').value;
  const q = (n == 50000) ? 10000 : 5000;

  const loading = document.getElementById('benchLoading');
  const resultsBox = document.getElementById('benchResults');
  const runBtn = document.getElementById('runBenchBtn');

  loading.classList.remove('hidden');
  resultsBox.classList.add('hidden');
  runBtn.disabled = true;

  try {
    const res = await fetch(`/api/benchmark?n=${n}&q=${q}`);
    const data = await res.json();

    document.getElementById('benchSeqTotal').textContent = `${data.seqDurationMs.toFixed(2)} ms`;
    document.getElementById('benchSeqAvg').textContent = `${data.seqAvgUs.toFixed(3)} µs/query`;

    document.getElementById('benchHashTotal').textContent = `${data.hashDurationMs.toFixed(2)} ms`;
    document.getElementById('benchHashAvg').textContent = `${data.hashAvgUs.toFixed(3)} µs/query`;

    document.getElementById('benchSpeedupFactor').textContent = data.speedup.toFixed(1);

    resultsBox.classList.remove('hidden');
  } catch (err) {
    console.error('Benchmark failed:', err);
  } finally {
    loading.classList.add('hidden');
    runBtn.disabled = false;
  }
}

function closeModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) modal.close();
}

function escapeHtml(str) {
  if (!str) return '';
  return str.toString()
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

