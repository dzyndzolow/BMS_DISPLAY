/**
 * @file diagnostics.cpp
 * @brief ESP32 Web Framework - Diagnostics Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "diagnostics.hpp"
#include "router.hpp"
#include "logger.hpp"
#include <WiFi.h>
#include <esp_system.h>
#include <esp_chip_info.h>

#if CONFIG_IDF_TARGET_ESP32
extern "C" {
    uint8_t temprature_sens_read();
}
#endif

namespace espweb {

// Profiler static members
std::map<String, uint32_t> Profiler::starts_;
std::map<String, std::pair<uint32_t, uint32_t>> Profiler::stats_;

// Singleton instance
Diagnostics& Diagnostics::getInstance() {
    static Diagnostics instance;
    return instance;
}

Diagnostics::Diagnostics() {
    mutex_ = xSemaphoreCreateMutex();
}

bool Diagnostics::init() {
    LOG_INFO("Diag", "Diagnostics initialized");
    return true;
}

//==============================================================================
// System Information
//==============================================================================

MemoryInfo Diagnostics::getMemoryInfo() {
    MemoryInfo info;
    info.totalHeap = ESP.getHeapSize();
    info.freeHeap = ESP.getFreeHeap();
    info.minFreeHeap = ESP.getMinFreeHeap();
    info.maxAllocHeap = ESP.getMaxAllocHeap();
    
    if (psramFound()) {
        info.totalPsram = ESP.getPsramSize();
        info.freePsram = ESP.getFreePsram();
        info.minFreePsram = ESP.getMinFreePsram();
    } else {
        info.totalPsram = 0;
        info.freePsram = 0;
        info.minFreePsram = 0;
    }
    
    return info;
}

CpuInfo Diagnostics::getCpuInfo() {
    CpuInfo info;
    info.cpuFreqMHz = ESP.getCpuFreqMHz();
    
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    info.numCores = chip_info.cores;
    info.chipRevision = chip_info.revision;
    
    switch (chip_info.model) {
        case CHIP_ESP32: info.chipModel = "ESP32"; break;
        case CHIP_ESP32S2: info.chipModel = "ESP32-S2"; break;
        case CHIP_ESP32S3: info.chipModel = "ESP32-S3"; break;
        case CHIP_ESP32C3: info.chipModel = "ESP32-C3"; break;
        default: info.chipModel = "Unknown";
    }
    
    info.temperature = getTemperature();
    
    return info;
}

uint32_t Diagnostics::getUptime() {
    return millis() / 1000;
}

String Diagnostics::getUptimeString() {
    uint32_t seconds = getUptime();
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;
    
    String result;
    if (days > 0) {
        result += String(days) + "d ";
    }
    result += String(hours % 24) + "h ";
    result += String(minutes % 60) + "m ";
    result += String(seconds % 60) + "s";
    
    return result;
}

float Diagnostics::getFreeHeapPercent() {
    MemoryInfo mem = getMemoryInfo();
    return (float)mem.freeHeap / mem.totalHeap * 100.0f;
}

float Diagnostics::getFreePsramPercent() {
    MemoryInfo mem = getMemoryInfo();
    if (mem.totalPsram == 0) return 0;
    return (float)mem.freePsram / mem.totalPsram * 100.0f;
}

float Diagnostics::getTemperature() {
#if CONFIG_IDF_TARGET_ESP32
    return (temprature_sens_read() - 32) / 1.8f;
#else
    return 0; // Temperature sensor not available on all chips
#endif
}

//==============================================================================
// Task Information
//==============================================================================

std::vector<TaskInfo> Diagnostics::getTaskInfo() {
    std::vector<TaskInfo> tasks;
    
    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    TaskStatus_t* taskStatusArray = (TaskStatus_t*)pvPortMalloc(
        taskCount * sizeof(TaskStatus_t));
    
    if (taskStatusArray) {
        uint32_t totalRunTime;
        taskCount = uxTaskGetSystemState(taskStatusArray, taskCount, &totalRunTime);
        
        for (UBaseType_t i = 0; i < taskCount; i++) {
            TaskInfo info;
            info.name = taskStatusArray[i].pcTaskName;
            info.stackHighWaterMark = taskStatusArray[i].usStackHighWaterMark;
            info.priority = taskStatusArray[i].uxCurrentPriority;
            info.coreId = taskStatusArray[i].xCoreID;
            info.runtime = taskStatusArray[i].ulRunTimeCounter;
            tasks.push_back(info);
        }
        
        vPortFree(taskStatusArray);
    }
    
    return tasks;
}

size_t Diagnostics::getTaskCount() {
    return uxTaskGetNumberOfTasks();
}

//==============================================================================
// Network Information
//==============================================================================

String Diagnostics::getWiFiInfo() {
    JsonDocument doc;
    
    doc["connected"] = WiFi.isConnected();
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["gateway"] = WiFi.gatewayIP().toString();
    doc["subnet"] = WiFi.subnetMask().toString();
    doc["dns"] = WiFi.dnsIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["channel"] = WiFi.channel();
    
    String result;
    serializeJson(doc, result);
    return result;
}

String Diagnostics::getIPAddress() {
    return WiFi.localIP().toString();
}

String Diagnostics::getMACAddress() {
    return WiFi.macAddress();
}

int Diagnostics::getRSSI() {
    return WiFi.RSSI();
}

//==============================================================================
// Server Statistics
//==============================================================================

void Diagnostics::recordRequest(const String& path, const String& method,
                                uint32_t responseTime, int statusCode) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    totalRequests_++;
    totalResponseTime_ += responseTime;
    
    requestsByPath_[path]++;
    statusCodes_[statusCode]++;
    
    if (statusCode >= 400) {
        errorCount_++;
    }
    
    xSemaphoreGive(mutex_);
}

float Diagnostics::getRequestsPerSecond() {
    uint32_t uptime = getUptime();
    if (uptime == 0) return 0;
    return (float)totalRequests_ / uptime;
}

float Diagnostics::getAvgResponseTime() {
    if (totalRequests_ == 0) return 0;
    return (float)totalResponseTime_ / totalRequests_;
}

std::map<String, uint32_t> Diagnostics::getRequestsByPath() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    std::map<String, uint32_t> copy = requestsByPath_;
    xSemaphoreGive(mutex_);
    return copy;
}

std::map<int, uint32_t> Diagnostics::getStatusCodes() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    std::map<int, uint32_t> copy = statusCodes_;
    xSemaphoreGive(mutex_);
    return copy;
}

//==============================================================================
// Performance Monitoring
//==============================================================================

void Diagnostics::startSampling(uint32_t intervalMs) {
    if (sampling_) return;
    
    sampling_ = true;
    
    // Create sampling task
    xTaskCreatePinnedToCore(
        samplingTask,
        "DiagSampling",
        2048,
        this,
        1,
        &samplingTaskHandle_,
        0
    );
}

void Diagnostics::stopSampling() {
    if (!sampling_) return;
    
    sampling_ = false;
    
    if (samplingTaskHandle_) {
        vTaskDelete(samplingTaskHandle_);
        samplingTaskHandle_ = nullptr;
    }
}

std::vector<PerfSample> Diagnostics::getSamples(size_t count) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    std::vector<PerfSample> result;
    size_t start = samples_.size() > count ? samples_.size() - count : 0;
    
    for (size_t i = start; i < samples_.size(); i++) {
        result.push_back(samples_[i]);
    }
    
    xSemaphoreGive(mutex_);
    return result;
}

void Diagnostics::samplingTask(void* parameter) {
    Diagnostics* diag = static_cast<Diagnostics*>(parameter);
    
    while (diag->sampling_) {
        PerfSample sample;
        sample.timestamp = millis();
        sample.freeHeap = ESP.getFreeHeap();
        sample.freePsram = psramFound() ? ESP.getFreePsram() : 0;
        sample.requestsPerSecond = diag->getRequestsPerSecond();
        // CPU usage would require more complex measurement
        sample.cpuUsage = 0;
        
        xSemaphoreTake(diag->mutex_, portMAX_DELAY);
        diag->samples_.push_back(sample);
        
        // Limit sample count
        while (diag->samples_.size() > diag->maxSamples_) {
            diag->samples_.erase(diag->samples_.begin());
        }
        xSemaphoreGive(diag->mutex_);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

//==============================================================================
// Diagnostics Panel
//==============================================================================

void Diagnostics::registerRoutes(Router& router) {
    router.get("/_diag", [this](Request& request) -> Response {
        return Response::html(generateDashboard());
    }, "diagnostics_dashboard");
    
    router.get("/_diag/api", [this](Request& request) -> Response {
        return Response::json(generateJson());
    }, "diagnostics_api");
}

String Diagnostics::generateDashboard() {
    MemoryInfo mem = getMemoryInfo();
    CpuInfo cpu = getCpuInfo();
    
    String html = R"html(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Web Framework - Diagnostics</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        h1 { color: #333; }
        .card { background: white; border-radius: 8px; padding: 20px; margin: 10px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
        .stat { margin: 10px 0; }
        .stat-label { color: #666; font-size: 0.9em; }
        .stat-value { font-size: 1.5em; font-weight: bold; color: #333; }
        .progress { background: #e0e0e0; border-radius: 10px; height: 20px; }
        .progress-bar { background: #4caf50; height: 100%; border-radius: 10px; }
        table { width: 100%; border-collapse: collapse; }
        th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }
        .refresh-btn { background: #2196F3; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; }
        .refresh-btn:hover { background: #1976D2; }
    </style>
</head>
<body>
    <h1>🔧 ESP32 Web Framework - Diagnostics</h1>
    <button class="refresh-btn" onclick="location.reload()">Refresh</button>
    
    <div class="grid">
        <div class="card">
            <h2>🖥️ System</h2>
            <div class="stat">
                <div class="stat-label">Chip</div>
                <div class="stat-value">)html" + cpu.chipModel + " Rev " + String(cpu.chipRevision) + R"html(</div>
            </div>
            <div class="stat">
                <div class="stat-label">CPU Frequency</div>
                <div class="stat-value">)html" + String(cpu.cpuFreqMHz) + R"html( MHz</div>
            </div>
            <div class="stat">
                <div class="stat-label">Cores</div>
                <div class="stat-value">)html" + String(cpu.numCores) + R"html(</div>
            </div>
            <div class="stat">
                <div class="stat-label">Uptime</div>
                <div class="stat-value">)html" + getUptimeString() + R"html(</div>
            </div>
        </div>
        
        <div class="card">
            <h2>💾 Memory</h2>
            <div class="stat">
                <div class="stat-label">Heap: )html" + String(mem.freeHeap / 1024) + "KB / " + String(mem.totalHeap / 1024) + R"html(KB</div>
                <div class="progress">
                    <div class="progress-bar" style="width: )html" + String(100 - getFreeHeapPercent()) + R"html(%"></div>
                </div>
            </div>)html";
    
    if (mem.totalPsram > 0) {
        html += R"html(
            <div class="stat">
                <div class="stat-label">PSRAM: )html" + String(mem.freePsram / 1024) + "KB / " + String(mem.totalPsram / 1024) + R"html(KB</div>
                <div class="progress">
                    <div class="progress-bar" style="width: )html" + String(100 - getFreePsramPercent()) + R"html(%"></div>
                </div>
            </div>)html";
    }
    
    html += R"html(
        </div>
        
        <div class="card">
            <h2>📡 Network</h2>
            <div class="stat">
                <div class="stat-label">IP Address</div>
                <div class="stat-value">)html" + getIPAddress() + R"html(</div>
            </div>
            <div class="stat">
                <div class="stat-label">MAC Address</div>
                <div class="stat-value">)html" + getMACAddress() + R"html(</div>
            </div>
            <div class="stat">
                <div class="stat-label">RSSI</div>
                <div class="stat-value">)html" + String(getRSSI()) + R"html( dBm</div>
            </div>
        </div>
        
        <div class="card">
            <h2>📊 Server Stats</h2>
            <div class="stat">
                <div class="stat-label">Total Requests</div>
                <div class="stat-value">)html" + String(totalRequests_) + R"html(</div>
            </div>
            <div class="stat">
                <div class="stat-label">Avg Response Time</div>
                <div class="stat-value">)html" + String(getAvgResponseTime(), 1) + R"html( ms</div>
            </div>
            <div class="stat">
                <div class="stat-label">Errors</div>
                <div class="stat-value">)html" + String(errorCount_) + R"html(</div>
            </div>
        </div>
    </div>
    
    <div class="card">
        <h2>⚙️ Tasks</h2>
        <table>
            <tr><th>Name</th><th>Priority</th><th>Core</th><th>Stack Free</th></tr>)html";
    
    std::vector<TaskInfo> tasks = getTaskInfo();
    for (const auto& task : tasks) {
        html += "<tr><td>" + task.name + "</td>";
        html += "<td>" + String(task.priority) + "</td>";
        html += "<td>" + String(task.coreId) + "</td>";
        html += "<td>" + String(task.stackHighWaterMark * 4) + " bytes</td></tr>";
    }
    
    html += R"html(
        </table>
    </div>
    
    <script>
        // Auto-refresh every 5 seconds
        // setTimeout(() => location.reload(), 5000);
    </script>
</body>
</html>)html";
    
    return html;
}

String Diagnostics::generateJson() {
    JsonDocument doc;
    
    MemoryInfo mem = getMemoryInfo();
    CpuInfo cpu = getCpuInfo();
    
    doc["uptime"] = getUptime();
    doc["uptimeStr"] = getUptimeString();
    
    JsonObject memory = doc["memory"].to<JsonObject>();
    memory["totalHeap"] = mem.totalHeap;
    memory["freeHeap"] = mem.freeHeap;
    memory["minFreeHeap"] = mem.minFreeHeap;
    memory["totalPsram"] = mem.totalPsram;
    memory["freePsram"] = mem.freePsram;
    
    JsonObject cpuObj = doc["cpu"].to<JsonObject>();
    cpuObj["model"] = cpu.chipModel;
    cpuObj["freqMHz"] = cpu.cpuFreqMHz;
    cpuObj["cores"] = cpu.numCores;
    cpuObj["revision"] = cpu.chipRevision;
    cpuObj["temperature"] = cpu.temperature;
    
    JsonObject network = doc["network"].to<JsonObject>();
    network["ip"] = getIPAddress();
    network["mac"] = getMACAddress();
    network["rssi"] = getRSSI();
    
    JsonObject server = doc["server"].to<JsonObject>();
    server["totalRequests"] = totalRequests_;
    server["errorCount"] = errorCount_;
    server["avgResponseTime"] = getAvgResponseTime();
    server["requestsPerSecond"] = getRequestsPerSecond();
    
    String result;
    serializeJson(doc, result);
    return result;
}

std::vector<String> Diagnostics::checkAlerts() {
    std::vector<String> alerts;
    
    MemoryInfo mem = getMemoryInfo();
    
    if (mem.freeHeap < lowMemoryThreshold_) {
        alerts.push_back("Low heap memory: " + String(mem.freeHeap) + " bytes");
    }
    
    if (mem.totalPsram > 0 && mem.freePsram < lowMemoryThreshold_) {
        alerts.push_back("Low PSRAM: " + String(mem.freePsram) + " bytes");
    }
    
    int rssi = getRSSI();
    if (rssi < -80) {
        alerts.push_back("Weak WiFi signal: " + String(rssi) + " dBm");
    }
    
    return alerts;
}

//==============================================================================
// Profiler Implementation
//==============================================================================

void Profiler::start(const String& name) {
    starts_[name] = micros();
}

uint32_t Profiler::end(const String& name) {
    auto it = starts_.find(name);
    if (it == starts_.end()) {
        return 0;
    }
    
    uint32_t elapsed = micros() - it->second;
    starts_.erase(it);
    
    // Update statistics
    auto statIt = stats_.find(name);
    if (statIt != stats_.end()) {
        statIt->second.first++;  // count
        statIt->second.second += elapsed;  // total time
    } else {
        stats_[name] = {1, elapsed};
    }
    
    return elapsed;
}

std::map<String, std::pair<uint32_t, uint32_t>> Profiler::getStats() {
    return stats_;
}

void Profiler::clear() {
    starts_.clear();
    stats_.clear();
}

} // namespace espweb
