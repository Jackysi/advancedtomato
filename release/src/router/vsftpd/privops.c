/*
 * Part of Very Secure FTPd
 * License: GPL v2
 * Author: Chris Evans
 * privops.c
 *
 * Code implementing the privileged operations that the unprivileged client
 * might request.
 * Look for suitable paranoia in this file.
 */

#include "privops.h"
#include "session.h"
#include "sysdeputil.h"
#include "sysutil.h"
#include "utility.h"
#include "str.h"
#include "tunables.h"
#include "defs.h"
#include "logging.h"

/* File private functions */
static enum EVSFPrivopLoginResult handle_anonymous_login(
  struct vsf_session* p_sess, const struct mystr* p_pass_str);
static enum EVSFPrivopLoginResult handle_local_login(
  struct vsf_session* p_sess, const struct mystr* p_user_str,
  const struct mystr* p_pass_str);
static void setup_username_globals(struct vsf_session* p_sess,
                                   const struct mystr* p_str);
static enum EVSFPrivopLoginResult handle_login(
  struct vsf_session* p_sess, const struct mystr* p_user_str,
  const struct mystr* p_pass_str);

int
vsf_privop_get_ftp_port_sock(struct vsf_session* p_sess)
{
  static struct vsf_sysutil_sockaddr* p_sockaddr;
  int retval;
  int i;
  int s = vsf_sysutil_get_ipsock(p_sess->p_local_addr);
  vsf_sysutil_activate_reuseaddr(s);
  /* A report of failure here on Solaris, presumably buggy address reuse
   * support? We'll retry.
   */
  for (i = 0; i < 2; ++i)
  {
    double sleep_for;
    vsf_sysutil_sockaddr_clone(&p_sockaddr, p_sess->p_local_addr);
    vsf_sysutil_sockaddr_set_port(p_sockaddr, tunable_ftp_data_port);
    retval = vsf_sysutil_bind(s, p_sockaddr);
    if (retval == 0)
    {
      return s;
    }
    if (vsf_sysutil_get_error() != kVSFSysUtilErrADDRINUSE || i == 1)
    {
      die("vsf_sysutil_bind");
    }
    sleep_for = vsf_sysutil_get_random_byte();
    sleep_for /= 256.0;
    sleep_for += 1.0;
    vsf_sysutil_sleep(sleep_for);
  }
  return s;
}

void
vsf_privop_do_file_chown(struct vsf_session* p_sess, int fd)
{
  static struct vsf_sysutil_statbuf* s_p_statbuf;
  vsf_sysutil_fstat(fd, &s_p_statbuf);
  /* Do nothing if it is already owned by the desired user. */
  if (vsf_sysutil_statbuf_get_uid(s_p_statbuf) ==
      p_sess->anon_upload_chown_uid)
  {
    return;
  }
  /* Drop it like a hot potato unless it's a regular file owned by
   * the the anonymous ftp user
   */
  if (p_sess->anon_upload_chown_uid == -1 ||
      !vsf_sysutil_statbuf_is_regfile(s_p_statbuf) ||
      (vsf_sysutil_statbuf_get_uid(s_p_statbuf) != p_sess->anon_ftp_uid &&
       vsf_sysutil_statbuf_get_uid(s_p_statbuf) != p_sess->guest_user_uid))
  {
    die("invalid fd in cmd_process_chown");
  }
  /* SECURITY! You need an OS which strips SUID/SGID bits on chown(),
   * otherwise a compromise of the FTP user will lead to compromise of
   * the "anon_upload_chown_uid" user (think chmod +s).
   */
  vsf_sysutil_fchown(fd, p_sess->anon_upload_chown_uid, -1);
}

enum EVSFPrivopLoginResult
vsf_privop_do_login(struct vsf_session* p_sess,
                    const struct mystr* p_pass_str)
{
  enum EVSFPrivopLoginResult result =
    handle_login(p_sess, &p_sess->user_str, p_pass_str);
  vsf_log_start_entry(p_sess, kVSFLogEntryLogin);
  if (result == kVSFLoginFail)
  {
    vsf_log_do_log(p_sess, 0);
  }
  else
  {
    vsf_log_do_log(p_sess, 1);
    if (tunable_delay_successful_login)
    {
      vsf_sysutil_sleep((double) tunable_delay_successful_login);
    }
  }
  return result;
}

static enum EVSFPrivopLoginResult
handle_login(struct vsf_session* p_sess, const struct mystr* p_user_str,
             const struct mystr* p_pass_str)
{
  /* Do not assume PAM can cope with dodgy input, even though it
   * almost certainly can.
   */
  int anonymous_login = 0;
  char first_char;
  unsigned int len = str_getlen(p_user_str);
  if (len == 0 || len > VSFTP_USERNAME_MAX)
  {
    return kVSFLoginFail;
  }
  /* Throw out dodgy start characters */
  first_char = str_get_char_at(p_user_str, 0);
  if (!vsf_sysutil_isalnum(first_char) &&
      first_char != '_' &&
      first_char != '.')
  {
    return kVSFLoginFail;
  }
  /* Throw out non-printable characters and space in username */
  if (str_contains_space(p_user_str) ||
      str_contains_unprintable(p_user_str))
  {
    return kVSFLoginFail;
  }
  /* Throw out excessive length passwords */
  len = str_getlen(p_pass_str);
  if (len > VSFTP_PASSWORD_MAX)
  {
    return kVSFLoginFail;
  }
  /* Check for an anonymous login or "real" login */
  if (tunable_anonymous_enable)
  {
    struct mystr upper_str = INIT_MYSTR;
    str_copy(&upper_str, p_user_str);
    str_upper(&upper_str);
    if (str_equal_text(&upper_str, "FTP") ||
        str_equal_text(&upper_str, "ANONYMOUS"))
    {
      anonymous_login = 1;
    }
    str_free(&upper_str);
  }
  {
    enum EVSFPrivopLoginResult result = kVSFLoginFail;
    if (anonymous_login)
    {
      result = handle_anonymous_login(p_sess, p_pass_str);
    }
    else
    {
      if (!tunable_local_enable)
      {
        die("unexpected local login in handle_login");
      }
      result = handle_local_login(p_sess, p_user_str, p_pass_str);
    }
    return result;
  }
}

static enum EVSFPrivopLoginResult
handle_anonymous_login(struct vsf_session* p_sess,
                       const struct mystr* p_pass_str)
{
  if (!str_isempty(&p_sess->banned_email_str) &&
      str_contains_line(&p_sess->banned_email_str, p_pass_str))
  {
    return kVSFLoginFail;
  }
  if (!str_isempty(&p_sess->email_passwords_str) &&
      (!str_contains_line(&p_sess->email_passwords_str, p_pass_str) ||
       str_isempty(p_pass_str)))
  {
    return kVSFLoginFail;
  }
  /* Store the anonymous identity string */
  str_copy(&p_sess->anon_pass_str, p_pass_str);
  if (str_isempty(&p_sess->anon_pass_str))
  {
    str_alloc_text(&p_sess->anon_pass_str, "?");
  }
  /* "Fix" any characters which might upset the log processing */
  str_replace_char(&p_sess->anon_pass_str, ' ', '_');
  str_replace_char(&p_sess->anon_pass_str, '\n', '?');
  {
    struct mystr ftp_username_str = INIT_MYSTR;
    str_alloc_text(&ftp_username_str, tunable_ftp_username);
    setup_username_globals(p_sess, &ftp_username_str);
    str_free(&ftp_username_str);
  }
  str_free(&p_sess->banned_email_str);
  str_free(&p_sess->email_passwords_str);
  return kVSFLoginAnon;
}

static enum EVSFPrivopLoginResult
handle_local_login(struct vsf_session* p_sess,
                   const struct mystr* p_user_str,
                   const struct mystr* p_pass_str)
{
  if (!vsf_sysdep_check_auth(p_user_str, p_pass_str, &p_sess->remote_ip_str))
  {
    return kVSFLoginFail;
  }
  setup_username_globals(p_sess, p_user_str);
  return kVSFLoginReal;
}

static void
setup_username_globals(struct vsf_session* p_sess, const struct mystr* p_str)
{
  str_copy(&p_sess->user_str, p_str);
  if (tunable_setproctitle_enable)
  {
    struct mystr prefix_str = INIT_MYSTR;
    str_copy(&prefix_str, &p_sess->remote_ip_str);
    str_append_char(&prefix_str, '/');
    str_append_str(&prefix_str, p_str);
    vsf_sysutil_set_proctitle_prefix(&prefix_str);
    str_free(&prefix_str);
  }
}

