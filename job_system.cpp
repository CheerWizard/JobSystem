#include "job_system.h"

#include <sstream>
#include <assert.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace engine { namespace core {

    JobSystem::JobSystem() {
        m_FinishedLabel.store(0);
        // detect thread count and feed thread pool
        m_ThreadCount = std::max(1u, std::thread::hardware_concurrency());
        for (u32 i = 0; i < m_ThreadCount; ++i) {
            std::thread worker([this] {
                std::function<void()> job;
                while (true) {
                    if (m_JobPool.popFront(job)) {
                        // execute job and update worker label state
                        job();
                        m_FinishedLabel.fetch_add(1);
                    } else {
                        // no job, put thread to sleep
                        std::unique_lock<std::mutex> lock(m_WakeMutex);
                        m_WakeCondition.wait(lock);
                    }
                }
            });

#ifdef _WIN32
            HANDLE handle = (HANDLE)worker.native_handle();
            // set thread into dedicated core
            DWORD_PTR affinityMask = 1ull << i;
            DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
            assert(affinity_result > 0);
            // set priority
            BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
            assert(priority_result != 0);
            // set thread name
            std::wstringstream wss;
            wss << "JobSystem_" << i;
            HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
            assert(SUCCEEDED(hr));
#endif // _WIN32

            worker.detach();
        }
    }

    void JobSystem::execute(const std::function<void()> &job) {
        m_CurrentLabel += 1;
        // try to push a new job until it is pushed
        while (!m_JobPool.pushBack(job)) {
            poll();
        }
        m_WakeCondition.notify_one();
    }

    void JobSystem::execute(u32 jobsPerThread, u32 jobSize, const std::function<void(JobArgs)> &job) {
        if (jobSize == 0 || jobsPerThread == 0) {
            return;
        }

        u32 jobGroups = (jobsPerThread + jobSize - 1) / jobSize;
        m_CurrentLabel += jobGroups;
        for (u32 i = 0; i < jobGroups; ++i) {
            // create single job from group
            const auto& jobGroup = [i, job, jobSize, jobsPerThread]() {

                u32 groupJobOffset = i * jobSize;
                u32 groupJobEnd = std::min(groupJobOffset + jobSize, jobsPerThread);

                JobArgs args;
                args.groupIndex = i;

                for (u32 j = groupJobOffset; j < groupJobEnd; ++j) {
                    args.index = j;
                    job(args);
                }
            };
            // try to push new group as a job until its pushed
            while (!m_JobPool.pushBack(jobGroup)) {
                poll();
            }
            m_WakeCondition.notify_one();
        }
    }

    bool JobSystem::isBusy() {
        return m_FinishedLabel.load() < m_CurrentLabel;
    }

    void JobSystem::wait() {
        while (isBusy()) {
            poll();
        }
    }

    void JobSystem::poll() {
        m_WakeCondition.notify_one();
        std::this_thread::yield();
    }

} }
