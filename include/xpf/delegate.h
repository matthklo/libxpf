/********
 * Benefited from project ACF
 *    http://www.codeproject.com/Articles/11464/Yet-Another-C-style-Delegate-Class-in-Standard-C
 * 
 * A non thread-safe, header only, delegate implementation.
 *
 ********/

//-------------------------------------------------------------------------
//
// Copyright (C) 2004-2005 Yingle Jia
//
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied warranty, 
// and with no claim as to its suitability for any purpose.
//
// AcfDelegate.h
//

#ifndef __Acf_Delegate__
#define __Acf_Delegate__

#ifndef __XPF_PATCHED__
#define __XPF_PATCHED__
#endif

#include <stdexcept> // for std::logic_error
#include <utility> // for std::pair
#ifdef __XPF_PATCHED__
#include <string>
#endif

// Macros for template metaprogramming

#define ACF_JOIN(a, b)        ACF_DO_JOIN(a, b)
#define ACF_DO_JOIN(a, b)     ACF_DO_JOIN2(a, b)
#define ACF_DO_JOIN2(a, b)    a##b

#ifdef __XPF_PATCHED__
#define ACF_MAKE_PARAMS1_0(t)
#define ACF_MAKE_PARAMS1_1(t)    t##1
#define ACF_MAKE_PARAMS1_2(t)    t##1, t##2
#define ACF_MAKE_PARAMS1_3(t)    t##1, t##2, t##3
#define ACF_MAKE_PARAMS1_4(t)    t##1, t##2, t##3, t##4
#define ACF_MAKE_PARAMS1_5(t)    t##1, t##2, t##3, t##4, t##5
#define ACF_MAKE_PARAMS1_6(t)    t##1, t##2, t##3, t##4, t##5, t##6
#else
#define ACF_MAKE_PARAMS1_0(t)
#define ACF_MAKE_PARAMS1_1(t)    t##1
#define ACF_MAKE_PARAMS1_2(t)    t##1, ##t##2
#define ACF_MAKE_PARAMS1_3(t)    t##1, ##t##2, ##t##3
#define ACF_MAKE_PARAMS1_4(t)    t##1, ##t##2, ##t##3, ##t##4
#define ACF_MAKE_PARAMS1_5(t)    t##1, ##t##2, ##t##3, ##t##4, ##t##5
#define ACF_MAKE_PARAMS1_6(t)    t##1, ##t##2, ##t##3, ##t##4, ##t##5, ##t##6
#endif

#define ACF_MAKE_PARAMS2_0(t1, t2)
#define ACF_MAKE_PARAMS2_1(t1, t2)   t1##1 t2##1
#define ACF_MAKE_PARAMS2_2(t1, t2)   t1##1 t2##1, t1##2 t2##2
#define ACF_MAKE_PARAMS2_3(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3
#define ACF_MAKE_PARAMS2_4(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3, t1##4 t2##4
#define ACF_MAKE_PARAMS2_5(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3, t1##4 t2##4, t1##5 t2##5
#define ACF_MAKE_PARAMS2_6(t1, t2)   t1##1 t2##1, t1##2 t2##2, t1##3 t2##3, t1##4 t2##4, t1##5 t2##5, t1##6 t2##6

#define ACF_MAKE_PARAMS1(n, t)         ACF_JOIN(ACF_MAKE_PARAMS1_, n) (t)
#define ACF_MAKE_PARAMS2(n, t1, t2)    ACF_JOIN(ACF_MAKE_PARAMS2_, n) (t1, t2)

#ifdef __XPF_PATCHED__
namespace xpf {
#else
namespace Acf {
#endif

class InvalidCallException : public std::logic_error
{
public:
	InvalidCallException() : std::logic_error("An empty delegate is called")
	{
	}
};

#ifdef __XPF_PATCHED__
template <class T>
inline T _HandleInvalidCall()
{
	throw InvalidCallException();
}

template <>
inline void _HandleInvalidCall<void>()
{
}
#else
template <class T>
inline static T _HandleInvalidCall()
{
    throw InvalidCallException();
}

template <>
inline static void _HandleInvalidCall<void>()
{
}
#endif

template <class TSignature>
class Delegate; // no body

} // namespace Acf

// Specializations

#define ACF_DELEGATE_NUM_ARGS	0 // Delegate<R ()>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#define ACF_DELEGATE_NUM_ARGS	1 // Delegate<R (T1)>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#define ACF_DELEGATE_NUM_ARGS	2 // Delegate<R (T1, T2)>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#define ACF_DELEGATE_NUM_ARGS	3 // Delegate<R (T1, T2, T3)>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#define ACF_DELEGATE_NUM_ARGS	4 // Delegate<R (T1, T2, T3, T4)>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#define ACF_DELEGATE_NUM_ARGS	5 // Delegate<R (T1, T2, T3, T4, T5)>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#define ACF_DELEGATE_NUM_ARGS	6 // Delegate<R (T1, T2, T3, T4, T5, T6)>
#ifdef __XPF_PATCHED__
#include "delegate_template.h"
#else
#include "AcfDelegateTemplate.h"
#endif
#undef ACF_DELEGATE_NUM_ARGS

#undef __XPF_PATCHED__

#endif // #ifndef __Acf_Delegate__
