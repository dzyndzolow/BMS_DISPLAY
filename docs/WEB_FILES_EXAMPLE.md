# Pliki Webowe - Przykładowa Struktura

## Lokalizacja na karcie SD

Wszystkie pliki webowe powinny być umieszczone w katalogu:
```
SD:/www/
```

## Struktura katalogów

```
SD:/www/
├── index.html          ← Strona główna
├── css/
│   └── style.css       ← Arkusze stylów
├── js/
│   └── app.js          ← Skrypty JavaScript
└── img/
    └── logo.png        ← Obrazy
```

## Dostęp przez przeglądarkę

Po uruchomieniu serwera web:

- **Strona główna**: `http://<IP_ESP32>/web`
- **Pliki statyczne**: `http://<IP_ESP32>/static/css/style.css`
- **API Status**: `http://<IP_ESP32>/api/status`

## Przykładowa strona index.html

```html
<!DOCTYPE html>
<html lang="pl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 LVGL Control Panel</title>
    <link rel="stylesheet" href="/static/css/style.css">
</head>
<body>
    <div class="container">
        <h1>ESP32-S3 Control Panel</h1>
        
        <div class="card">
            <h2>System Status</h2>
            <div id="status">
                <p>Uptime: <span id="uptime">--</span>s</p>
                <p>Free Heap: <span id="heap">--</span> bytes</p>
                <p>CPU Temp: <span id="temp">--</span>°C</p>
            </div>
        </div>
        
        <div class="card">
            <h2>Brightness Control</h2>
            <input type="range" id="brightness" min="0" max="100" value="50">
            <span id="brightnessValue">50%</span>
            <button onclick="setBrightness()">Set</button>
        </div>
        
        <div class="card">
            <h2>Screen Control</h2>
            <button onclick="changeScreen('home')">Home</button>
            <button onclick="changeScreen('settings')">Settings</button>
        </div>
    </div>
    
    <script src="/static/js/app.js"></script>
</body>
</html>
```

## Przykładowy CSS (style.css)

```css
body {
    font-family: Arial, sans-serif;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    margin: 0;
    padding: 20px;
}

.container {
    max-width: 800px;
    margin: 0 auto;
}

.card {
    background: white;
    border-radius: 10px;
    padding: 20px;
    margin-bottom: 20px;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
}

h1 {
    color: white;
    text-align: center;
}

button {
    background: #667eea;
    color: white;
    border: none;
    padding: 10px 20px;
    border-radius: 5px;
    cursor: pointer;
    margin: 5px;
}

button:hover {
    background: #5568d3;
}

input[type="range"] {
    width: 200px;
    margin: 10px;
}
```

## Przykładowy JavaScript (app.js)

```javascript
// Aktualizacja statusu systemowego
function updateStatus() {
    fetch('/api/status')
        .then(response => response.json())
        .then(data => {
            document.getElementById('uptime').textContent = data.uptime;
            document.getElementById('heap').textContent = data.freeHeap.toLocaleString();
            document.getElementById('temp').textContent = data.cpuTemp;
        })
        .catch(error => console.error('Error:', error));
}

// Ustawienie jasności
function setBrightness() {
    const level = document.getElementById('brightness').value;
    
    fetch('/api/brightness', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ level: parseInt(level) })
    })
    .then(response => response.json())
    .then(data => {
        console.log('Brightness set:', data);
        alert('Brightness set to ' + level + '%');
    })
    .catch(error => console.error('Error:', error));
}

// Zmiana ekranu
function changeScreen(screenName) {
    fetch('/api/screen', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ screen: screenName })
    })
    .then(response => response.json())
    .then(data => {
        console.log('Screen changed:', data);
        alert('Changed to ' + screenName + ' screen');
    })
    .catch(error => console.error('Error:', error));
}

// Aktualizacja wartości suwaka jasności
document.getElementById('brightness').addEventListener('input', function() {
    document.getElementById('brightnessValue').textContent = this.value + '%';
});

// Automatyczna aktualizacja statusu co 2 sekundy
setInterval(updateStatus, 2000);
updateStatus();
```

## Kopiowanie plików na kartę SD

### Opcja 1: Przez czytnik kart SD
1. Włóż kartę SD do czytnika
2. Stwórz folder `www`
3. Skopiuj pliki HTML/CSS/JS

### Opcja 2: Przez USB MSC (jeśli włączone)
1. Podłącz ESP32 przez USB
2. Karta SD pojawi się jako dysk w systemie
3. Skopiuj pliki do folderu `www`

### Opcja 3: Przez FTP (przyszła implementacja)
- Upload przez WiFi

## Testowanie

1. Wgraj firmware do ESP32
2. Skopiuj pliki do `SD:/www/`
3. Podłącz do WiFi
4. Otwórz przeglądarkę: `http://192.168.x.x/web`

## Debugowanie

Sprawdź logi Serial Monitor:
```
[WebInt] Setting up static file serving from /www
[WebInt] Serving file: /www/index.html
[WebInt] Static file routes registered
```
