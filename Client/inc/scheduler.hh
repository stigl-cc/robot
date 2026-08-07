#pragma once
#include <condition_variable>
#include <functional>
#include <chrono>
#include <thread>

class Task {
    public:
    typedef std::function<void()> function_t;
    typedef std::chrono::milliseconds timeunit_t;
    typedef std::chrono::time_point<std::chrono::steady_clock, timeunit_t> timepoint_t;

    private:
    std::function<void()> function_;

    bool shouldRepeat_;
    timeunit_t interval_;
    timepoint_t nextExecutionPoint_;

    public:
    Task(std::function<void()> function, timeunit_t interval, bool shouldRepeat = true);

    void schedule();
    timepoint_t getNextExecutionPoint();
    bool checkInvoke();
    bool operator==(const Task&) const;
    bool operator!=(const Task&) const;
};

class TaskScheduler {
    private:
    static constexpr std::string_view LOG_TAG = "Scheduler";
    bool shouldTaskSchedulerRun_;
    std::mutex mutex;
    std::condition_variable cv_;

    std::thread thread_;
    std::vector<Task> tasks_;

    void reset_worker();
    void worker_thread();

    public:
    void start(bool blocking);

    void registerTask(const Task& task);
    void unregisterTask(const Task& task);

    TaskScheduler& operator+=(const Task& task);
    TaskScheduler& operator-=(const Task& task);
    bool isWorkerRunning();

    void stop();
    ~TaskScheduler();
};
