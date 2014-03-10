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

#include <xpf/netiomux.h>
#include <xpf/threadlock.h>
#include <deque>

namespace xpf
{

class NetIoMuxSyncFifo
{
public:
    NetIoMuxSyncFifo()
    {
    }

    ~NetIoMuxSyncFifo()
    {
    }

    void* pop_front(u32 & count)
    {
        ScopedThreadLock ml(mLock);
		if (mList.empty())
		{
			count = 0;
			return 0;
		}
        void* ret = mList.front();
        mList.pop_front();
        count = mList.size();
        return ret;
    }

    void push_back(void *data)
    {
        if (data)
        {
            ScopedThreadLock ml(mLock);
            mList.push_back(data);
        }
    }
	
	// Note: O(N)
	bool erase(void *data)
	{
		ScopedThreadLock ml(mLock);
		if (data == 0 || mList.empty())
			return false;
		for (std::deque<void*>::iterator it = mList.begin();
				it != mList.end(); ++it)
		{
			void *p = *it;
			if (p == data)
			{
				mList.erase(it);
				return true;
			}
		}
		return false;
	}

private:
    ThreadLock mLock;
    std::deque<void*> mList;
};

} // end of namespace xpf

