/*******************************************************************************
 * Copyright (c) 2013 matt@moregeek.com.tw
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 ********************************************************************************/

#ifndef _XPF_REFCNT_HEADER_
#define _XPF_REFCNT_HEADER_

#include "platform.h"
#include "compiler.h"

namespace xpf {

    /**
     *  A base class for objects whose life-cycle should be controlled by reference counting.
     *  RefCounted-derived objects born with ref count 1. Owners (except the one who new
     *  it) of such object should increase the reference count of it by calling ref(), and  
     *  decrease the reference count by unref() after done using. Object with a reference
     *  count 0 will be deleted before the return of unref().
     */
    class RefCounted
    {
    public:
        RefCounted()
            : mRefCount(1)
            , mDebugName(0)
        {
        }

        virtual ~RefCounted()
        {
        }

        virtual s32 ref() const
        {
            return ++mRefCount;
        }

        virtual bool unref() const
        {
            xpfAssert(mRefCount > 0);

            if (0 == (--mRefCount))
            {
                delete this;
                return true;
            }
            return false;
        }

        inline s32 getRefCount() const
        {
            return mRefCount;
        }

        inline const c8* getDebugName() const
        {
            return mDebugName;
        }

    protected:
        // For derived class to associate a name for
        // debugging purpose. Suggest to use a static
        // immutable c-string since the life cycle of
        // given string is not controlled by class.
        void setDebugName(const c8* name)
        {
            mDebugName = name;
        }

    private:
        mutable s32  mRefCount;
        const c8*    mDebugName;
    };



    /**
     *  A RefCounted-derived class with floating reference support.
     *  A floating reference means the object has been created with
     *  no owner. Whoever call ref() of this object at the first 
     *  place owns this object (the ref count won't increase, instead 
     *  the floating flag will be turn off). 
     *
     *  Ex: Class A creates a ref-counted object and than pass it to
     *      B and do unref() because A no more need to own the object.
     *
     *     With RefCounted obj, the code will be:
     *        RefCounted *obj = new MyObject;
     *        B.adopt(obj);  // B calls ref() on obj in adopt()
     *        obj->unref();
     *
     *     With FloatingRefCounted obj, the code can be shorten to:
     *        B.adopt(new MyFloatingRefObject);
     */
    class FloatingRefCounted : virtual public RefCounted
    {
    public:
        FloatingRefCounted()
            : mFloating(true)
        {
        }

        virtual ~FloatingRefCounted()
        {
        }

        virtual s32 ref() const
        {
            if ( xpfUnlikely(mFloating) )
            {
                mFloating = false;
                return getRefCount();
            }
            return RefCounted::ref();
        }

        virtual bool unref() const
        {
            mFloating = false;
            return RefCounted::unref();
        }

        inline bool isFloating() const
        {
            return mFloating;
        }

        inline void setFloating(bool f)
        {
            mFloating = f;
        }

    private:
        mutable bool mFloating;
    };

} // end of namespace xpf

#endif // _XPF_REFCNT_HEADER_