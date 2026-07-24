/**
 * @file cron.cpp
 * @brief ESP32 Web Framework - Scheduler Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "cron.hpp"
#include "logger.hpp"

namespace espweb {

// Singleton instance
Scheduler& Scheduler::getInstance() {
    static Scheduler instance;
    return instance;
}

Scheduler::Scheduler() {
    mutex_ = xSemaphoreCreateMutex();
}

bool Scheduler::init() {
    LOG_INFO("Cron", "Scheduler initialized");
    return true;
}

bool Scheduler::start() {
    if (running_) {
        return true;
    }
    
    // Set running BEFORE creating task to avoid race condition
    running_ = true;
    
    BaseType_t result = xTaskCreatePinnedToCore(
        schedulerTask,
        "CronTask",
        settings::CRON_STACK_SIZE,
        this,
        settings::CRON_TASK_PRIORITY,
        &taskHandle_,
        settings::CRON_CORE
    );
    
    if (result != pdPASS) {
        running_ = false;
        LOG_ERROR("Cron", "Failed to create scheduler task");
        return false;
    }
    
    LOG_INFO("Cron", "Scheduler started");
    return true;
}

void Scheduler::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (taskHandle_) {
        vTaskDelete(taskHandle_);
        taskHandle_ = nullptr;
    }
    
    LOG_INFO("Cron", "Scheduler stopped");
}

uint32_t Scheduler::every(const String& name, uint32_t intervalMs,
                          JobCallback callback, uint32_t repeatCount) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto job = std::make_unique<Job>();
    job->id = nextJobId_++;
    job->name = name;
    job->callback = callback;
    job->intervalMs = intervalMs;
    job->repeatCount = repeatCount;
    job->executedCount = 0;
    job->nextRun = millis() + intervalMs;
    job->lastRun = 0;
    job->status = JobStatus::ACTIVE;
    
    uint32_t id = job->id;
    jobs_.push_back(std::move(job));
    
    xSemaphoreGive(mutex_);
    
    LOG_DEBUG("Cron", "Added job: " + name + " (every " + String(intervalMs) + "ms)");
    return id;
}

uint32_t Scheduler::once(const String& name, uint32_t delayMs, JobCallback callback) {
    return every(name, delayMs, callback, 1);
}

uint32_t Scheduler::everySecond(const String& name, JobCallback callback) {
    return every(name, 1000, callback);
}

uint32_t Scheduler::everyMinute(const String& name, JobCallback callback) {
    return every(name, 60000, callback);
}

uint32_t Scheduler::everyHour(const String& name, JobCallback callback) {
    return every(name, 3600000, callback);
}

bool Scheduler::remove(uint32_t jobId) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    for (auto it = jobs_.begin(); it != jobs_.end(); ++it) {
        if ((*it)->id == jobId) {
            LOG_DEBUG("Cron", "Removed job: " + (*it)->name);
            jobs_.erase(it);
            xSemaphoreGive(mutex_);
            return true;
        }
    }
    
    xSemaphoreGive(mutex_);
    return false;
}

bool Scheduler::remove(const String& name) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    for (auto it = jobs_.begin(); it != jobs_.end(); ++it) {
        if ((*it)->name == name) {
            jobs_.erase(it);
            xSemaphoreGive(mutex_);
            return true;
        }
    }
    
    xSemaphoreGive(mutex_);
    return false;
}

void Scheduler::pause(uint32_t jobId) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    for (auto& job : jobs_) {
        if (job->id == jobId) {
            job->status = JobStatus::PAUSED;
            break;
        }
    }
    
    xSemaphoreGive(mutex_);
}

void Scheduler::resume(uint32_t jobId) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    for (auto& job : jobs_) {
        if (job->id == jobId) {
            job->status = JobStatus::ACTIVE;
            job->nextRun = millis() + job->intervalMs;
            break;
        }
    }
    
    xSemaphoreGive(mutex_);
}

Job* Scheduler::getJob(uint32_t jobId) {
    for (auto& job : jobs_) {
        if (job->id == jobId) {
            return job.get();
        }
    }
    return nullptr;
}

Job* Scheduler::getJob(const String& name) {
    for (auto& job : jobs_) {
        if (job->name == name) {
            return job.get();
        }
    }
    return nullptr;
}

std::vector<Job*> Scheduler::getAllJobs() {
    std::vector<Job*> result;
    for (auto& job : jobs_) {
        result.push_back(job.get());
    }
    return result;
}

void Scheduler::clear() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    jobs_.clear();
    xSemaphoreGive(mutex_);
}

size_t Scheduler::getActiveJobCount() const {
    size_t count = 0;
    for (const auto& job : jobs_) {
        if (job->status == JobStatus::ACTIVE) {
            count++;
        }
    }
    return count;
}

void Scheduler::schedulerTask(void* parameter) {
    Scheduler* scheduler = static_cast<Scheduler*>(parameter);
    
    LOG_DEBUG("Cron", "Scheduler task started");
    
    // FreeRTOS tasks must NEVER return - use infinite loop
    for (;;) {
        if (scheduler->running_) {
            scheduler->tick();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
    
    // This code is never reached - task deletion handled by stop() method
}

void Scheduler::tick() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    uint32_t now = millis();
    
    for (auto it = jobs_.begin(); it != jobs_.end();) {
        Job& job = **it;
        
        if (job.status != JobStatus::ACTIVE) {
            ++it;
            continue;
        }
        
        if (now >= job.nextRun) {
            executeJob(job);
            
            // Check if job should be removed
            if (job.repeatCount > 0 && job.executedCount >= job.repeatCount) {
                job.status = JobStatus::COMPLETED;
                it = jobs_.erase(it);
                continue;
            }
            
            // Schedule next run
            job.nextRun = now + job.intervalMs;
        }
        
        ++it;
    }
    
    xSemaphoreGive(mutex_);
}

void Scheduler::executeJob(Job& job) {
    uint32_t startTime = millis();
    
    try {
        job.callback();
        job.lastError = "";
    } catch (...) {
        job.status = JobStatus::ERROR;
        job.lastError = "Exception during execution";
        LOG_ERROR("Cron", "Job " + job.name + " threw exception");
    }
    
    job.lastRun = startTime;
    job.lastDuration = millis() - startTime;
    job.executedCount++;
    totalExecutions_++;
}

} // namespace espweb
