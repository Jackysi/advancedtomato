/* inststr.c ... stolen from bdupdate.c, which stole it from perl 4.
 *               Theft by C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: inststr.c,v 1.1.1.1 2002/07/25 06:52:39 honor Exp $
 */

#include <string.h>

void
inststr(int argc, char **argv, char **environ, char *src)
{
    if (strlen(src) <= strlen(argv[0]))
    {
        char *ptr;

        for (ptr = argv[0]; *ptr; *(ptr++) = '\0');

        strcpy(argv[0], src);
    } else
    {
        /* stolen from the source to perl 4.036 (assigning to $0) */
        char *ptr, *ptr2;
        int count;
        ptr = argv[0] + strlen(argv[0]);
        for (count = 1; count < argc; count++) {
               if (argv[count] == ptr + 1) {
                   int fwd = strlen(++ptr);
                   ptr += fwd; 
               }
        }
        if (environ[0] == ptr + 1) {
            for (count = 0; environ[count]; count++)
                if (environ[count] == ptr + 1) {
                   int fwd = strlen(++ptr);
                   ptr += fwd; 
               }
        }
        count = 0;
        for (ptr2 = argv[0]; ptr2 <= ptr; ptr2++) {
            *ptr2 = '\0';
            count++;
        }
        strncpy(argv[0], src, count);
    }
}
