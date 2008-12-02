#include <linux/config.h>
#include <linux/kernprof.h>
#include <linux/module.h>

/* profiling function to call in timer interrupt */
prof_hook_p prof_timer_hook = 0;

/* profiling function to call in perf counter interrupt */
prof_hook_p prof_perfctr_hook = 0;

/* profiling function to call in scheduler upon switch-out */
mcount_hook_p prof_scheduler_hook = 0;

/* profiling function to call in scheduler upon wake-up */
wakeup_hook_p prof_wakeup_hook = 0;

/* profiling function called by mcount() */
mcount_hook_p mcount_hook = 0;

#ifdef CONFIG_FRAME_POINTER
int prof_have_frameptr = 1;
#else
int prof_have_frameptr = 0;
#endif
#ifdef CONFIG_MCOUNT
int prof_have_mcount = 1;
#else
int prof_have_mcount = 0;
#endif

/*
 * The following functions are defined so their names may appear in profiles.
 * They are not intended to be called.
 */
void USER(void) {}
void UNKNOWN_KERNEL(void) {}
void FIRMWARE(void) {}
void STALLED(void) {}
void SLEEPING(void) {}
void MODULE(void) {}

EXPORT_SYMBOL_NOVERS(USER);
EXPORT_SYMBOL_NOVERS(UNKNOWN_KERNEL);
EXPORT_SYMBOL_NOVERS(FIRMWARE);
EXPORT_SYMBOL_NOVERS(STALLED);
EXPORT_SYMBOL_NOVERS(SLEEPING);
EXPORT_SYMBOL_NOVERS(MODULE);
EXPORT_SYMBOL(prof_timer_hook);
EXPORT_SYMBOL(prof_perfctr_hook);
EXPORT_SYMBOL(prof_scheduler_hook);
EXPORT_SYMBOL(prof_wakeup_hook);
EXPORT_SYMBOL(mcount_hook);
EXPORT_SYMBOL_NOVERS(_stext);
EXPORT_SYMBOL_NOVERS(_etext);
EXPORT_SYMBOL(prof_have_frameptr);
EXPORT_SYMBOL(prof_have_mcount);
