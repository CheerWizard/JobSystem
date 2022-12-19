#pragma once

#include <functional>
#include <algorithm>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <mutex>

typedef uint32_t u32;
typedef uint64_t u64;

namespace engine { namespace core {

    template<typename T, size_t capacity>
    class RingBuffer final {

    public:
        bool pushBack(const T& item);
        bool popFront(T& item);

    private:
        T m_Data[capacity];
        size_t m_Tail = 0;
        size_t m_Head = 0;
        std::mutex m_Lock;
    };

    template<typename T, size_t capacity>
    bool RingBuffer<T, capacity>::pushBack(const T &item) {
        bool pushed = false;
        std::lock_guard<std::mutex> lock(m_Lock);
        size_t next = (m_Head + 1) % capacity;
        if (next != m_Tail) {
            m_Data[m_Head] = item;
            m_Head = next;
            pushed = true;
        }
        return pushed;
    }

    template<typename T, size_t capacity>
    bool RingBuffer<T, capacity>::popFront(T &item) {
        bool popped = false;
        std::lock_guard<std::mutex> lock(m_Lock);
        if (m_Tail != m_Head) {
            item = m_Data[m_Tail];
            m_Tail = (m_Tail + 1) % capacity;
            popped = true;
        }
        return popped;
    }

    struct JobArgs {
        u32 index = 0;
        u32 groupIndex = 0;
    };

    class JobSystem final {

    private:
        JobSystem();

    public:
        static JobSystem& get() {
            static JobSystem instance;
            return instance;
        }
        // executes single job in a single worker thread
        void execute(const std::function<void()>& job);
        // executes multiple jobs with multiple amount of workers per job
        // jobSize - count of inner jobs inside single job
        // jobsPerThread - count of jobs for each worker thread
        void execute(u32 jobsPerThread, u32 jobSize, const std::function<void(JobArgs)>& job);

        inline bool isBusy();

        void wait();

    private:
        // wakes only one worker
        // allows calling thread to be rescheduled by OS
        inline void poll();

    private:
        u32 m_ThreadCount;
        RingBuffer<std::function<void()>, 256> m_JobPool;
        std::condition_variable m_WakeCondition;
        std::mutex m_WakeMutex;
        u64 m_CurrentLabel; // tracks state of execution of main thread
        std::atomic<u64> m_FinishedLabel; // tracks state of execution around worker threads
    };

} }