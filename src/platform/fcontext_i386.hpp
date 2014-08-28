
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_FCONTEXT_I386H
#define BOOST_CONTEXT_DETAIL_FCONTEXT_I386H

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

struct fcontext_t
{
    u32     fc_greg[6];
    stack_t fc_stack;
    u32     fc_freg[2];

    fcontext_t() :
        fc_greg(),
        fc_stack(),
        fc_freg()
    {}
};

}

}}

#endif // BOOST_CONTEXT_DETAIL_FCONTEXT_I386_H
