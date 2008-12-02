/* util.h ....... error message utilities.
 *                C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: util.h,v 1.1.1.1 2002/07/25 06:52:39 honor Exp $
 */

#ifndef INC_UTIL_H
#define INC_UTIL_H

void _log(char *func, char *file, int line, char *format, ...)
     __attribute__ ((format (printf, 4, 5)));
void _warn(char *func, char *file, int line, char *format, ...)
     __attribute__ ((format (printf, 4, 5)));
void _fatal(char *func, char *file, int line, char *format, ...)
     __attribute__ ((format (printf, 4, 5))) __attribute__ ((noreturn));

#define log(format, args...) \
	_log(__FUNCTION__,__FILE__,__LINE__, format , ## args)
#define warn(format, args...) \
	_warn(__FUNCTION__,__FILE__,__LINE__, format , ## args)
#define fatal(format, args...) \
	_fatal(__FUNCTION__,__FILE__,__LINE__, format , ## args)

#endif /* INC_UTIL_H */
