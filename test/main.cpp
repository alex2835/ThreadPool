#include <cassert>
#include <print>
#include "fixed_size_function.hpp"
#include "fixed_size_packaged_task.hpp"
#include "thread_pool.hpp"

int mulFunc( int x, int y )
{
    return x * y;
}


int main( void )
{
    // Fixed size function
    {
        int i = 0;
        FixedSizeFunction<int( int, int )> mul( [&]( int a, int b ){ return a * b + i; } );
        FixedSizeFunction<int( int, int )> sum( [&]( int a, int b ){ return a + b + i; } );

        assert( mul( 2, 3 ) == 6 );
        assert( sum( 2, 3 ) == 5 );
        std::swap( mul, sum );
        assert( mul( 2, 3 ) == 5 );
        assert( sum( 2, 3 ) == 6 );
    }
    {
        FixedSizeFunction<int( int, int )> mul( []( int a, int b ){ return a * b; } );
        assert( mul( 2, 3 ) == 6 );
    }
    {
        FixedSizeFunction<int( int, int )> mul( mulFunc );
        assert( mul( 2, 3 ) == 6 );
    }


    // Fixed size packaged task
    {
        FixedSizePackagedTask<int( int, int )> mulTask( []( int a, int b ){ return a * b; } );
        mulTask( 2, 3 );
        assert( mulTask.getResult() == 6 );
    }
    {
        FixedSizePackagedTask<int( int, int )> mulTask( mulFunc );
        mulTask( 2, 3 );
        assert( mulTask.getResult() == 6 );
    }


    // Thread pool — caller owns tasks (stable address), waits via the task itself.
    {
        ThreadPool threadPool;

        FixedSizePackagedTask<int()> t1( [](){ std::println( "task 1 thread:{}", std::this_thread::get_id() ); return 10; } );
        FixedSizePackagedTask<int()> t2( [](){ std::println( "task 2 thread:{}", std::this_thread::get_id() ); return 20; } );
        FixedSizePackagedTask<int()> t3( [](){ std::println( "task 3 thread:{}", std::this_thread::get_id() ); return 30; } );

        threadPool.AddTask( t1 );
        threadPool.AddTask( t2 );
        threadPool.AddTask( t3 );

        assert( t1.get() == 10 );
        assert( t2.get() == 20 );
        assert( t3.get() == 30 );

        std::println( "tasks joined, sum = {}", t1.getResult() + t2.getResult() + t3.getResult() );
    }

    std::println( "\nFixedSizeFunction size {}", sizeof( FixedSizeFunction<void( void )> ) );
    std::println( "FixedSizePackagedTask size {}\n", sizeof( FixedSizePackagedTask<void( void )> ) );

    return 0;
}
