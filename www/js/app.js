// ESP32-S3 Web Interface JavaScript
// Handles API communication and UI updates

// Configuration
const API_BASE = '/api';
const REFRESH_INTERVAL = 5000; // 5 seconds
let refreshTimer = null;

// API Endpoints
const API = {
    status: `${API_BASE}/status`,
    brightness: `${API_BASE}/brightness`,
    screen: `${API_BASE}/screen`,
    restart: `${API_BASE}/restart`,
    logs: `${API_BASE}/logs`,
    diagnostics: `${API_BASE}/diagnostics`
};

// Fetch system status
async function fetchStatus() {
    try {
        const response = await fetch(API.status);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const data = await response.json();
        updateStatus(data);
        return data;
    } catch (error) {
        console.error('Failed to fetch status:', error);
        showError('Nie można połączyć się z urządzeniem');
    }
}

// Update UI with status data
function updateStatus(data) {
    // Main page elements
    const elements = {
        uptime: document.getElementById('uptime'),
        memory: document.getElementById('memory'),
        wifi: document.getElementById('wifi'),
        temp: document.getElementById('temp')
    };

    if (elements.uptime && data.uptime) {
        elements.uptime.textContent = formatUptime(data.uptime);
    }
    
    if (elements.memory && data.freeHeap) {
        const usedKB = Math.round((320 * 1024 - data.freeHeap) / 1024);
        const freeKB = Math.round(data.freeHeap / 1024);
        elements.memory.textContent = `${freeKB} KB wolne / ${usedKB} KB użyte`;
    }
    
    if (elements.wifi && data.rssi) {
        const quality = getRSSIQuality(data.rssi);
        elements.wifi.textContent = `${data.rssi} dBm (${quality})`;
    }
    
    if (elements.temp && data.temperature) {
        elements.temp.textContent = `${data.temperature}°C`;
    }
}

// Format uptime from milliseconds
function formatUptime(ms) {
    const seconds = Math.floor(ms / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);
    
    if (days > 0) return `${days}d ${hours % 24}h ${minutes % 60}m`;
    if (hours > 0) return `${hours}h ${minutes % 60}m`;
    if (minutes > 0) return `${minutes}m ${seconds % 60}s`;
    return `${seconds}s`;
}

// Get WiFi signal quality
function getRSSIQuality(rssi) {
    if (rssi >= -50) return 'Doskonały';
    if (rssi >= -60) return 'Bardzo dobry';
    if (rssi >= -70) return 'Dobry';
    if (rssi >= -80) return 'Słaby';
    return 'Bardzo słaby';
}

// Brightness control
const brightnessSlider = document.getElementById('brightness');
const brightnessValue = document.getElementById('brightness-value');

if (brightnessSlider && brightnessValue) {
    brightnessSlider.addEventListener('input', (e) => {
        brightnessValue.textContent = `${e.target.value}%`;
    });

    brightnessSlider.addEventListener('change', async (e) => {
        const value = e.target.value;
        try {
            const response = await fetch(API.brightness, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ brightness: parseInt(value) })
            });
            
            if (response.ok) {
                console.log(`Brightness set to ${value}%`);
            } else {
                showError('Nie udało się ustawić jasności');
            }
        } catch (error) {
            console.error('Brightness control error:', error);
            showError('Błąd komunikacji z urządzeniem');
        }
    });
}

// Screen control functions
async function screenOn() {
    return controlScreen('on');
}

async function screenOff() {
    return controlScreen('off');
}

async function controlScreen(action) {
    try {
        const response = await fetch(API.screen, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action })
        });
        
        if (response.ok) {
            console.log(`Screen ${action}`);
        } else {
            showError(`Nie udało się ${action === 'on' ? 'włączyć' : 'wyłączyć'} ekranu`);
        }
    } catch (error) {
        console.error('Screen control error:', error);
        showError('Błąd komunikacji z urządzeniem');
    }
}

// Restart device
async function restartDevice() {
    if (!confirm('Czy na pewno chcesz zrestartować urządzenie?')) {
        return;
    }
    
    try {
        const response = await fetch(API.restart, { method: 'POST' });
        if (response.ok) {
            alert('Urządzenie zostanie zrestartowane. Proszę czekać...');
            // Wait 10 seconds then reload page
            setTimeout(() => location.reload(), 10000);
        }
    } catch (error) {
        console.error('Restart error:', error);
        showError('Nie udało się zrestartować urządzenia');
    }
}

// Clear logs
async function clearLogs() {
    try {
        const response = await fetch(API.logs, { method: 'DELETE' });
        if (response.ok) {
            console.log('Logs cleared');
            if (typeof refreshLogs === 'function') {
                refreshLogs();
            }
        }
    } catch (error) {
        console.error('Clear logs error:', error);
    }
}

// Refresh status manually
async function refreshStatus() {
    await fetchStatus();
}

// Show error message
function showError(message) {
    // Simple alert for now - could be enhanced with toast notifications
    console.error(message);
}

// Diagnostics page functions
async function refreshDiagnostics() {
    try {
        const response = await fetch(API.diagnostics);
        if (!response.ok) return;
        
        const data = await response.json();
        updateDiagnostics(data);
    } catch (error) {
        console.error('Failed to fetch diagnostics:', error);
    }
}

function updateDiagnostics(data) {
    // Update diagnostic page elements
    const fields = [
        'chip-id', 'ram-free', 'ssid', 'ip', 'mac', 
        'rssi', 'requests', 'server-uptime', 
        'sd-total', 'sd-used', 'sd-free'
    ];
    
    fields.forEach(field => {
        const element = document.getElementById(field);
        if (element && data[field] !== undefined) {
            element.textContent = data[field];
        }
    });
}

async function refreshLogs() {
    try {
        const response = await fetch(API.logs);
        if (!response.ok) return;
        
        const logs = await response.json();
        const container = document.getElementById('logs');
        
        if (container && logs.entries) {
            container.innerHTML = logs.entries.map(entry => 
                `<div class="log-entry">
                    <span class="log-time">[${entry.time}]</span> ${entry.message}
                </div>`
            ).join('');
        }
    } catch (error) {
        console.error('Failed to fetch logs:', error);
    }
}

function downloadLogs() {
    window.location.href = `${API.logs}?download=true`;
}

function exportDiagnostics() {
    window.location.href = `${API.diagnostics}?export=true`;
}

// Test functions for diagnostics
async function testDisplay() {
    alert('Test wyświetlacza - funkcja w rozwoju');
}

async function testTouch() {
    alert('Test dotyku - funkcja w rozwoju');
}

async function testSD() {
    alert('Test karty SD - funkcja w rozwoju');
}

async function pingTest() {
    const start = Date.now();
    try {
        await fetch(API.status);
        const latency = Date.now() - start;
        alert(`Ping: ${latency}ms`);
    } catch (error) {
        alert('Test połączenia nieudany');
    }
}

// Auto-refresh functionality
function startAutoRefresh() {
    if (refreshTimer) return;
    
    // Initial fetch
    fetchStatus();
    
    // Set up interval
    refreshTimer = setInterval(fetchStatus, REFRESH_INTERVAL);
    console.log('Auto-refresh started');
}

function stopAutoRefresh() {
    if (refreshTimer) {
        clearInterval(refreshTimer);
        refreshTimer = null;
        console.log('Auto-refresh stopped');
    }
}

// Initialize on page load
window.addEventListener('load', () => {
    console.log('ESP32-S3 Web Interface loaded');
    
    // Start auto-refresh on main page
    if (document.getElementById('uptime')) {
        startAutoRefresh();
    }
});

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    stopAutoRefresh();
});
