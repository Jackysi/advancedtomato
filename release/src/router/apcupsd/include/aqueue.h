#ifndef __AQUEUE_H
#define __AQUEUE_H

#include <pthread.h>
#include <sys/time.h>
#include "alist.h"
#include "autil.h"

template<class T>
class aqueue
{
public:

   aqueue()
   {
      pthread_mutex_init(&_mutex, NULL);
      pthread_cond_init(&_condvar, NULL);
   }

   ~aqueue()
   {
      pthread_cond_destroy(&_condvar);
      pthread_mutex_destroy(&_mutex);
   }

   void enqueue(const T &elem)
   {
      pthread_mutex_lock(&_mutex);
      _queue.append(elem);
      pthread_mutex_unlock(&_mutex);
      pthread_cond_signal(&_condvar);
   }

   bool dequeue(T& elem, int msec = TIMEOUT_FOREVER)
   {
      int rc = 0;

      pthread_mutex_lock(&_mutex);
      if (msec != TIMEOUT_FOREVER) {
         struct timespec abstime;
         calc_abstimeout(msec, &abstime);
         while (rc == 0 && _queue.empty())
            rc = pthread_cond_timedwait(&_condvar, &_mutex, &abstime);
      } else {
         while (rc == 0 && _queue.empty())
            rc = pthread_cond_wait(&_condvar, &_mutex);
      }

      if (rc) {
         pthread_mutex_unlock(&_mutex);
         return false;
      }

      elem = _queue.first();
      _queue.remove_first();
      pthread_mutex_unlock(&_mutex);
      return true;
   }

   T dequeue()
   {
      pthread_mutex_lock(&_mutex);

      int rc = 0;
      while (rc == 0 && _queue.empty())
         rc = pthread_cond_wait(&_condvar, &_mutex);

      T elem = _queue.first();
      _queue.remove_first();
      pthread_mutex_unlock(&_mutex);
      return elem;
   }

   bool empty()
   {
      pthread_mutex_lock(&_mutex);
      bool tmp = _queue.empty();
      pthread_mutex_unlock(&_mutex);
      return tmp;
   }

   void clear()
   {
      pthread_mutex_lock(&_mutex);
      _queue.clear();
      pthread_mutex_unlock(&_mutex);
   }

private:

   static const int TIMEOUT_FOREVER = -1;
   pthread_mutex_t _mutex;
   pthread_cond_t _condvar;
   alist<T> _queue;

   // Prevent use
   aqueue(const aqueue<T> &rhs);
   aqueue<T> &operator=(const aqueue<T> &rhs);
};

#endif
