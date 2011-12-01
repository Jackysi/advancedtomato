/*
 *  static.c
 *
 *  static idmapping functions for gss principals.
 *
 *  Copyright (c) 2008 David Härdeman <david@hardeman.nu>.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the University nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "cfg.h"
#include "nfsidmap.h"
#include "nfsidmap_internal.h"

/*
 * Static Translation Methods
 *
 * These functions use getpwnam to find uid/gid(s) for gss principals
 * which are first mapped to local user names using static mappings
 * in idmapd.conf.
 */

struct pwbuf {
	struct passwd pwbuf;
	char buf[1];
};

static struct passwd *static_getpwnam(const char *name, const char *domain,
				      int *err_p)
{
	struct passwd *pw;
	struct pwbuf *buf;
	size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
	char *localname;
	int err;

	buf = malloc(sizeof(*buf) + buflen);
	if (!buf) {
		err = ENOMEM;
		goto err;
	}

	localname = conf_get_str("Static", (char *)name);
	if (!localname) {
		err = ENOENT;
		goto err;
	}

	IDMAP_LOG(4, ("static_getpwnam: name '%s' mapped to '%s'\n",
		  name, localname));

again:
	err = getpwnam_r(localname, &buf->pwbuf, buf->buf, buflen, &pw);

	if (err == EINTR)
		goto again;

	if (!pw) {
		if (err == 0)
			err = ENOENT;

		IDMAP_LOG(0, ("static_getpwnam: name '%s' not found\n",
			  localname));

		goto err_free_buf;
	}

	*err_p = 0;
	return pw;

err_free_buf:
	free(buf);
err:
	*err_p = err;
	return NULL;
}

static int static_gss_princ_to_ids(char *secname, char *princ,
				   uid_t *uid, uid_t *gid,
				   extra_mapping_params **ex)
{
	struct passwd *pw;
	int err;

	/* XXX: Is this necessary? */
	if (strcmp(secname, "krb5") != 0 && strcmp(secname, "spkm3") != 0)
		return -EINVAL;

	pw = static_getpwnam(princ, NULL, &err);

	if (pw) {
		*uid = pw->pw_uid;
		*gid = pw->pw_gid;
		free(pw);
	}

	return -err;
}

static int static_gss_princ_to_grouplist(char *secname, char *princ,
					 gid_t *groups, int *ngroups,
					 extra_mapping_params **ex)
{
	struct passwd *pw;
	int err;

	/* XXX: Is this necessary? */
	if (strcmp(secname, "krb5") != 0 && strcmp(secname, "spkm3") != 0)
		return -EINVAL;

	pw = static_getpwnam(princ, NULL, &err);

	if (pw) {
		if (getgrouplist(pw->pw_name, pw->pw_gid, groups, ngroups) < 0)
			err = -ERANGE;
		free(pw);
	}

	return -err;
}


struct trans_func static_trans = {
	.name			= "static",
	.init			= NULL,
	.name_to_uid		= NULL,
	.name_to_gid		= NULL,
	.uid_to_name		= NULL,
	.gid_to_name		= NULL,
	.princ_to_ids		= static_gss_princ_to_ids,
	.gss_princ_to_grouplist	= static_gss_princ_to_grouplist,
};

struct trans_func *libnfsidmap_plugin_init()
{
	return (&static_trans);
}

