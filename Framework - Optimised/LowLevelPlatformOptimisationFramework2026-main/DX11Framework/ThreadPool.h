#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:

    size_t ThreadCount() const { return _workers.size(); }
    
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency() - 1)
    {
        for (size_t i = 0; i < threadCount; ++i)
            _workers.emplace_back([this](std::stop_token st) { WorkerLoop(st); });
    }

    ~ThreadPool()
    {
        for (auto& w : _workers) w.request_stop();
        _wake.notify_all();
    }

    void Submit(std::function<void()> task)
    {
        {
            std::scoped_lock lock(_mutex);
            _tasks.push(std::move(task));
            ++_pending;
        }
        _wake.notify_one();
    }

    void Wait()
    {
        // Block until every submit task finished
        std::unique_lock lock(_mutex);
        _done.wait(lock, [this] { return _pending == 0; });
    }


private:
    
    std::mutex _mutex;

    std::condition_variable_any _wake;
    std::condition_variable _done;

    std::queue<std::function<void()>> _tasks;

    size_t _pending = 0;

    std::vector<std::jthread> _workers;
    
    void WorkerLoop(std::stop_token st)
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock lock(_mutex);

                if (!_wake.wait(lock, st, [this] { return !_tasks.empty(); }))
                    return; // stop requested
                task = std::move(_tasks.front());
                _tasks.pop();
            }

            task();

            {
                std::scoped_lock lock(_mutex);
                if (--_pending == 0) _done.notify_all();
            }
        }
    }

};