#include "threadpool.h"
namespace sol::math {

threadpool::threadpool(const size_t _num_threads)
    : m_stop(false), m_unfinished_jobs(0)
{
    for (size_t i = 0; i < _num_threads; ++i) {
        m_workers.emplace_back(&threadpool::worker_thread, this);
    }
}

threadpool::~threadpool() {
    m_stop.store(true);
    m_condition.notify_all();
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void threadpool::enqueue_job(std::function<void(JobArgs)> const& _job, JobArgs const& _args)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_job_queue.emplace(_job, _args);
        ++m_unfinished_jobs;
    }
    m_condition.notify_one();
}

void threadpool::wait_for_completion() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_jobs_done_condition.wait(lock, [this]() { return m_unfinished_jobs == 0; });
}

void threadpool::worker_thread() {
    while (!m_stop.load()) {
        JobQueueElement job_element;
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_condition.wait(lock, [this]() { return !m_job_queue.empty() || m_stop.load(); });
            if (m_stop.load() && m_job_queue.empty()) {
                return;
            }
            job_element = m_job_queue.front();
            m_job_queue.pop();
        }
        auto const& job = std::get<0>(job_element);
        job(std::get<1>(job_element));
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            --m_unfinished_jobs;
            if (m_unfinished_jobs == 0) {
                m_jobs_done_condition.notify_all();
            }
        }
    }
}


}
