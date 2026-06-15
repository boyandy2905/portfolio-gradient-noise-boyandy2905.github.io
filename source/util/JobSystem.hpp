/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include "util/Environment.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace util
{
    class JobSystem
    {
    public:
        using Job            = std::function<void(void)>;
        using ComputeAtIndex = std::function<void(int)>;

        JobSystem();
        ~JobSystem();

        JobSystem(const JobSystem&)            = delete;
        JobSystem(JobSystem&&)                 = delete;
        JobSystem& operator=(const JobSystem&) = delete;
        JobSystem& operator=(JobSystem&&)      = delete;

        void DoJob(Job job);
        void WaitUntilDone();
        bool IsDone() const;

        void DoJobs(int how_many, ComputeAtIndex compute);

    private:
        void WorkerLoop();
        bool TryGetJob(Job& out_job);

    private:
        std::vector<std::thread> mWorkers{};
        std::queue<Job>          mJobs{};
        std::mutex               mJobsMutex{};
        std::condition_variable  mJobAvailable{};
        std::atomic<bool>        mIsDone{ false };
        std::atomic<int>         mJobsRemaining{ 0 };
        unsigned                 mThreadCount = 1;
    };
}