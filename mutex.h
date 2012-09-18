#ifndef __REFLECT_MUTEX_HEADER__
#define __REFLECT_MUTEX_HEADER__

#include <cassert>
#include <errno.h>
#include <pthread.h>

class Mutex
{
	private:
		pthread_mutex_t mutex_t;
		
	public:
		Mutex();
		~Mutex();
		
	public:
		void lock();
		bool trylock();
		void unlock();
};

////////////////////////////////////////////////////////////////////////////////

Mutex::Mutex()
{
	pthread_mutex_init(&this->mutex_t, NULL);
}
		
Mutex::~Mutex()
{
	pthread_mutex_destroy(&this->mutex_t);
}

void Mutex::lock()
{
	int rc = pthread_mutex_lock(&this->mutex_t);
	assert(rc == 0);
}

bool Mutex::trylock()
{
	int rc = pthread_mutex_trylock(&this->mutex_t);
	assert(rc == 0 or rc == EBUSY);
	return !rc;
}

void Mutex::unlock()
{
	int rc = pthread_mutex_unlock(&this->mutex_t);
	assert(rc == 0);
}

#endif
