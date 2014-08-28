
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_FCONTEXT_X86_64_H
#define BOOST_CONTEXT_DETAIL_FCONTEXT_X86_64_H

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

struct fcontext_t
{
    u64     fc_greg[10];
    stack_t fc_stack;
    void*   fc_local_storage;
    u64     fc_fp[24];
    u64     fc_dealloc;

    fcontext_t() :
        fc_greg(),
        fc_stack(),
        fc_local_storage( 0),
        fc_fp(),
        fc_dealloc()
    {}
};

}

}}

#if defined(XPF_COMPILER_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_CONTEXT_DETAIL_FCONTEXT_X86_64_H
