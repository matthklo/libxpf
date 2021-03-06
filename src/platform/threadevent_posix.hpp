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

#include <xpf/threadevent.h>

#ifdef _XPF_THREADEVENT_IMPL_INCLUDED_
#error Multiple ThreadEvent implementation files included
#else
#define _XPF_THREADEVENT_IMPL_INCLUDED_
#endif

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

namespace xpf { namespace details {

	class XpfThreadEvent
	{
	public:
		explicit XpfThreadEvent(bool set /* = false */)
			: m_set(false)
		{
			pthread_mutexattr_t mattr;
			pthread_mutexattr_init(&mattr);
			pthread_mutex_init(&m_lock, &mattr);
			pthread_cond_init(&m_ready, NULL);

			if (set)
				this->set();
		}

		~XpfThreadEvent()
		{
			pthread_mutex_destroy(&m_lock);
			pthread_cond_destroy(&m_ready);
		}

		inline void set()
		{
			pthread_mutex_lock(&m_lock);
			m_set = true;
			pthread_cond_signal(&m_ready);
			pthread_mutex_unlock(&m_lock);
		}

		inline void reset()
		{
			pthread_mutex_lock(&m_lock);
			m_set = false;
			pthread_cond_destroy(&m_ready);
			pthread_cond_init(&m_ready, NULL);
			pthread_mutex_unlock(&m_lock);
		}

		inline bool wait(u32 timeoutMs /* = -1 */)
		{
			if (-1 == timeoutMs)
			{
				pthread_mutex_lock(&m_lock);
				if (!m_set)
					pthread_cond_wait(&m_ready, &m_lock);
				pthread_mutex_unlock(&m_lock);
				return true;
			}

			// Timed wait
			bool ret = false;
			if (m_set)
			{
				ret = true;
			}
			else
			{
				pthread_mutex_lock(&m_lock);

				struct timeval  tvnow, tvadv, tvres; // secs, microsecs (10^-6)
				struct timespec ts; // secs, nanosecs (10^-9)

				gettimeofday(&tvnow, NULL);

				tvadv.tv_sec = (timeoutMs / 1000);
				tvadv.tv_usec = (timeoutMs % 1000) * 1000;

				timeradd(&tvnow, &tvadv, &tvres);

				// convert timeval to timespec.
				ts.tv_sec = tvres.tv_sec;
				ts.tv_nsec = tvres.tv_usec * 1000;

				if ( 0 == pthread_cond_timedwait(&m_ready, &m_lock, &ts) )
					ret = true;

				pthread_mutex_unlock(&m_lock);
			}
			return ret;
		}

		inline bool isSet()
		{
			return m_set;
		}

	private:
		XpfThreadEvent(const XpfThreadEvent& that)
		{
			xpfAssert( ( "non-copyable object", false ) );
		}

		XpfThreadEvent& operator = (const XpfThreadEvent& that)
		{
			xpfAssert( ( "non-copyable object", false ) );
			return *this;
		}

		pthread_cond_t  m_ready;
		pthread_mutex_t m_lock;
		bool            m_set;
	};

};};
