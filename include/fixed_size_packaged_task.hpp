#pragma once
#include <condition_variable>
#include <mutex>
#include <type_traits>
#include <utility>
#include "fixed_size_function.hpp"

template <typename Signature, size_t StorageSize = 64>
class FixedSizePackagedTask;

template <typename Ret, typename... Args, size_t StorageSize>
class FixedSizePackagedTask<Ret( Args... ), StorageSize>
{
public:
    FixedSizePackagedTask() = delete;
    FixedSizePackagedTask( const FixedSizePackagedTask& ) = delete;
    FixedSizePackagedTask& operator=( const FixedSizePackagedTask& ) = delete;
    FixedSizePackagedTask( FixedSizePackagedTask&& ) = delete;
    FixedSizePackagedTask& operator=( FixedSizePackagedTask&& ) = delete;
    ~FixedSizePackagedTask() = default;

    template <typename FuncObj,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<FuncObj>, FixedSizePackagedTask>>>
    FixedSizePackagedTask( FuncObj&& func )
        : mFunction( std::forward<FuncObj>( func ) )
    {}

    void operator()( Args... args )
    {
        mResult = mFunction( std::forward<Args>( args )... );
        {
            std::lock_guard<std::mutex> lock( mMutex );
            mDone = true;
        }
        mCondVar.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock( mMutex );
        mCondVar.wait( lock, [this]{ return mDone; } );
    }

    const Ret& get()
    {
        wait();
        return mResult;
    }

    const Ret& getResult() const noexcept { return mResult; }
    bool isDone() const noexcept
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mDone;
    }

private:
    FixedSizeFunction<Ret( Args... ), StorageSize> mFunction;
    Ret mResult{};
    mutable std::mutex mMutex;
    std::condition_variable mCondVar;
    bool mDone = false;
};

template <typename... Args, size_t StorageSize>
class FixedSizePackagedTask<void( Args... ), StorageSize>
{
public:
    FixedSizePackagedTask() = delete;
    FixedSizePackagedTask( const FixedSizePackagedTask& ) = delete;
    FixedSizePackagedTask& operator=( const FixedSizePackagedTask& ) = delete;
    FixedSizePackagedTask( FixedSizePackagedTask&& ) = delete;
    FixedSizePackagedTask& operator=( FixedSizePackagedTask&& ) = delete;
    ~FixedSizePackagedTask() = default;

    template <typename FuncObj,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<FuncObj>, FixedSizePackagedTask>>>
    FixedSizePackagedTask( FuncObj&& func )
        : mFunction( std::forward<FuncObj>( func ) )
    {}

    void operator()( Args... args )
    {
        mFunction( std::forward<Args>( args )... );
        {
            std::lock_guard<std::mutex> lock( mMutex );
            mDone = true;
        }
        mCondVar.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock( mMutex );
        mCondVar.wait( lock, [this]{ return mDone; } );
    }

    void get() { wait(); }

    bool isDone() const noexcept
    {
        std::lock_guard<std::mutex> lock( mMutex );
        return mDone;
    }

private:
    FixedSizeFunction<void( Args... ), StorageSize> mFunction;
    mutable std::mutex mMutex;
    std::condition_variable mCondVar;
    bool mDone = false;
};
