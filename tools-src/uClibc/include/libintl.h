/* Message catalog support for internationalization is not currently
 * provided by uClibc, and so I have added macros here to disable it.
 * Sorry about that.
 */

#ifndef _LIBINTL_H
#define _LIBINTL_H	1

#undef bindtextdomain
#define bindtextdomain(Domain, Directory) /* empty */
#undef textdomain
#define textdomain(Domain) /* empty */
#define _(Text) (Text)
#define N_(Text) (Text)


#endif /* _LIBINTL_H */

