#ifndef __ATHREAD_H
#define __ATHREAD_H

#include <pthread.h>
 
class athread
{
public:

   athread(int prio = PRIORITY_INHERIT)
      : _prio(prio),
        _running(false) {}

   virtual ~athread() {}

   virtual bool run();
   virtual bool join();

protected:

   virtual void body() = 0;

   static void *springboard(void *arg);

   static const int PRIORITY_INHERIT;

   pthread_t _tid;
   int _prio;
   bool _running;
};

#endif
