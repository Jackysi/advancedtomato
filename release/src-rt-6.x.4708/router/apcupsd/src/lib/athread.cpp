#include "athread.h"

const int athread::PRIORITY_INHERIT = -1;

bool athread::run()
{
   if (_running)
      return false;

   pthread_attr_t attr;
   pthread_attr_init(&attr);

   int inherit = PTHREAD_INHERIT_SCHED;
   if (_prio != PRIORITY_INHERIT) {
      struct sched_param param;
      param.sched_priority = _prio;
      pthread_attr_setschedparam(&attr, &param);
      inherit = PTHREAD_EXPLICIT_SCHED;
   }

   pthread_attr_setinheritsched(&attr, inherit);

   int rc = pthread_create(&_tid, &attr, &athread::springboard, this);
   if (rc == 0)
      _running = true;

   pthread_attr_destroy(&attr);
   return _running;
}

void *athread::springboard(void *arg)
{
   athread *_this = (athread*)arg;
   _this->body();
   _this->_running = false;
   return NULL;
}

bool athread::join()
{
   return pthread_join(_tid, NULL) == 0;
}
