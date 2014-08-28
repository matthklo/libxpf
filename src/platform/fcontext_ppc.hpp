
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_FCONTEXT_PPC_H
#define BOOST_CONTEXT_DETAIL_FCONTEXT_PPC_H

#include <xpf/platform.h>

namespace xpf {
namespace detail {

extern "C" {

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
    u64     fc_freg[19];

    fp_t() :
        fc_freg()
    {}
};

struct fcontext_t
{
# if defined(__powerpc64__)
    u64     fc_greg[23];
# else
    u32     fc_greg[23];
# endif
    stack_t             fc_stack;
    fp_t                fc_fp;

    fcontext_t() :
        fc_greg(),
        fc_stack(),
        fc_fp()
    {}
};

}

}}

#endif // BOOST_CONTEXT_DETAIL_FCONTEXT_PPC_H
