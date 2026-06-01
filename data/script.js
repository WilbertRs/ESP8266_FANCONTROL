// Alamat IP WebSocket otomatis menyesuaikan dengan IP pengakses (ESP8266)
const wsUrl = `ws://${window.location.hostname}:81/`;
let websocket;

// Elemen-Elemen DOM
const tempVal = document.getElementById('temp-val');
const humVal = document.getElementById('hum-val');
const fanVal = document.getElementById('fan-val');
const rpmVal = document.getElementById('rpm-val');
const wsStatus = document.getElementById('ws-status');
const wsText = document.getElementById('ws-text');
const modeToggle = document.getElementById('mode-toggle');
const pwmGroup = document.getElementById('pwm-group');
const pwmSlider = document.getElementById('pwm-slider');
const pwmDisplay = document.getElementById('pwm-display');

// Konfigurasi Umum Chart.js
const chartConfig = {
    type: 'line',
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: { duration: 0 }, // Disable animasi default agar scrolling lebih smooth
        scales: {
            x: { display: false },
            y: {
                grid: { color: '#30363d' },
                ticks: { color: '#8b949e' }
            }
        },
        plugins: {
            legend: { display: false }
        },
        elements: {
            point: { radius: 0 },
            line: { borderWidth: 2, tension: 0.3 }
        }
    }
};

const maxDataPoints = 30; // Jumlah titik maksimum pada grafik agar efek bergeser stabil

// Inisialisasi Grafik (dengan proteksi jika library gagal dimuat)
let tempChart, humChart;

function initCharts() {
    if (typeof Chart === 'undefined') {
        console.warn("Chart.js belum dimuat. Grafik tidak akan ditampilkan.");
        return;
    }

    const ctxTemp = document.getElementById('tempChart').getContext('2d');
    tempChart = new Chart(ctxTemp, {
        ...chartConfig,
        data: {
            labels: Array(maxDataPoints).fill(''),
            datasets: [{
                data: Array(maxDataPoints).fill(null),
                borderColor: '#f85149',
                backgroundColor: 'rgba(248, 81, 73, 0.1)',
                fill: true
            }]
        }
    });

    const ctxHum = document.getElementById('humChart').getContext('2d');
    humChart = new Chart(ctxHum, {
        ...chartConfig,
        data: {
            labels: Array(maxDataPoints).fill(''),
            datasets: [{
                data: Array(maxDataPoints).fill(null),
                borderColor: '#58a6ff',
                backgroundColor: 'rgba(88, 166, 255, 0.1)',
                fill: true
            }]
        }
    });
}

// Koneksi WebSocket dengan Reconnect Otomatis
function initWebSocket() {
    websocket = new WebSocket(wsUrl);
    
    websocket.onopen = () => {
        wsStatus.className = 'dot connected';
        wsText.innerText = 'CONNECTED';
        wsText.style.color = 'var(--success)';
    };

    websocket.onclose = () => {
        wsStatus.className = 'dot disconnected';
        wsText.innerText = 'DISCONNECTED';
        wsText.style.color = 'var(--danger)';
        // Lakukan reconnect otomatis setiap 2 detik jika putus
        setTimeout(initWebSocket, 2000); 
    };

    websocket.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            updateDashboard(data);
        } catch (e) {
            console.error("Gagal mem-parsing data JSON", e);
        }
    };
}

// Update UI Dashboard Berdasarkan Data WebSocket
let isUserInteracting = false;

function updateDashboard(data) {
    // 1. Perbarui Angka Teks
    tempVal.innerText = data.temp.toFixed(1);
    humVal.innerText = data.hum.toFixed(1);
    
    // 2. Hitung persentase Fan & Estimasi RPM (Asumsi kipas max 3000 RPM)
    const fanPercent = Math.round((data.pwm / 255) * 100);
    fanVal.innerText = fanPercent;
    rpmVal.innerText = Math.round((data.pwm / 255) * 3000);

    // 3. Perbarui Status Panel Kontrol (Mencegah loncat nilai ketika user menggeser slider)
    if (!isUserInteracting) {
        if (data.mode === 'MANUAL') {
            modeToggle.checked = true;
            pwmGroup.classList.add('active');
        } else {
            modeToggle.checked = false;
            pwmGroup.classList.remove('active');
        }
        pwmSlider.value = data.pwm;
        pwmDisplay.innerText = data.pwm;
    }

    // 4. Update Grafik (Hanya jika chart berhasil diinisialisasi)
    if (tempChart) {
        const tempDataset = tempChart.data.datasets[0].data;
        tempDataset.push(data.temp);
        tempDataset.shift();
        tempChart.update();
    }

    if (humChart) {
        const humDataset = humChart.data.datasets[0].data;
        humDataset.push(data.hum);
        humDataset.shift();
        humChart.update();
    }
}

// --- Event Listeners untuk Panel Kontrol ---

// Logika Toggle Mode Auto/Manual
modeToggle.addEventListener('change', (e) => {
    isUserInteracting = true;
    const mode = e.target.checked ? 'MANUAL' : 'AUTO';
    
    // Mengaktifkan atau menonaktifkan slider UI
    if (mode === 'MANUAL') {
        pwmGroup.classList.add('active');
    } else {
        pwmGroup.classList.remove('active');
    }

    // Kirim perintah ke ESP8266
    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({ mode: mode }));
    }
    
    // Matikan flag interaksi setelah jeda
    setTimeout(() => { isUserInteracting = false; }, 500);
});

// Update Teks Label secara Real-time saat Slider ditarik (Sebelum mouse dilepas)
pwmSlider.addEventListener('input', (e) => {
    pwmDisplay.innerText = e.target.value;
    isUserInteracting = true;
});

// Kirim data PWM saat Slider selesai digeser (Mouse dilepas)
pwmSlider.addEventListener('change', (e) => {
    if (modeToggle.checked && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({
            mode: 'MANUAL',
            pwm: parseInt(e.target.value)
        }));
    }
    setTimeout(() => { isUserInteracting = false; }, 500);
});

// Inisialisasi semuanya ketika jendela browser selesai dimuat
window.onload = () => {
    initCharts();
    initWebSocket();
};
