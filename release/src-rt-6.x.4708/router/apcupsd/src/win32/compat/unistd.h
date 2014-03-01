#ifndef HAVE_MINGW
#include "compat.h"
#else
#include_next<unistd.h>
#endif
