#pragma once

class CThread 
{
public:
    template <typename Function, typename... Args>
    CThread(Function&& func, Args&&... args) : isDetached(false)
    {
        workerThread = std::thread(std::forward<Function>(func), std::forward<Args>(args)...);
    }

    ~CThread()
    {
        if (workerThread.joinable() && !isDetached)
        {
            workerThread.join();
        }
    }

    inline bool isJoinable() const
    {
        return workerThread.joinable();
    }

    inline void join()
    {
        workerThread.join();
    }

    // Detach the thread
    inline void detach()
    {
        workerThread.detach();
        isDetached = true;
    }

    CThread(const CThread&) = delete;
    CThread& operator=(const CThread&) = delete;

    CThread(CThread&& other) noexcept
    {
        workerThread = std::move(other.workerThread);
        isDetached = other.isDetached.exchange(true);
    }

    CThread& operator=(CThread&& other) noexcept
    {
        if (this != &other)
        {
            if (workerThread.joinable() && !isDetached)
            {
                workerThread.join();
            }
            workerThread = std::move(other.workerThread);
            isDetached = other.isDetached.exchange(true);
        }
        return *this;
    }

    static const uint32_t GetConCurrentThreads()
    {
        const uint32_t threadCount = std::thread::hardware_concurrency();
        return threadCount != 0 ? threadCount : 1; // Even if we have no count, at least 1 thread should exist else this pc wouldn't be running..
    }

private:
    std::thread workerThread;
    std::atomic<bool> isDetached;
};

class CParallelTask
{
public:
    CParallelTask(const uint32_t maxThreads) : maxConcurrentThreads(maxThreads) {}

    template <typename Function, typename... Args>
    void addTask(Function&& func, const uint32_t count, Args&&... args)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            auto task = [func = std::forward<Function>(func), args = std::make_tuple(std::forward<Args>(args)...)]() mutable
            {
                std::apply(func, args);
            };

            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(std::move(task));
        }
    }

    void execute()
    {
        for (uint32_t i = 0; i < maxConcurrentThreads; ++i)
        {
            threads.emplace_back([this]() { this->workerThread(); });
        }
    }

    void wait()
    {
        for (auto& thread : threads)
        {
            thread.join();
        }

        threads.clear();
    }

    const uint32_t getRemainingTasks()
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        return static_cast<uint32_t>(tasks.size());
    }

private:
    std::vector<CThread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    uint32_t maxConcurrentThreads;

    void workerThread()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if (!tasks.empty())
                {
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                else
                {
                    break;
                }
            }

            task();
        }
    }
};