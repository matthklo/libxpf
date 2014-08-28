//          Copyright Martin Husemann 2012
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CTX_DETAIL_FCONTEXT_SPARC_H
#define BOOST_CTX_DETAIL_FCONTEXT_SPARC_H

#include <xpf/platform.h>

namespace xpf {
namespace detail {

extern "C" {

// if defined(_LP64) we are compiling for sparc64, otherwise it is 32 bit
// sparc.


struct stack_t
{
    void* sp;
    vptr  size;

    stack_t() :
        sp( 0), size( 0)
    {}
};

struct fp_t
{
#ifdef _LP64
    u64 fp_freg[32];
    u64	fp_fprs, fp_fsr;
#else
    u64 fp_freg[16];
    u32	fp_fsr;
#endif

    fp_t() :
        fp_freg(),
#ifdef _LP64
	fp_fprs(),
#endif
	fp_fsr()
    {}
}
#ifdef _LP64
		 __attribute__((__aligned__(64)))	// allow VIS instructions to be used
#endif
;

struct fcontext_t
{
    fp_t                fc_fp;	// fpu stuff first, for easier alignement
#ifdef _LP64
    u64
#else
    u32
#endif
			fc_greg[8];
    stack_t             fc_stack;

    fcontext_t() :
        fc_fp(),
        fc_greg(),
        fc_stack()
    {}
};

}

}}

#endif // BOOST_CTX_DETAIL_FCONTEXT_SPARC_H
