/**
 * @file diagnostics.hpp
 * @brief ESP32 Web Framework - Diagnostics Panel
 * 
 * System diagnostics and monitoring:
 * - Memory usage
 * - CPU information
 * - Task status
 * - Route listing
 * - Live logs
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_DIAGNOSTICS_HPP
#define ESP_WEB_FRAMEWORK_DIAGNOSTICS_HPP

#include <Arduino.h>
#include <vector>
#include <map>

#include "http.hpp"
#include "settings.h"

namespace espweb {

/**
 * @brief Task info structure
 */
struct TaskInfo {
    String name;
    uint32_t stackHighWaterMark;
    uint32_t priority;
    int coreId;
    uint32_t runtime;
};

/**
 * @brief Memory info structure
 */
struct MemoryInfo {
    size_t totalHeap;
    size_t freeHeap;
    size_t minFreeHeap;
    size_t maxAllocHeap;
    size_t totalPsram;
    size_t freePsram;
    size_t minFreePsram;
};

/**
 * @brief CPU info structure
 */
struct CpuInfo {
    uint32_t cpuFreqMHz;
    uint8_t numCores;
    String chipModel;
    uint8_t chipRevision;
    float temperature;
};

/**
 * @brief Connection info
 */
struct ConnectionInfo {
    String clientIP;
    uint16_t clientPort;
    uint32_t connectedAt;
    size_t bytesReceived;
    size_t bytesSent;
};

/**
 * @brief Performance sample
 */
struct PerfSample {
    uint32_t timestamp;
    float cpuUsage;
    size_t freeHeap;
    size_t freePsram;
    uint8_t activeConnections;
    float requestsPerSecond;
};

/**
 * @brief Diagnostics class - Singleton
 * 
 * Provides system information and monitoring.
 */
class Diagnostics {
public:
    /**
     * @brief Get singleton instance
     */
    static Diagnostics& getInstance();
    
    // Delete copy constructor and assignment
    Diagnostics(const Diagnostics&) = delete;
    Diagnostics& operator=(const Diagnostics&) = delete;
    
    /**
     * @brief Initialize diagnostics
     * @return true if successful
     */
    bool init();
    
    //==========================================================================
    // System Information
    //==========================================================================
    
    /**
     * @brief Get memory info
     */
    MemoryInfo getMemoryInfo();
    
    /**
     * @brief Get CPU info
     */
    CpuInfo getCpuInfo();
    
    /**
     * @brief Get uptime in seconds
     */
    uint32_t getUptime();
    
    /**
     * @brief Get uptime as formatted string
     */
    String getUptimeString();
    
    /**
     * @brief Get free heap percentage
     */
    float getFreeHeapPercent();
    
    /**
     * @brief Get free PSRAM percentage
     */
    float getFreePsramPercent();
    
    /**
     * @brief Get chip temperature
     */
    float getTemperature();
    
    //==========================================================================
    // Task Information
    //==========================================================================
    
    /**
     * @brief Get all task info
     */
    std::vector<TaskInfo> getTaskInfo();
    
    /**
     * @brief Get task count
     */
    size_t getTaskCount();
    
    //==========================================================================
    // Network Information
    //==========================================================================
    
    /**
     * @brief Get WiFi info as JSON string
     */
    String getWiFiInfo();
    
    /**
     * @brief Get IP address
     */
    String getIPAddress();
    
    /**
     * @brief Get MAC address
     */
    String getMACAddress();
    
    /**
     * @brief Get WiFi signal strength (RSSI)
     */
    int getRSSI();
    
    //==========================================================================
    // Server Statistics
    //==========================================================================
    
    /**
     * @brief Record request
     * @param path Request path
     * @param method HTTP method
     * @param responseTime Response time in ms
     * @param statusCode Response status
     */
    void recordRequest(const String& path, const String& method,
                       uint32_t responseTime, int statusCode);
    
    /**
     * @brief Get total requests
     */
    uint32_t getTotalRequests() const { return totalRequests_; }
    
    /**
     * @brief Get requests per second (rolling average)
     */
    float getRequestsPerSecond();
    
    /**
     * @brief Get average response time
     */
    float getAvgResponseTime();
    
    /**
     * @brief Get error count (4xx + 5xx)
     */
    uint32_t getErrorCount() const { return errorCount_; }
    
    /**
     * @brief Get request count by path
     */
    std::map<String, uint32_t> getRequestsByPath();
    
    /**
     * @brief Get status code distribution
     */
    std::map<int, uint32_t> getStatusCodes();
    
    //==========================================================================
    // Performance Monitoring
    //==========================================================================
    
    /**
     * @brief Start performance sampling
     * @param intervalMs Sample interval
     */
    void startSampling(uint32_t intervalMs = 1000);
    
    /**
     * @brief Stop performance sampling
     */
    void stopSampling();
    
    /**
     * @brief Get performance samples
     * @param count Number of samples
     */
    std::vector<PerfSample> getSamples(size_t count = 60);
    
    //==========================================================================
    // Diagnostics Panel
    //==========================================================================
    
    /**
     * @brief Register diagnostics routes
     * @param router Router reference
     */
    void registerRoutes(class Router& router);
    
    /**
     * @brief Generate diagnostics HTML page
     */
    String generateDashboard();
    
    /**
     * @brief Generate diagnostics JSON
     */
    String generateJson();
    
    //==========================================================================
    // Alerts
    //==========================================================================
    
    /**
     * @brief Set low memory threshold
     * @param bytes Threshold in bytes
     */
    void setLowMemoryThreshold(size_t bytes) { lowMemoryThreshold_ = bytes; }
    
    /**
     * @brief Check for alerts
     */
    std::vector<String> checkAlerts();
    
private:
    Diagnostics();
    ~Diagnostics() = default;
    
    /**
     * @brief Sampling task
     */
    static void samplingTask(void* parameter);
    
    uint32_t totalRequests_ = 0;
    uint32_t errorCount_ = 0;
    uint64_t totalResponseTime_ = 0;
    
    std::map<String, uint32_t> requestsByPath_;
    std::map<int, uint32_t> statusCodes_;
    
    std::vector<PerfSample> samples_;
    size_t maxSamples_ = 3600;
    bool sampling_ = false;
    TaskHandle_t samplingTaskHandle_ = nullptr;
    
    size_t lowMemoryThreshold_ = 10000;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get diagnostics
 */
inline Diagnostics& Diag() {
    return Diagnostics::getInstance();
}

//==============================================================================
// Profiler
//==============================================================================

/**
 * @brief Simple profiler for measuring execution time
 */
class Profiler {
public:
    /**
     * @brief Start profiling
     * @param name Profile name
     */
    static void start(const String& name);
    
    /**
     * @brief End profiling
     * @param name Profile name
     * @return Elapsed time in microseconds
     */
    static uint32_t end(const String& name);
    
    /**
     * @brief Get profile statistics
     */
    static std::map<String, std::pair<uint32_t, uint32_t>> getStats();
    
    /**
     * @brief Clear statistics
     */
    static void clear();
    
private:
    static std::map<String, uint32_t> starts_;
    static std::map<String, std::pair<uint32_t, uint32_t>> stats_; // count, total time
};

/**
 * @brief RAII profiler scope
 */
class ProfileScope {
public:
    ProfileScope(const String& name) : name_(name) {
        Profiler::start(name_);
    }
    
    ~ProfileScope() {
        Profiler::end(name_);
    }
    
private:
    String name_;
};

#define PROFILE_SCOPE(name) espweb::ProfileScope _profile_##__LINE__(name)
#define PROFILE_FUNCTION() espweb::ProfileScope _profile_func(__FUNCTION__)

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_DIAGNOSTICS_HPP
