#pragma once
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace sol::math {
class threadpool {
    // NOLINT
public:
    struct JobArgs {
        size_t a;
        size_t b;
    };

public:
    explicit threadpool(size_t _num_threads);
    ~threadpool();

    size_t workers_num() const { return m_workers.size(); }
    void enqueue_job(std::function<void(JobArgs)> const& _job, JobArgs const&);
    void wait_for_completion();

private:
    void worker_thread();

    std::vector<std::thread> m_workers;
    typedef std::tuple<std::function<void(JobArgs)>, JobArgs> JobQueueElement;
    std::queue<JobQueueElement> m_job_queue;
    std::mutex m_queue_mutex;

    std::condition_variable m_condition;
    std::condition_variable m_jobs_done_condition;
    std::atomic<bool> m_stop;
    std::atomic<int> m_unfinished_jobs;
};
}
