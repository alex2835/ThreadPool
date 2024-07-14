#pragma once
#include <type_traits>
#include <array>

template <typename RetType, typename... Args>
class FunctionView<RetType( Args... )>
{
	using FunctionPtrType = RetType( * )( Args... );
	void* mPtr;
	m

public:
	FunctionView() = default;

	template <typename FUNC>
	FunctionView( FUNC&& object )
	{
		typedef typename std::remove_reference<FUNC>::type UnrefFunctionType;
		static_assert( sizeof( UnrefFunctionType ) < StorageSize, "functional object doesn't fit into internal storage" );
		static_assert( std::is_move_constructible<UnrefFunctionType>::value, "Type should be movable" );

	}

	FunctionView( const FunctionView& other )
	{
		mPtr = other.mPtr;
	}

	FunctionView& operator=( const FunctionView& other )
	{
		mPtr = other.mPtr;
	}

	~FunctionView()
	{
	}

	RetType operator()( Args&&... args )
	{
		if ( !mCollable )
			throw std::runtime_error( "call of empty functor" );
		return mCollable( mStorage, mFunctionPtr, std::forward<Args>( args )... );
	}

private:
	union
	{
		FunctionPtrType mFunctionPtr;
		char mStorage[StorageSize];
	};
	CollableType mCollable = nullptr;
	AllocType mAllocFunc = nullptr;
	DealocType mDealocFunc = nullptr;
};