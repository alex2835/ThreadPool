#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <future>
#include <atomic>
#include <condition_variable>
#include "fixed_size_function.hpp"
#include "fixed_size_packaged_task.hpp"

template <size_t MaxFuncSize = 128>
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
                    FixedSizeFunction<void( void ), MaxFuncSize> task;
                    {
                        std::unique_lock<std::mutex> lock( mEventMutex );

                        mConditionVariable.wait( lock, [this]()
                        {
                            return !mIsRunning || !mTasks.empty();
                        } );

                        if ( !mIsRunning && mTasks.empty() )
                            break;

                        task = std::move( mTasks.front() );
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

    // Submits a zero-argument callable, returns a future for its result.
    // The task is fully owned by the pool after this call.
    template <typename FuncObj>
    auto AddTask( FuncObj&& func )
    {
        using Ret = std::invoke_result_t<std::decay_t<FuncObj>>;
        FixedSizePackagedTask<Ret()> task( std::forward<FuncObj>( func ) );
        auto future = task.getFuture();
        {
            std::unique_lock<std::mutex> lock( mEventMutex );
            mTasks.emplace( std::move( task ) );
        }
        mConditionVariable.notify_one();
        return future;
    }

private:
    std::queue<FixedSizeFunction<void( void ), MaxFuncSize>> mTasks;
    std::condition_variable mConditionVariable;
    std::mutex mEventMutex;
    std::vector<std::thread> mThreadsPool;
    std::atomic<bool> mIsRunning;
};
