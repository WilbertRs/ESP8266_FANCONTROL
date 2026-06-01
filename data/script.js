const wsUrl = `ws://${window.location.hostname}:81/`;
let websocket;

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

const chartConfig = {
    type: 'line',
    options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: { duration: 0 },
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

const maxDataPoints = 30;

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

let isUserInteracting = false;

function updateDashboard(data) {
    tempVal.innerText = data.temp.toFixed(1);
    humVal.innerText = data.hum.toFixed(1);
    const fanPercent = Math.round((data.pwm / 255) * 100);
    fanVal.innerText = fanPercent;
    rpmVal.innerText = Math.round((data.pwm / 255) * 3000);

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

modeToggle.addEventListener('change', (e) => {
    isUserInteracting = true;
    const mode = e.target.checked ? 'MANUAL' : 'AUTO';
    
    if (mode === 'MANUAL') {
        pwmGroup.classList.add('active');
    } else {
        pwmGroup.classList.remove('active');
    }

    if (websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({ mode: mode }));
    }

    setTimeout(() => { isUserInteracting = false; }, 500);
});

pwmSlider.addEventListener('input', (e) => {
    pwmDisplay.innerText = e.target.value;
    isUserInteracting = true;
});

pwmSlider.addEventListener('change', (e) => {
    if (modeToggle.checked && websocket.readyState === WebSocket.OPEN) {
        websocket.send(JSON.stringify({
            mode: 'MANUAL',
            pwm: parseInt(e.target.value)
        }));
    }
    setTimeout(() => { isUserInteracting = false; }, 500);
});

window.onload = () => {
    initCharts();
    initWebSocket();
};
