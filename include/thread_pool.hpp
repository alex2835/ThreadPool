#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include "function_view.hpp"
#include "fixed_size_packaged_task.hpp"

class ThreadPool
{
public:
    explicit ThreadPool( size_t threads = std::thread::hardware_concurrency() )
        : mIsRunning( true )
    {
        if ( threads == 0 )
            threads = 1;

        for ( size_t i = 0; i < threads; i++ )
        {
            mThreadsPool.emplace_back( std::thread( [this]()
            {
                while ( true )
                {
                    FunctionView<void()> task;
                    {
                        std::unique_lock<std::mutex> lock( mEventMutex );

                        mConditionVariable.wait( lock, [this]()
                        {
                            return !mIsRunning || !mTasks.empty();
                        } );

                        if ( !mIsRunning && mTasks.empty() )
                            break;

                        task = mTasks.front();
                        mTasks.pop();
                    }
                    task();
                }
            } ) );
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock( mEventMutex );
            mIsRunning = false;
        }
        mConditionVariable.notify_all();
        for ( std::thread& thread : mThreadsPool )
        {
            if ( thread.joinable() )
                thread.join();
        }
    }

    // Fork: enqueue a non-owning view of a packaged task.
    // The caller owns the task and must keep it alive until task.wait()/get() returns.
    template <typename Ret, size_t StorageSize>
    void AddTask( FixedSizePackagedTask<Ret(), StorageSize>& task )
    {
        {
            std::unique_lock<std::mutex> lock( mEventMutex );
            mTasks.emplace( task );
        }
        mConditionVariable.notify_one();
    }

private:
    std::queue<FunctionView<void()>> mTasks;
    std::condition_variable mConditionVariable;
    std::mutex mEventMutex;
    std::vector<std::thread> mThreadsPool;
    std::atomic<bool> mIsRunning;
};
