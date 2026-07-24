/**
 * @file cron.hpp
 * @brief ESP32 Web Framework - Task Scheduler
 * 
 * Periodic task scheduler:
 * - Cron-like job scheduling
 * - Interval-based tasks
 * - Integration with other modules
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_CRON_HPP
#define ESP_WEB_FRAMEWORK_CRON_HPP

#include <Arduino.h>
#include <vector>
#include <functional>
#include <memory>

#include "settings.h"

namespace espweb {

/**
 * @brief Scheduled job callback type
 */
using JobCallback = std::function<void()>;

/**
 * @brief Job status
 */
enum class JobStatus {
    ACTIVE,
    PAUSED,
    COMPLETED,
    ERROR
};

/**
 * @brief Scheduled job definition
 */
struct Job {
    uint32_t id;
    String name;
    JobCallback callback;
    
    // Scheduling
    uint32_t intervalMs;            ///< Interval in milliseconds
    uint32_t repeatCount;           ///< Number of times to repeat (0 = infinite)
    uint32_t executedCount;         ///< Times executed
    
    // Timing
    uint32_t nextRun;               ///< Next run timestamp (millis)
    uint32_t lastRun;               ///< Last run timestamp
    uint32_t lastDuration;          ///< Last execution duration
    
    // Status
    JobStatus status;
    String lastError;
    
    Job() : id(0), intervalMs(0), repeatCount(0), executedCount(0),
            nextRun(0), lastRun(0), lastDuration(0), status(JobStatus::ACTIVE) {}
};

/**
 * @brief Scheduler class - Singleton
 * 
 * Manages periodic task execution.
 */
class Scheduler {
public:
    /**
     * @brief Get singleton instance
     */
    static Scheduler& getInstance();
    
    // Delete copy constructor and assignment
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    
    /**
     * @brief Initialize scheduler
     * @return true if successful
     */
    bool init();
    
    /**
     * @brief Start scheduler task
     * @return true if started
     */
    bool start();
    
    /**
     * @brief Stop scheduler
     */
    void stop();
    
    /**
     * @brief Check if running
     */
    bool isRunning() const { return running_; }
    
    //==========================================================================
    // Job Management
    //==========================================================================
    
    /**
     * @brief Add interval job
     * @param name Job name
     * @param intervalMs Interval in milliseconds
     * @param callback Job callback
     * @param repeatCount Number of repeats (0 = infinite)
     * @return Job ID
     */
    uint32_t every(const String& name, uint32_t intervalMs, 
                   JobCallback callback, uint32_t repeatCount = 0);
    
    /**
     * @brief Alias for every() - schedule a periodic job
     * @param name Job name
     * @param intervalMs Interval in milliseconds
     * @param callback Job callback
     * @return Job ID
     */
    uint32_t schedule(const String& name, uint32_t intervalMs, JobCallback callback) {
        return every(name, intervalMs, callback, 0);
    }
    
    /**
     * @brief Add one-time job
     * @param name Job name
     * @param delayMs Delay before execution
     * @param callback Job callback
     * @return Job ID
     */
    uint32_t once(const String& name, uint32_t delayMs, JobCallback callback);
    
    /**
     * @brief Add job that runs every second
     */
    uint32_t everySecond(const String& name, JobCallback callback);
    
    /**
     * @brief Add job that runs every minute
     */
    uint32_t everyMinute(const String& name, JobCallback callback);
    
    /**
     * @brief Add job that runs every hour
     */
    uint32_t everyHour(const String& name, JobCallback callback);
    
    /**
     * @brief Remove job by ID
     * @param jobId Job ID
     * @return true if removed
     */
    bool remove(uint32_t jobId);
    
    /**
     * @brief Remove job by name
     * @param name Job name
     * @return true if removed
     */
    bool remove(const String& name);
    
    /**
     * @brief Pause job
     * @param jobId Job ID
     */
    void pause(uint32_t jobId);
    
    /**
     * @brief Resume job
     * @param jobId Job ID
     */
    void resume(uint32_t jobId);
    
    /**
     * @brief Get job by ID
     * @param jobId Job ID
     * @return Pointer to job or nullptr
     */
    Job* getJob(uint32_t jobId);
    
    /**
     * @brief Get job by name
     * @param name Job name
     * @return Pointer to job or nullptr
     */
    Job* getJob(const String& name);
    
    /**
     * @brief Get all jobs
     * @return Vector of job pointers
     */
    std::vector<Job*> getAllJobs();
    
    /**
     * @brief Clear all jobs
     */
    void clear();
    
    //==========================================================================
    // Statistics
    //==========================================================================
    
    /**
     * @brief Get total executions
     */
    uint32_t getTotalExecutions() const { return totalExecutions_; }
    
    /**
     * @brief Get active job count
     */
    size_t getActiveJobCount() const;
    
private:
    Scheduler();
    ~Scheduler() = default;
    
    /**
     * @brief Scheduler task function
     */
    static void schedulerTask(void* parameter);
    
    /**
     * @brief Execute job
     */
    void executeJob(Job& job);
    
    /**
     * @brief Check and run due jobs
     */
    void tick();
    
    std::vector<std::unique_ptr<Job>> jobs_;
    
    bool running_ = false;
    TaskHandle_t taskHandle_ = nullptr;
    uint32_t nextJobId_ = 1;
    uint32_t totalExecutions_ = 0;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get scheduler
 */
inline Scheduler& Cron() {
    return Scheduler::getInstance();
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_CRON_HPP
