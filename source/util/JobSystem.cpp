/**
 * \file
 * \author Sehoon Kim(sehoon.kim@digipen.edu)
 * \date 2026 Spring
 * \par CS250 Computer Graphics II
 * \copyright DigiPen Institute of Technology
 */

#include "JobSystem.hpp"

#include <algorithm>
#include <utility>

namespace util
{
    JobSystem::JobSystem()
    {
        #if defined(CAN_USE_THREADS)
            mThreadCount = std::thread::hardware_concurrency();

            if (mThreadCount == 0u)
            {
                mThreadCount = 1u;
            }

            const unsigned worker_count = (mThreadCount > 1u) ? (mThreadCount - 1u) : 0u;
            mWorkers.reserve(worker_count);

            for (unsigned i = 0; i < worker_count; ++i)
            {
                mWorkers.emplace_back(&JobSystem::WorkerLoop, this);
            }
            #else
                mThreadCount = 1u;
        #endif
    }

    JobSystem::~JobSystem()
    {
        #if defined(CAN_USE_THREADS)
            WaitUntilDone();

            mIsDone.store(true, std::memory_order_release);
            mJobAvailable.notify_all();

            for (auto& worker : mWorkers)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }
        #endif
    }

    void JobSystem::DoJob(Job job)
    {
        if (!job)
        {
            return;
        }

        #if defined(CAN_USE_THREADS)
            {
                std::lock_guard<std::mutex> lock(mJobsMutex);
                ++mJobsRemaining;
                mJobs.emplace(std::move(job));
            }

            mJobAvailable.notify_one();
        #else
            job();
        #endif
    }

    void JobSystem::WaitUntilDone()
    {
        #if defined(CAN_USE_THREADS)
            while (mJobsRemaining.load(std::memory_order_acquire) > 0)
            {
                Job job;
                if (TryGetJob(job))
                {
                    job();
                    --mJobsRemaining;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        #endif
    }

    bool JobSystem::IsDone() const
    {
        #if defined(CAN_USE_THREADS)
            return mJobsRemaining.load(std::memory_order_acquire) == 0;
        #else
            return true;
        #endif
    }

    void JobSystem::DoJobs(int how_many, ComputeAtIndex compute)
    {
        if (how_many <= 0 || !compute)
        {
            return;
        }

        #if defined(CAN_USE_THREADS)
            const unsigned threads_to_use = (mThreadCount == 0u) ? 1u : mThreadCount;
            const int      group_count    = static_cast<int>(threads_to_use);

            const int elements_per_group = how_many / group_count;
            int       remainder          = how_many % group_count;

            int start_index = 0;

            for (int group = 0; group < group_count; ++group)
            {
                int task_total = elements_per_group;

                if (remainder > 0)
                {
                    ++task_total;
                    --remainder;
                }

                if (task_total <= 0)
                {
                    continue;
                }

                const int begin = start_index;
                const int end   = start_index + task_total;
                start_index     = end;

                DoJob(
                    [begin, end, compute]()
                    {
                        for (int i = begin; i < end; ++i)
                        {
                            compute(i);
                        }
                    });
            }
        #else
            for (int i = 0; i < how_many; ++i)
            {
                compute(i);
            }
        #endif
    }

    bool JobSystem::TryGetJob(Job& out_job)
    {
        #if defined(CAN_USE_THREADS)
            std::lock_guard<std::mutex> lock(mJobsMutex);

            if (mJobs.empty())
            {
                return false;
            }

            out_job = std::move(mJobs.front());
            mJobs.pop();

            return true;
        #else
            static_cast<void>(out_job);
            return false;
        #endif
    }

    void JobSystem::WorkerLoop()
    {
        #if defined(CAN_USE_THREADS)
            for (;;)
            {
                Job job;

                {
                    std::unique_lock<std::mutex> lock(mJobsMutex);

                    mJobAvailable.wait(
                        lock,
                        [this]()
                        {
                            return mIsDone.load(std::memory_order_acquire) || !mJobs.empty();
                        });

                    if (mIsDone.load(std::memory_order_acquire) && mJobs.empty())
                    {
                        return;
                    }

                    if (!mJobs.empty())
                    {
                        job = std::move(mJobs.front());
                        mJobs.pop();
                    }
                }

                if (job)
                {
                    job();
                    --mJobsRemaining;
                }
            }
        #endif
    }
}
