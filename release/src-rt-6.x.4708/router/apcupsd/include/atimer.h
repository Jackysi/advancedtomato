#ifndef __ATIMER_H
#define __ATIMER_H

#include <pthread.h>
#include "athread.h"

class atimer: public athread
{
public:

   class client
   {
   public:
      virtual void HandleTimeout(int id) = 0;
   protected:
      client() {}
      virtual ~client() {}
   };

   atimer(client &cli, int id = 0);
   ~atimer();

   void start(unsigned long msec);
   void stop();

private:

   virtual void body();

   client &_client;
   int _id;
   pthread_mutex_t _mutex;
   pthread_cond_t _condvar;
   bool _started;
   struct timespec _abstimeout;

   // Prevent use
   atimer(const atimer &rhs);
   atimer &operator=(const atimer &rhs);
};

#endif // __ATIMER_H
