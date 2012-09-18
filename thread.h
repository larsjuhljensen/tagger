#ifndef __REFLECT_THREAD_HEADER__
#define __REFLECT_THREAD_HEADER__

#include <cassert>
#include <pthread.h>

class IThread
{
	public:
		virtual void start() = 0;
		virtual void join() = 0;
		virtual void run() = 0;
};

class Thread : public IThread
{
	private:
		pthread_t posix_thread;
	
	private:
		static void* wrap_run(void* object);
		
	public:
		void start();
		void join();
		virtual void run();
};

////////////////////////////////////////////////////////////////////////////////

void* Thread::wrap_run(void* object)
{
	((IThread*)object)->run();
	return NULL;
}

void Thread::start()
{
	int rc = pthread_create(&(this->posix_thread), NULL, Thread::wrap_run, (void*)this);
	assert(rc == 0);
}

void Thread::join()
{
	void* status;
	int rc = pthread_join(this->posix_thread, &status);
	assert(rc == 0);
}

void Thread::run()
{
}

#endif
