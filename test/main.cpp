#include <iostream>
#include <cassert>
#include <print>
#include "fixed_size_function.hpp"
#include "fixed_size_packaged_task.hpp"
#include "thread_pool.hpp"


void* operator new ( size_t size, const char* filename, int line )
{
    assert( false );
    void* ptr = new char[size];
    return ptr;
}


int main( void )
{
    // Fixed size function
    {
        int i = 0;
        FixedSizeFunction<int( int, int )> mul( [&]( int a, int b ) { return a * b + i; } );
        FixedSizeFunction<int( int, int )> sum( [&]( int a, int b ) { return a + b + i; } );

        assert( mul( 2, 3 ) == 6 );
        assert( sum( 2, 3 ) == 5 );
        mul.Swap( sum );
        assert( mul( 2, 3 ) == 5 );
        assert( sum( 2, 3 ) == 6 );
    }

    // Fixed size packaged task
    {
        std::future<int> future;
        {
            FixedSizePackagedTask<int( int, int )> mulTask( [&]( int a, int b ) { return a * b; } );
            future = mulTask.getFuture();
            mulTask( 2, 3 );
        }
        assert( future.get() == 6 );
    }

    // Thread pool
    {
        ThreadPool thread_pool;
        for ( size_t i = 0; i < 10; i++ )
        {
            thread_pool.AddTask( [i]()
            {
                std::println( "hello {}", i );
            } );
        }
        std::println( "End will be in destructor" );
    }
    
    //std::println( "\n\n\n" );
    //{
    //    ThreadPool thread_pool;

    //    std::vector<std::packaged_task<void(void)>> tasks;
    //    for ( size_t i = 0; i < 10; i++ )
    //    {
    //        std::packaged_task<void( void )> task( []()
    //        {
    //            std::cout << "hello future" << std::endl;
    //        } );
    //        tasks.push_back( task );
    //        thread_pool.AddTask( task );
    //    }
    //    for ( auto& task: tasks )
    //        task.get_future().get();
    //    std::cout << "\n future end \n";
    //}
    return 0;
}
