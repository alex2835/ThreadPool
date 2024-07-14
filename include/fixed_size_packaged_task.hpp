#pragma once
#include <type_traits>
#include <array>
#include <future>
#include "fixed_size_function.hpp"

template <typename Signature, size_t StorageSize = 64>
class FixedSizePackagedTask;

template <typename Ret, typename... Args, size_t StorageSize>
class FixedSizePackagedTask<Ret( Args... ), StorageSize>
{
public:
    template <typename FuncObj>
    FixedSizePackagedTask( FuncObj&& func )
        : mFunction( [&]( Args&&... args )
          {
             try
             {
                 if constexpr ( std::is_void_v<std::invoke_result_t<FuncObj, Args...>> )
                     mPromise.set_value();
                 else
                    mPromise.set_value( func( std::forward<Args>( args )... ) );
             }
             catch ( std::exception_ptr e )
             {
                 mPromise.set_exception( e );
             }
          } )
    {}

    std::future<Ret> getFuture()
    {
        return mPromise.get_future();
    }

    void operator()( Args&&... args )
    {
        mFunction( std::forward<Args>( args )... );
    }

private:
    FixedSizeFunction<void( Args... ), StorageSize> mFunction;
    std::promise<Ret> mPromise;
};