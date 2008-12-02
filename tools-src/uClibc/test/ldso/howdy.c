#include <pthread.h>
#include <stdio.h>

extern int __pthread_return_0 (void);

int howdy(const char *s)
{
    return printf("howdy:  __pthread_return_0 = %p\n"
		  "howdy: pthread_cond_signal = %p\n",
		  __pthread_return_0, pthread_cond_signal);
}

void __attribute__((constructor)) howdy_ctor(void)
{
    printf("I am the libhowdy constructor!\n");
}

void __attribute__((destructor)) howdy_dtor(void)
{
    printf("I am the libhowdy destructor!\n");
}



