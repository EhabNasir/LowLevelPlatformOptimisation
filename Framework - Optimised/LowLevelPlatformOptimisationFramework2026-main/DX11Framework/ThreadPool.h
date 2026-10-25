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
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency() - 1)
    {
        for (size_t i = 0; i < threadCount; ++i)
            _workers.emplace_back([this](std::stop_token st) { WorkerLoop(st); });
    }

    ~ThreadPool()
    {
        for (auto& w : _workers) w.request_stop();
        _wake.notify_all();            // workers exit; jthread joins automatically
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

    // Blocks (sleeping, not spinning) until every submitted task has finished
    void Wait()
    {
        std::unique_lock lock(_mutex);
        _done.wait(lock, [this] { return _pending == 0; });
    }

    size_t ThreadCount() const { return _workers.size(); }

private:
    void WorkerLoop(std::stop_token st)
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock lock(_mutex);
                // Thread sleeps here when there's no work: no CPU thrashing
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

    std::mutex _mutex;
    std::condition_variable_any _wake;   // _any is needed for stop_token support
    std::condition_variable _done;
    std::queue<std::function<void()>> _tasks;
    size_t _pending = 0;
    std::vector<std::jthread> _workers;  // declared last so it's destroyed first
};