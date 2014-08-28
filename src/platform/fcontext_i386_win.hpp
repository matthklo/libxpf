
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_FCONTEXT_I386H
#define BOOST_CONTEXT_DETAIL_FCONTEXT_I386H

#include <xpf/platform.h>

#if defined(XPF_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable:4351)
#endif

namespace xpf {
namespace detail {

extern "C" {

struct stack_t
{
    void* sp;
    vptr  size;
    void* limit;

    stack_t() :
        sp( 0), size( 0), limit( 0)
    {}
};

struct fp_t
{
    u32     fc_freg[2];

    fp_t() :
        fc_freg()
    {}
};

struct fcontext_t
{
    u32     fc_greg[6];
    stack_t fc_stack;
    void*   fc_excpt_lst;
    void*   fc_local_storage;
    fp_t    fc_fp;
    u32     fc_dealloc;

    fcontext_t() :
        fc_greg(),
        fc_stack(),
        fc_excpt_lst( 0),
        fc_local_storage( 0),
        fc_fp(),
        fc_dealloc( 0)
    {}
};

}

}}

#if defined(XPF_COMPILER_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_CONTEXT_DETAIL_FCONTEXT_I386_H
