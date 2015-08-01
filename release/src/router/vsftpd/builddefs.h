#ifndef VSF_BUILDDEFS_H
#define VSF_BUILDDEFS_H

#include <tomato_config.h>

#undef VSF_BUILD_TCPWRAPPERS
#undef VSF_BUILD_PAM

#ifdef TCONFIG_FTP_SSL
	#define VSF_BUILD_SSL
#else
	#undef VSF_BUILD_SSL
#endif


#endif /* VSF_BUILDDEFS_H */

