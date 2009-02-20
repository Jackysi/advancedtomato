#ifndef VSF_PRIVOPS_H
#define VSF_PRIVOPS_H

struct mystr;
struct vsf_session;

/* vsf_privop_get_ftp_port_sock()
 * PURPOSE
 * Return a network socket bound to a privileged port (less than 1024).
 * PARAMETERS
 * p_sess       - the current session object
 * RETURNS
 * A file descriptor which is a socket bound to the privileged port.
 */
int vsf_privop_get_ftp_port_sock(struct vsf_session* p_sess);

/* vsf_privop_do_file_chown()
 * PURPOSE
 * Takes a file owned by the unprivileged FTP user, and change the ownership
 * to the value defined in the config file.
 * PARAMETERS
 * p_sess       - the current session object
 * fd           - the file descriptor of the regular file
 */
void vsf_privop_do_file_chown(struct vsf_session* p_sess, int fd);

enum EVSFPrivopLoginResult
{
  kVSFLoginNull = 0,
  kVSFLoginFail,
  kVSFLoginAnon,
  kVSFLoginReal
};
/* vsf_privop_do_login()
 * PURPOSE
 * Check if the supplied username/password combination is valid. This
 * interface caters for checking both anonymous and real logins.
 * PARAMETERS
 * p_sess       - the current session object
 * p_pass_str   - the proposed password
 * RETURNS
 * kVSFLoginFail - access denied
 * kVSFLoginAnon - anonymous login credentials OK
 * kVSFLoginReal - real login credentials OK
 */
enum EVSFPrivopLoginResult vsf_privop_do_login(
  struct vsf_session* p_sess, const struct mystr* p_pass_str);

#endif /* VSF_PRIVOPS_H */

