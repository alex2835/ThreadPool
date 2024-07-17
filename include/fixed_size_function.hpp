﻿#pragma once
#include <array>
#include <type_traits>

template <typename Signature, size_t StorageSize = 64>
class FixedSizeFunction;

template <typename Ret, typename... Args, size_t StorageSize>
class FixedSizeFunction<Ret( Args... ), StorageSize>
{
	using StorageType = std::array<unsigned char, StorageSize>;
	using FunctionPtrType = Ret( * )( Args... );
	using CallableType = Ret( * )( StorageType& funcObj, FunctionPtrType func, Args&&... args );
	using AllocationFunctionType = void ( * )( StorageType& storage, void* Callable );
	using DeallocationFunctionType = void ( * )( StorageType& storage );

public:
	FixedSizeFunction() = default;
	FixedSizeFunction( FixedSizeFunction& ) = delete;
	FixedSizeFunction& operator=( FixedSizeFunction& ) = delete;

	/**
	 * @brief FixedSizeFunction Constructor from functional object.
	 * @param object Functor object will be stored in the internal storage
	 * using move constructor. Unmovable objects are prohibited explicitly.
	 */
	template <typename FuncObj>
	FixedSizeFunction( FuncObj&& funcObj )
	{
        typedef typename std::remove_reference<FuncObj>::type UnrefFunctionType;
		static_assert( sizeof( UnrefFunctionType ) < StorageSize, "functional object doesn't fit into internal storage" );
		static_assert( std::is_move_constructible<UnrefFunctionType>::value, "Type should be movable" );

		mCallable = []( StorageType& funcObjStorage, FunctionPtrType /*func*/, Args&&... args ) -> Ret
		{
			if constexpr( std::is_same_v<Ret, void> )
				reinterpret_cast<UnrefFunctionType*>( funcObjStorage.data() )->operator()( std::forward<Args>( args )... );
			else
				return reinterpret_cast<UnrefFunctionType*>( funcObjStorage.data() )->operator()( std::forward<Args>( args )... );
		};

		mAllocationFunction = []( StorageType& storage, void* funcObj )
		{
			UnrefFunctionType* functionalObject = reinterpret_cast<UnrefFunctionType*>( funcObj );
			::new( storage.data() ) UnrefFunctionType( std::forward<UnrefFunctionType>( *functionalObject ) );
		};

		mDeallocationFunction = []( StorageType& storage )
		{
			reinterpret_cast<UnrefFunctionType*>( storage.data() )->~UnrefFunctionType();
		};

		mAllocationFunction( mStorage, &funcObj );
	}

	/**
	 * @brief FixedSizeFunction Constructor from function pointer
	 */
	template <typename FuncRet, typename... FuncArgs>
	FixedSizeFunction( FuncRet( *func )( FuncArgs... ) )
	{
		std::println( "func func constructor" );
		mFunctionPtr = func;
		mCallable = []( StorageType& /*funcObjStorage*/, FunctionPtrType func, Args&&... args ) -> Ret
		{
			return reinterpret_cast<FuncRet( * )( FuncArgs... )>( func )( std::forward<Args>( args )... );
		};
	}

    FixedSizeFunction( FixedSizeFunction&& other ) noexcept
    {
        swap( other );
    }
	
	FixedSizeFunction& operator=( FixedSizeFunction&& other ) noexcept
	{
		swap( other );
		return *this;
	}

	~FixedSizeFunction()
	{
		if ( mDeallocationFunction )
			mDeallocationFunction( mStorage );
	}

    void swap( FixedSizeFunction& other )
    {
        std::swap( mFunctionPtr, other.mFunctionPtr );
        std::swap( mStorage, other.mStorage );
        std::swap( mCallable, other.mCallable );
        std::swap( mDeallocationFunction, other.mDeallocationFunction );
        std::swap( mAllocationFunction, other.mAllocationFunction );
    }

    Ret operator()( Args&&... args )
    {
        if ( not mCallable )
            throw std::runtime_error( "Callable not set" );
        return mCallable( mStorage, mFunctionPtr, std::forward<Args>( args )... );
    }

private:
    CallableType mCallable = nullptr;
	// Pure function
	FunctionPtrType mFunctionPtr = nullptr;
	// Functional object
	StorageType mStorage;
	AllocationFunctionType mAllocationFunction = nullptr;
	DeallocationFunctionType mDeallocationFunction = nullptr;
};
