#include <scheduler.hh>
#include <logger.hh>

#include <functional>
#include <chrono>
#include <thread>
using namespace std::chrono;

Task::Task(function_t function, milliseconds interval, bool shouldRepeat)
    : function_(function),
      shouldRepeat_(shouldRepeat),
      interval_(interval),
      nextExecutionPoint_() {
    nextExecutionPoint_ = time_point_cast<milliseconds>(steady_clock::now()) + interval_;
}

Task::timepoint_t Task::getNextExecutionPoint() {
    return nextExecutionPoint_;
}

bool Task::checkInvoke() {
    time_point<steady_clock, milliseconds> currentTime = time_point_cast<milliseconds>(steady_clock::now());
    if(currentTime >= nextExecutionPoint_) {
        function_();
        nextExecutionPoint_ = shouldRepeat_
            ? std::max(currentTime, nextExecutionPoint_) + interval_
            : timepoint_t::max();

        return true;
    }
    return false;
}

bool Task::operator==(const Task& other) const {
    return
        other.shouldRepeat_ == shouldRepeat_ &&
        other.function_.template target<void()>() == function_.template target<void()>() &&
        other.interval_ == interval_;
}

bool Task::operator!=(const Task& other) const {
    return !(other == *this);
}

void TaskScheduler::worker_thread() {
    std::unique_lock lock(mutex);

    while(shouldTaskSchedulerRun_) {
        Task::timepoint_t earliest_execution_point = Task::timepoint_t::max();

        for(Task& t : tasks_) {
            Task::timepoint_t execution_point = t.getNextExecutionPoint();
            if(execution_point < earliest_execution_point)
                earliest_execution_point = execution_point;

            t.checkInvoke();
        }

        cv_.wait_until(lock, earliest_execution_point);
    }

    lock.unlock();
}

void TaskScheduler::reset_worker() {
    cv_.notify_one();
}

void TaskScheduler::start() {
    if(isWorkerRunning()) {
        log_tag(LOG_WARN, "Attempted to double start worker thread!");
        return;
    }

    shouldTaskSchedulerRun_ = true;

    thread_ = std::thread(&TaskScheduler::worker_thread, this);
}

void TaskScheduler::registerTask(const Task& task) {
    {
        std::lock_guard lock(mutex);
        tasks_.push_back(task);
    }
    reset_worker();
}

TaskScheduler& TaskScheduler::operator+=(const Task& task) {
    registerTask(task);
    return *this;
}

void TaskScheduler::unregisterTask(const Task& task) {
    {
        std::lock_guard lock(mutex);
        tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(), [&task](const Task& t){return t == task;}));
    }
    reset_worker();
}

TaskScheduler& TaskScheduler::operator-=(const Task& task) {
    unregisterTask(task);
    return *this;
}

bool TaskScheduler::isWorkerRunning() {
    std::lock_guard lock(mutex);
    return shouldTaskSchedulerRun_;
}

void TaskScheduler::stop() {
    if(!isWorkerRunning()) {
        log_tag(LOG_WARN, "Attempted to stop a non-running worker thread!");
        return;
    }

    {
        std::lock_guard lock(mutex);
        shouldTaskSchedulerRun_ = false;
    }

    cv_.notify_one();
    if(thread_.joinable())
        thread_.join();
}
TaskScheduler::~TaskScheduler() {
    if(isWorkerRunning())
        stop();
}
