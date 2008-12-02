/* dirutil.h ... directory utilities.
 *               C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: dirutil.h,v 1.1.1.1 2002/07/25 06:52:39 honor Exp $
 */

/* Returned malloc'ed string representing basename */
char *basenamex(char *pathname);
/* Return malloc'ed string representing directory name (no trailing slash) */
char *dirname(char *pathname);
/* In-place modify a string to remove trailing slashes.  Returns arg. */
char *stripslash(char *pathname);
/* ensure dirname exists, creating it if necessary. */
int make_valid_path(char *dirname, mode_t mode);
