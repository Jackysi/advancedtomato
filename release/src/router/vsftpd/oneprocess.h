#ifndef VSF_ONEPROCESS_H
#define VSF_ONEPROCESS_H

struct mystr;
struct vsf_session;

/* vsf_one_process_start()
 * PURPOSE
 * Called to start FTP login processing using the one process model. Before
 * processing starts, all possible privileges are dropped.
 * PARAMETERS
 * p_sess       - the current session object
 */
void vsf_one_process_start(struct vsf_session* p_sess);

/* vsf_one_process_login()
 * PURPOSE
 * Called to propose a login using the one process model. Only anonymous
 * logins supported!
 * PARAMETERS
 * p_sess       - the current session object
 * p_pass_str   - the proposed password
 */
void vsf_one_process_login(struct vsf_session* p_sess,
                           const struct mystr* p_pass_str);

/* vsf_one_process_get_priv_data_sock()
 * PURPOSE
 * Get a privileged port 20 bound data socket using the one process model.
 * PARAMETERS
 * p_sess       - the current session object
 * RETURNS
 * The file descriptor of the privileged socket
 */
int vsf_one_process_get_priv_data_sock(struct vsf_session* p_sess);

/* vsf_one_process_chown_upload()
 * PURPOSE
 * Change ownership of an uploaded file using the one process model.
 * PARAMETERS
 * p_sess       - the current session object
 * fd           - the file descriptor to change ownership on
 */
void vsf_one_process_chown_upload(struct vsf_session* p_sess, int fd);

#endif /* VSF_ONEPROCESS_H */

