/***
 * ensure new[] and delete[] are being used
 */

#include <cstddef>
 
#ifndef TSTACK_USE_NEWDELETE_OPS_H
#define TSTACK_USE_NEWDELETE_OPS_H

namespace sneaky_use_newdelete_ops {
// class-specific allocation functions --- lifted from cppreference.com
struct sneaky_t {
    static void* operator new(std::size_t count)
    {
		non_array_calls++;
        return ::operator new(count==0?1:count);
    }
    static void operator delete (void* ptr)
    {
		non_array_calls++;
		::operator delete(ptr);
    }
  
    static void* operator new[](std::size_t count)
    {
		new_calls++;
		if( !last_allocation_size ) {
			last_allocation_size = count;
		} else {
			double this_growth_factor = count/double(last_allocation_size);
			max_growth_factor = this_growth_factor>max_growth_factor ? this_growth_factor : max_growth_factor;
		}
		/***
		 * some platforms misbehave on zero byte requests it seems
		 */
        return ::operator new[](count==0?1:count);
    }
	static void operator delete[](void* ptr, std::size_t size)
	{
		delete_calls++;
		return ::operator delete[](ptr,size);
	}
	static size_t new_calls;
	static size_t delete_calls;
	static size_t non_array_calls;
	static size_t last_allocation_size;
	static double max_growth_factor;
	static void reset() 
	{
		new_calls = delete_calls = non_array_calls = last_allocation_size = 0;
		max_growth_factor = 0;
	}
};

static inline std::ostream& operator<<( std::ostream& os, const sneaky_t& )
{
	return os << "st" ;
}

size_t sneaky_t::new_calls;
size_t sneaky_t::delete_calls;
size_t sneaky_t::non_array_calls;
size_t sneaky_t::last_allocation_size;
double sneaky_t::max_growth_factor;

} // namespace


TEST(ProjectRequirements, UsesNewDeleteOps) {
	using namespace sneaky_use_newdelete_ops;
	const size_t calls[] = {2,4,7,12,14,21};

	for( size_t c=0; c<sizeof(calls)/sizeof(size_t); ++c ) {
		sneaky_t::reset();
		stack<sneaky_t> stk;

		// bitshift is exponentiation with a base of 2
		// 1<<calls[c] == 2**calls[c]
		const int N = (1<<calls[c]) - 1;
		for( int i=0; i<N; ++i ) {
			stk.push( sneaky_t() );
		}
		// must be at least one new[] call
		ASSERT_GE(sneaky_t::new_calls,1 );
		// new() and delete (non [] versions) should not be used
		ASSERT_EQ(sneaky_t::non_array_calls,0);
	}
}


TEST(ProjectRequirements, DontUseMoreThan32AsYourBaseCapacity) {
	using namespace sneaky_use_newdelete_ops;
	/***
	 * "base capacity" is the size of array allocated in your default
	 * constructor
	 */
	sneaky_t::reset();
	stack<sneaky_t> stk;

	const int N = 33;
	for( int i=0; i<N; ++i ) {
		stk.push( sneaky_t() );
	}
	// must be at least two new[] calls (and less than N)
	ASSERT_GE(sneaky_t::new_calls,2 );
	// must be less than N new[] calls (don't expand the stack store on each push) 
	ASSERT_LT(sneaky_t::new_calls,33 );
}


TEST(ProjectRequirements, DeletesShouldBeOneLessThanNews ) {
	using namespace sneaky_use_newdelete_ops;
	/***
	 * If you are getting tripped up by this test then you have memory
	 * management issues in your stack expansion logic (or you've implemented
	 * an inefficient solution).
	 */
	const size_t calls[] = {2,4,7,12,14,21};
	for( size_t c=0; c<sizeof(calls)/sizeof(size_t); ++c ) {
		sneaky_t::reset();
		stack<sneaky_t> stk;

		// bitshift is exponentiation with a base of 2
		// 1<<calls[c] == 2**calls[c]
		const int N = (1<<calls[c]) - 1;
		for( int i=0; i<N; ++i ) {
			stk.push( sneaky_t() );
		}
		// there should be one less delete[] than new[]
		ASSERT_EQ(sneaky_t::delete_calls + 1,sneaky_t::new_calls);
	}
}



//TEST(ProjectRequirements, GrowthFactorTooLarge ) {
//	using namespace sneaky_use_newdelete_ops;
//	/***
//	 * Then at some point in your stack expansion logic you are growing your
//	 * array by way too much.  1.5x, 2x, 3x, 4x ... is OK, but above 8x is a
//	 * little too much.
//	 */
//	const size_t calls[] = {2,4,7,12,14,21};
//	for( size_t c=0; c<sizeof(calls)/sizeof(size_t); ++c ) {
//		sneaky_t::reset();
//		stack<sneaky_t> stk;
//
//		// bitshift is exponentiation with a base of 2
//		// 1<<calls[c] == 2**calls[c]
//		const int N = (1<<calls[c]) - 1;
//		for( int i=0; i<N; ++i ) {
//			stk.push( sneaky_t() );
//		}
//		// growth factor should not be more than 8
//		ASSERT_LE(sneaky_t::max_growth_factor,8.0);
//	}
//}
//

#endif
