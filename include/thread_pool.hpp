#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <future>
#include <condition_variable>
#include "fixed_size_function.hpp"

template <size_t MaxFuncSize = 128>
class ThreadPool
{
public:
    ThreadPool( size_t threads = std::thread::hardware_concurrency() )
        : mIsRunning( true )
    {
        for ( int i = 0; i < threads; i++ )
        {
            mThreadsPool.emplace_back( std::thread( [&]()
            {
                while ( true )
                {
                    FixedSizeFunction<void( void ), MaxFuncSize> task;
                    {
                        std::unique_lock<std::mutex> lock( mEventMutex );

                        mConditionVariable.wait( lock, [&]()
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
        mIsRunning = false;
        mConditionVariable.notify_all();
        for ( std::thread& thread : mThreadsPool )
            thread.join();
    }

    template <typename T>
    void AddTask( T&& task )
    {
        {
            std::unique_lock<std::mutex> lock( mEventMutex );
            mTasks.push( std::forward<T>( task ) );
        }
        mConditionVariable.notify_one();
    }

    template <typename TasksType>
    void AddTasks( TasksType& tasks )
    {
        {
            std::unique_lock<std::mutex> lock( mEventMutex );
            for ( auto& task : tasks )
                mTasks.push( [&](){ task(); } );
        }
        mConditionVariable.notify_all();
    }

private:
    std::queue<FixedSizeFunction<void( void ), MaxFuncSize>> mTasks;
    std::condition_variable mConditionVariable;
    std::mutex mEventMutex;
    std::vector<std::thread> mThreadsPool;
    std::atomic<bool> mIsRunning;
};
