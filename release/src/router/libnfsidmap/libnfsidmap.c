/*
 *  libnfsidmap.c
 *
 *  nfs idmapping library, primarily for nfs4 client/server kernel idmapping
 *  and for userland nfs4 idmapping by acl libraries.
 *
 *  Copyright (c) 2004 The Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  Marius Aamodt Eriksen <marius@umich.edu>
 *  J. Bruce Fields <bfields@umich.edu>
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

#include "config.h"

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <err.h>
#include <syslog.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <ctype.h>
#include "nfsidmap.h"
#include "nfsidmap_internal.h"
#include "cfg.h"

static char *default_domain;
static struct conf_list *local_realms;
int idmap_verbosity = 0;
static struct mapping_plugin **nfs4_plugins = NULL;
static struct mapping_plugin **gss_plugins = NULL;
uid_t nobody_uid = (uid_t)-1;
gid_t nobody_gid = (gid_t)-1;

#ifndef PATH_PLUGINS
#define PATH_PLUGINS "/usr/lib/libnfsidmap"
#endif
#define PLUGIN_INIT_FUNC "libnfsidmap_plugin_init"


#ifndef PATH_IDMAPDCONF
#define PATH_IDMAPDCONF "/etc/idmapd.conf"
#endif

#ifndef IDMAPD_DEFAULT_DOMAIN
#define IDMAPD_DEFAULT_DOMAIN "localdomain"
#endif

/* Default logging fuction */
static void default_logger(const char *fmt, ...)
{
	va_list vp;

	va_start(vp, fmt);
	vsyslog(LOG_WARNING, fmt, vp);
	va_end(vp);
}
nfs4_idmap_log_function_t idmap_log_func = default_logger;

static char * toupper_str(char *s)
{
	int i;
	for (i=0; i < strlen(s); i++)
		s[i] = toupper(s[i]);
	return s;
}

static int id_as_chars(char *name, int *id)
{
	long int value = strtol(name, NULL, 10);

	if (value == 0) {
		/* zero value ids are valid */
		if (strcmp(name, "0") != 0)
			return 0;
	}
	*id = (int)value;
	return 1;
}

static int domain_from_dns(char **domain)
{
	struct hostent *he;
	char hname[64], *c;

	if (gethostname(hname, sizeof(hname)) == -1)
		return -1;
	if ((he = gethostbyname(hname)) == NULL)
		return -1;
	if ((c = strchr(he->h_name, '.')) == NULL || *++c == '\0')
		return -1;
	*domain = strdup(c);
	return 0;
}

static int load_translation_plugin(char *method, struct mapping_plugin *plgn)
{
	void *dl = NULL;
	struct trans_func *trans = NULL;
	libnfsidmap_plugin_init_t init_func;
	char plgname[128];
	int ret = 0;

	snprintf(plgname, sizeof(plgname), "%s/%s.so", PATH_PLUGINS, method);

	dl = dlopen(plgname, RTLD_NOW | RTLD_LOCAL);
	if (dl == NULL) {
		IDMAP_LOG(1, ("libnfsidmap: Unable to load plugin: %s\n",
			  dlerror()));
		return -1;
	}
	init_func = (libnfsidmap_plugin_init_t) dlsym(dl, PLUGIN_INIT_FUNC);
	if (init_func == NULL) {
		IDMAP_LOG(1, ("libnfsidmap: Unable to get init function: %s\n",
			  dlerror()));
		dlclose(dl);
		return -1;
	}
	trans = init_func();
	if (trans == NULL) {
		IDMAP_LOG(1, ("libnfsidmap: Failed to initialize plugin %s\n",
			  PLUGIN_INIT_FUNC, plgname));
		dlclose(dl);
		return -1;
	}
	if (trans->init) {
		ret = trans->init();
		if (ret) {
			IDMAP_LOG(1, ("libnfsidmap: Failed in %s's init(), "
					"returned %d\n", plgname, ret));
			dlclose(dl);
			return -1;
		}
	}
	plgn->dl_handle = dl;
	plgn->trans = trans;
	IDMAP_LOG(1, ("libnfsidmap: loaded plugin %s for method %s\n",
		  plgname, method));

	return 0;
}

static void unload_plugins(struct mapping_plugin **plgns)
{
	int i;
	for (i = 0; plgns[i] != NULL; i++) {
		if (plgns[i]->dl_handle && dlclose(plgns[i]->dl_handle))
			IDMAP_LOG(1, ("libnfsidmap: failed to "
				  "unload plugin for method = %s\n",
				  plgns[i]->trans->name));
		free(plgns[i]);
	}
	free(plgns);
}

static int load_plugins(struct conf_list *methods,
			struct mapping_plugin ***plugins)
{
	int ret = -1, i = 0;
	struct mapping_plugin **plgns;
	struct conf_list_node *m;

	plgns = calloc(methods->cnt + 1, sizeof(struct mapping_plugin *));
	if (plgns == NULL)
		return -1;
	plgns[methods->cnt] = NULL;
	for (m = TAILQ_FIRST(&methods->fields), i = 0; m;
	     m = TAILQ_NEXT(m, link), i++) {
		plgns[i] = calloc(1, sizeof(struct mapping_plugin));
		if (plgns[i] == NULL)
			goto out;
		if (load_translation_plugin(m->field, plgns[i]) == -1) {
			IDMAP_LOG(0, ("libnfsidmap: requested translation "
				  "method, '%s', is not available\n",
				  m->field));
			goto out;
		}
	}
	ret = 0;
	*plugins = plgns;
out:
	if (ret)
		unload_plugins(plgns);
	return ret;
}
void nfs4_cleanup_name_mapping()
{
	if (nfs4_plugins)
		unload_plugins(nfs4_plugins);
	if (gss_plugins)
		unload_plugins(gss_plugins);
	nfs4_plugins = gss_plugins = NULL;
}

int nfs4_init_name_mapping(char *conffile)
{
	int ret = -ENOENT;
	int dflt = 0;
	struct conf_list *nfs4_methods, *gss_methods;
	char *nobody_user, *nobody_group;

	/* XXX: need to be able to reload configurations... */
	if (nfs4_plugins) /* already succesfully initialized */
		return 0;
	if (conffile)
		conf_path = conffile;
	else
		conf_path = PATH_IDMAPDCONF;
	conf_init();
	default_domain = conf_get_str("General", "Domain");
	if (default_domain == NULL) {
		dflt = 1;
		ret = domain_from_dns(&default_domain);
		if (ret) {
			IDMAP_LOG(1, ("libnfsidmap: Unable to determine "
				  "the NFSv4 domain; Using '%s' as the NFSv4 domain "
				  "which means UIDs will be mapped to the 'Nobody-User' "
				  "user defined in %s\n", 
				  IDMAPD_DEFAULT_DOMAIN, PATH_IDMAPDCONF));
			default_domain = IDMAPD_DEFAULT_DOMAIN;
		}
	}
	IDMAP_LOG(1, ("libnfsidmap: using%s domain: %s",
		(dflt ? " (default)" : ""), default_domain));

	/* Get list of "local equivalent" realms.  Meaning the list of realms
	 * where john@REALM.A is considered the same user as john@REALM.B
	 * If not specified, default to upper-case of local domain name */
	local_realms = conf_get_list("General", "Local-Realms");
	if (local_realms == NULL) {
		struct conf_list_node *node;

		local_realms = malloc(sizeof *local_realms);
		if (local_realms == NULL)
			return -ENOMEM;
		local_realms->cnt = 0;
		TAILQ_INIT(&local_realms->fields);

		node = calloc(1, sizeof *node);
		if (node == NULL)
			return -ENOMEM;
		node->field = strdup(get_default_domain());
		if (node->field == NULL)
			return -ENOMEM;
		toupper_str(node->field);

		TAILQ_INSERT_TAIL(&local_realms->fields, node, link);
		local_realms->cnt++;
	}

	if (idmap_verbosity >= 1) {
		struct conf_list_node *r;
		char *buf = NULL;
		int siz=0;

		if (local_realms) {
			TAILQ_FOREACH(r, &local_realms->fields, link) {
				siz += (strlen(r->field)+4);
			}
			buf = malloc(siz);
			if (buf) {
				*buf = 0;
				TAILQ_FOREACH(r, &local_realms->fields, link) {
					sprintf(buf+strlen(buf), "'%s' ", r->field);
				}
				IDMAP_LOG(1, ("libnfsidmap: Realms list: %s", buf));
				free(buf);
			}
		} else 
			IDMAP_LOG(1, ("libnfsidmap: Realms list: <NULL> "));
	}

	nfs4_methods = conf_get_list("Translation", "Method");
	if (nfs4_methods) {
		IDMAP_LOG(1, ("libnfsidmap: processing 'Method' list"));
		if (load_plugins(nfs4_methods, &nfs4_plugins) == -1)
			return -ENOENT;
	} else {
		struct conf_list list;
		struct conf_list_node node;

		TAILQ_INIT(&list.fields);
		list.cnt = 1;
		node.field = "nsswitch";
		TAILQ_INSERT_TAIL (&list.fields, &node, link);

		if (load_plugins(&list, &nfs4_plugins) == -1)
			return -ENOENT;
	}

	gss_methods = conf_get_list("Translation", "GSS-Methods");
	if (gss_methods) {
		IDMAP_LOG(1, ("libnfsidmap: processing 'GSS-Methods' list"));
		if (load_plugins(gss_methods, &gss_plugins) == -1)
			goto out;
	}

	nobody_user = conf_get_str("Mapping", "Nobody-User");
	if (nobody_user) {
		size_t buflen = sysconf(_SC_GETPW_R_SIZE_MAX);
		struct passwd *buf;
		struct passwd *pw = NULL;
		int err;

		buf = malloc(sizeof(*buf) + buflen);
		if (buf) {
			err = getpwnam_r(nobody_user, buf, ((char *)buf) + sizeof(*buf), buflen, &pw);
			if (err == 0 && pw != NULL)
				nobody_uid = pw->pw_uid;
			else
				IDMAP_LOG(1, ("libnfsidmap: Nobody-User (%s) not found: %s\n", 
					nobody_user, strerror(errno)));
			free(buf);
		} else
			IDMAP_LOG(0,("libnfsidmap: Nobody-User: no memory : %s\n", 
					nobody_user, strerror(errno)));
	}

	nobody_group = conf_get_str("Mapping", "Nobody-Group");
	if (nobody_group) {
		size_t buflen = sysconf(_SC_GETGR_R_SIZE_MAX);
		struct group *buf;
		struct group *gr = NULL;
		int err;

		buf = malloc(sizeof(*buf) + buflen);
		if (buf) {
			err = getgrnam_r(nobody_group, buf, ((char *)buf) + sizeof(*buf), buflen, &gr);
			if (err == 0 && gr != NULL)
				nobody_gid = gr->gr_gid;
			else
				IDMAP_LOG(1, ("libnfsidmap: Nobody-Group (%s) not found: %s\n", 
					nobody_group, strerror(errno)));
			free(buf);
		} else
			IDMAP_LOG(0,("libnfsidmap: Nobody-Group: no memory : %s\n", 
					nobody_group, strerror(errno)));
	}

	ret = 0;
out:
	if (ret) {
		if (nfs4_plugins)
			unload_plugins(nfs4_plugins);
		if (gss_plugins)
			unload_plugins(gss_plugins);
		nfs4_plugins = gss_plugins = NULL;
	}

	return ret ? -ENOENT: 0;
}

char * get_default_domain(void)
{
	int ret;

	if (default_domain)
		return default_domain;
	ret = domain_from_dns(&default_domain);
	if (ret) {
		IDMAP_LOG(0, ("Unable to determine a default nfsv4 domain; "
			" consider specifying one in idmapd.conf\n"));
		default_domain = "";
	}
	return default_domain;
}

struct conf_list *get_local_realms(void)
{
	return local_realms;
}

int
nfs4_get_default_domain(char *server, char *domain, size_t len)
{
	char *d = get_default_domain();

	if (strlen(d) + 1 > len)
		return -ERANGE;
	strcpy(domain, d);
	return 0;
}

/*
 * Run through each configured translation method for
 * function "funcname".
 * If "prefer_gss" is true, then use the gss_plugins list,
 * if present.  Otherwise, use the default nfs4_plugins list.
 *
 * If the plugin function returns -ENOENT, then continue
 * to the next plugin.
 */
#define RUN_TRANSLATIONS(funcname, prefer_gss, args...)			\
	do {								\
		int ret, i;						\
		struct mapping_plugin **plgns;				\
									\
		ret = nfs4_init_name_mapping(NULL);			\
		if (ret)						\
			return ret;					\
									\
		if ((prefer_gss) && gss_plugins)			\
			plgns = gss_plugins;				\
		else							\
			plgns = nfs4_plugins;				\
									\
		for (i = 0; plgns[i] != NULL; i++) {			\
			if (plgns[i]->trans->funcname == NULL)		\
				continue;				\
									\
			IDMAP_LOG(4, ("%s: calling %s->%s\n", __func__,	\
				  plgns[i]->trans->name, #funcname));	\
									\
			ret = plgns[i]->trans->funcname(args);		\
									\
			IDMAP_LOG(4, ("%s: %s->%s returned %d\n",	\
				  __func__, plgns[i]->trans->name,	\
				  #funcname, ret));			\
									\
			if (ret == -ENOENT)				\
				continue;				\
									\
			break;						\
		}							\
		IDMAP_LOG(4, ("%s: final return value is %d\n",		\
			  __func__, ret));				\
		return ret;						\
	} while (0)

int nfs4_uid_to_name(uid_t uid, char *domain, char *name, size_t len)
{
	RUN_TRANSLATIONS(uid_to_name, 0, uid, domain, name, len);
}

int nfs4_gid_to_name(gid_t gid, char *domain, char *name, size_t len)
{
	RUN_TRANSLATIONS(gid_to_name, 0, gid, domain, name, len);
}

int nfs4_uid_to_owner(uid_t uid, char *domain, char *name, size_t len)
{
	if (nfs4_uid_to_name(uid, domain, name, len))
		sprintf(name, "%u", uid);
	return 0;
}

int nfs4_gid_to_group_owner(gid_t gid, char *domain, char *name, size_t len)
{
	if (nfs4_gid_to_name(gid, domain, name, len))
		sprintf(name, "%u", gid);
	return 0;
}

int nfs4_name_to_uid(char *name, uid_t *uid)
{
	RUN_TRANSLATIONS(name_to_uid, 0, name, uid);
}

int nfs4_name_to_gid(char *name, gid_t *gid)
{
	RUN_TRANSLATIONS(name_to_gid, 0, name, gid);
}

static int set_id_to_nobody(int *id, int is_uid)
{
	int rc = 0;
	const char name[] = "nobody@";
	char nobody[strlen(name) + strlen(get_default_domain()) + 1];

	/* First try to see whether a Nobody-User/Nobody-Group was
         * configured, before we try to do a full lookup for the
         * NFS nobody user. */
	if (is_uid && nobody_uid != (uid_t)-1) {
		*id = (int)nobody_uid;
		return 0;
	} else if (!is_uid && nobody_gid != (gid_t)-1) {
		*id = (int)nobody_gid;
		return 0;
	}

	strcpy(nobody, name);
	strcat(nobody, get_default_domain());

	if (is_uid)
		rc = nfs4_name_to_uid(nobody, id);
	else
		rc = nfs4_name_to_gid(nobody, id);

	if (rc) {
		*id = -2;
		rc = 0;
	}
	return rc;
}

int nfs4_owner_to_uid(char *name, uid_t *uid)
{
	int rc = nfs4_name_to_uid(name, uid);
	if (rc && id_as_chars(name, uid))
		rc = 0;
	else if (rc)
		rc = set_id_to_nobody(uid, 1);
	return rc;
}

int nfs4_group_owner_to_gid(char *name, gid_t *gid)
{
	int rc = nfs4_name_to_gid(name, gid);
	if (rc && id_as_chars(name, gid))
		rc = 0;
	else if (rc)
		rc = set_id_to_nobody(gid, 0);
	return rc;
}

int nfs4_gss_princ_to_ids(char *secname, char *princ, uid_t *uid, gid_t *gid)
{
	RUN_TRANSLATIONS(princ_to_ids, 1, secname, princ, uid, gid, NULL);
}

int nfs4_gss_princ_to_grouplist(char *secname, char *princ,
				gid_t *groups, int *ngroups)
{
	RUN_TRANSLATIONS(gss_princ_to_grouplist, 1, secname, princ,
			 groups, ngroups, NULL);
}

int nfs4_gss_princ_to_ids_ex(char *secname, char *princ, uid_t *uid,
			     gid_t *gid, extra_mapping_params **ex)
{
	RUN_TRANSLATIONS(princ_to_ids, 1, secname, princ, uid, gid, ex);
}

int nfs4_gss_princ_to_grouplist_ex(char *secname, char *princ, gid_t *groups,
				   int *ngroups, extra_mapping_params **ex)
{
	RUN_TRANSLATIONS(gss_princ_to_grouplist, 1, secname, princ,
			 groups, ngroups, ex);
}

void nfs4_set_debug(int dbg_level, void (*logger)(const char *, ...))
{
	if (logger)
		idmap_log_func = logger;
	idmap_verbosity = dbg_level;
}

