/*****************************************************************************\
*  _  _       _          _              ___                                   *
* | || | ___ | |_  _ __ | | _  _  __ _ |_  )                                  *
* | __ |/ _ \|  _|| '_ \| || || |/ _` | / /                                   *
* |_||_|\___/ \__|| .__/|_| \_,_|\__, |/___|                                  *
*                 |_|            |___/                                        *
\*****************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mem_utils.h"
#include "hotplug2.h"
#include "rules.h"

#define last_rule return_rules->rules[return_rules->rules_c - 1]

static void mkdir_p(char *path) {
	char *ptr;
	struct stat statbuf;
	
	path = strdup(path);
	path = dirname(path);
	stat(path, &statbuf);
	/* All is well... */
	if (S_ISDIR(statbuf.st_mode)) {
		free(path);
		return;
	}
	
	for (ptr = path; ptr != NULL; ptr = strchr(ptr, '/')) {
		if (ptr == path) {
			ptr++;
			continue;
		}
		
		errno = 0;
		*ptr='\0';
		mkdir(path, 0755);
		*ptr='/';
		if (errno != 0 && errno != EEXIST)
			break;
		
		ptr++;
	}
	mkdir(path, 0755);
	free(path);
}

static char *replace_str(char *hay, char *needle, char *replacement) {
        char *ptr, *start, *bptr, *buf;
        int occurences, j;
        size_t needle_len;
        size_t replacement_len;
        size_t haystack_len;

	if (replacement == NULL || *replacement=='\0')
		return hay;
	
        if (needle == NULL || *needle=='\0')
                return hay;
 
        occurences = 0;
        j = 0;
        for (ptr = hay; *ptr != '\0'; ++ptr) {
                if (needle[j] == *ptr) {
                        ++j;
                        if (needle[j] == '\0') {
                                *(ptr-j+1) = '\0'; // mark occurence
                                occurences++;
                                j = 0;
                        }
                } else {
			j=0;
		}
        }
       
        if (occurences <= 0)
                return hay;
       
        haystack_len = (size_t)(ptr - hay);
        replacement_len = strlen(replacement);
        needle_len = strlen(needle);
	
	buf = xmalloc(haystack_len + (replacement_len - needle_len) * occurences + 1);
	start = hay;
	ptr = hay;
	
	bptr = buf;
        while (occurences-- > 0) {
                while (*ptr != '\0')
                        ++ptr;

		if (ptr-start > 0) {
			memcpy(bptr, start, ptr - start);
			bptr +=ptr - start;
		}
		
		memcpy(bptr, replacement, replacement_len);
		bptr+=replacement_len;
		ptr += needle_len;
		start = ptr;
	}
	
	while (*ptr != '\0')
		++ptr;

	if (ptr-start > 0) {
		memcpy(bptr, start, ptr - start);
		bptr +=ptr - start;
	}
	*bptr='\0';

	free(hay);
	
        return buf;
}

inline int isescaped(char *hay, char *ptr) {
	if (ptr <= hay)
		return 0;
	
	if (*(ptr-1) != '\\')
		return 0;
	
	return 1;
}

static char *replace_key_by_value(char *hay, struct hotplug2_event_t *event) {
	char *sptr = hay, *ptr = hay;
	char *buf, *replacement;
	
	while ((sptr = strchr(sptr, '%')) != NULL) {
		ptr = strchr(sptr+1, '%');
		if (ptr != NULL) {
			buf = xmalloc(ptr - sptr + 2);
			buf[ptr - sptr + 1] = '\0';
			memcpy(buf, sptr, ptr - sptr + 1);
			
			buf[ptr - sptr] = '\0';
			replacement = get_hotplug2_value_by_key(event, &buf[1]);
			buf[ptr - sptr] = '%';
			
			if (replacement != NULL) {
				hay = replace_str(hay, buf, replacement);
				sptr = hay;
			} else {
				sptr++;
			}
			
			free(buf);
		} else {
			sptr++;
		}
	}
	
	hay = replace_str(hay, "%\\", "%");
	
	return hay;
}

static int make_dev_from_event(struct hotplug2_event_t *event, char *path, mode_t devmode) {
	char *subsystem, *major, *minor, *devpath;
	int rv = 1;
	
	major = get_hotplug2_value_by_key(event, "MAJOR");
	if (major == NULL)
		goto return_value;
	
	minor = get_hotplug2_value_by_key(event, "MINOR");
	if (minor == NULL)
		goto return_value;
	
	devpath = get_hotplug2_value_by_key(event, "DEVPATH");
	if (devpath == NULL)
		goto return_value;
	
	subsystem = get_hotplug2_value_by_key(event, "SUBSYSTEM");
	if (!strcmp(subsystem, "block"))
		devmode |= S_IFBLK;
	else
		devmode |= S_IFCHR;
	
	path = replace_key_by_value(path, event);
	mkdir_p(path);
	rv = mknod(path, devmode, makedev(atoi(major), atoi(minor)));
	free(path);
	
return_value:
	return rv;
}

static int exec_noshell(struct hotplug2_event_t *event, char *application, char **argv) {
	pid_t p;
	int i, status;
	
	p = fork();
	switch (p) {
		case -1:
			return -1;
		case 0:
			for (i = 0; argv[i] != NULL; i++) {
				argv[i] = replace_key_by_value(argv[i], event);
			}
			execvp(application, argv);
			exit(0);
			break;
		default:
			if (waitpid(p, &status, 0) == -1)
				return -1;
			
			return WEXITSTATUS(status);
			break;
	}
}

static int exec_shell(struct hotplug2_event_t *event, char *application) {
	int rv;
	
	application = replace_key_by_value(strdup(application), event);
	rv = WEXITSTATUS(system(application));
	free(application);
	return rv;
}

static int make_symlink(struct hotplug2_event_t *event, char *target, char *linkname) {
	int rv;
	
	target = replace_key_by_value(strdup(target), event);
	linkname = replace_key_by_value(strdup(linkname), event);
	
	mkdir_p(linkname);
	rv = symlink(target, linkname);
	
	free(target);
	free(linkname);
	
	return rv;
}

static int chown_chgrp(int action, char *file, char *param) {
	struct group *grp;
	struct passwd *pwd;
	int rv;
	
	switch (action) {
		case ACT_CHOWN:
			pwd = getpwnam(param);
			rv = chown(file, pwd->pw_uid, -1);
			break;
		case ACT_CHGRP:
			grp = getgrnam(param);
			rv = chown(file, -1, grp->gr_gid);
			break;
	}
	
	return -1;
}

static int rule_condition_eval(struct hotplug2_event_t *event, struct condition_t *condition) {
	int rv;
	char *event_value = NULL;
	regex_t preg;
	
	event_value = get_hotplug2_value_by_key(event, condition->key);
	
	switch (condition->type) {
		case COND_MATCH_CMP:
		case COND_NMATCH_CMP:
			if (event_value == NULL)
				return EVAL_NOT_AVAILABLE;
			
			rv = strcmp(condition->value, event_value) ? EVAL_NOT_MATCH : EVAL_MATCH;
			if (condition->type == COND_NMATCH_CMP)
				rv = !rv;
			
			return rv;
		
		case COND_MATCH_RE:
		case COND_NMATCH_RE:
			if (event_value == NULL)
				return EVAL_NOT_AVAILABLE;
			
			regcomp(&preg, condition->value, REG_EXTENDED | REG_NOSUB);
			
			rv = regexec(&preg, event_value, 0, NULL, 0) ? EVAL_NOT_MATCH : EVAL_MATCH;
			if (condition->type == COND_NMATCH_RE)
				rv = !rv;
			
			regfree(&preg);
		
			return rv;
			
		case COND_MATCH_IS:
			if (!strcasecmp(condition->value, "set"))
				return event_value != NULL;
			
			if (!strcasecmp(condition->value, "unset"))
				return event_value == NULL;
	}
	
	return EVAL_NOT_AVAILABLE;
}

int rule_execute(struct hotplug2_event_t *event, struct rule_t *rule) {
	int i, last_rv;
	
	for (i = 0; i < rule->conditions_c; i++) {
		if (rule_condition_eval(event, &(rule->conditions[i])) != EVAL_MATCH)
			return 0;
	}
	
	last_rv = 0;
	
	for (i = 0; i < event->env_vars_c; i++)
		setenv(event->env_vars[i].key, event->env_vars[i].value, 1);
	
	for (i = 0; i < rule->actions_c; i++) {
		switch (rule->actions[i].type) {
			case ACT_STOP_PROCESSING:
				return 1;
				break;
			case ACT_STOP_IF_FAILED:
				if (last_rv != 0)
					return 1;
				break;
			case ACT_NEXT_EVENT:
				return -1;
				break;
			case ACT_NEXT_IF_FAILED:
				if (last_rv != 0)
					return -1;
				break;
			case ACT_MAKE_DEVICE:
				last_rv = make_dev_from_event(event, rule->actions[i].parameter[0], strtoul(rule->actions[i].parameter[1], NULL, 0));
				break;
			case ACT_CHMOD:
				last_rv = chmod(rule->actions[i].parameter[0], strtoul(rule->actions[i].parameter[1], NULL, 0));
				break;
			case ACT_CHOWN:
			case ACT_CHGRP:
				last_rv = chown_chgrp(rule->actions[i].type, rule->actions[i].parameter[0], rule->actions[i].parameter[1]);
				break;
			case ACT_SYMLINK:
				last_rv = make_symlink(event, rule->actions[i].parameter[0], rule->actions[i].parameter[1]);
				break;
			case ACT_RUN_SHELL:
				last_rv = exec_shell(event, rule->actions[i].parameter[0]);
				break;
			case ACT_RUN_NOSHELL:
				last_rv = exec_noshell(event, rule->actions[i].parameter[0], rule->actions[i].parameter);
				break;
			case ACT_SETENV:
				last_rv = setenv(rule->actions[i].parameter[0], rule->actions[i].parameter[1], 1);
				break;
		}
	}
	
	return 0;
}

static inline int isinitiator(int c) {
	switch (c) {
		case ',':
		case ';':
		case '{':
		case '}':
			return 1;
	}
	
	return 0;
}

static inline void add_buffer(char **buf, int *blen, int *slen, char c) {
	if (*slen + 1 >= *blen) {
		*blen = *blen + 64;
		*buf = xrealloc(*buf, *blen);
	}
	
	(*buf)[*slen] = c;
	(*buf)[*slen+1] = '\0';
	*slen += 1;
}

static char *rules_get_value(char *input, char **nptr) {
	int quotes = QUOTES_NONE;
	char *ptr = input;
	
	int blen, slen;
	char *buf;
	
	blen = slen = 0;
	buf = NULL;
	
	if (isinitiator(*ptr)) {
		add_buffer(&buf, &blen, &slen, *ptr);
		ptr++;
		goto return_value;
	}
	
	while (isspace(*ptr) && *ptr != '\0')
		ptr++;
	
	if (*ptr == '\0')
		return NULL;
	
	switch (*ptr) {
		case '"':
			quotes = QUOTES_DOUBLE;
			ptr++;
			break;
		case '\'':
			quotes = QUOTES_SINGLE;
			ptr++;
			break;
	}
	
	if (quotes != QUOTES_NONE) {
		while (quotes != QUOTES_NONE) {
			switch (*ptr) {
				case '\\':
					ptr++;
					add_buffer(&buf, &blen, &slen, *ptr);
					break;
				case '"':
					if (quotes == QUOTES_DOUBLE)
						quotes = QUOTES_NONE;
					break;
				case '\'':
					if (quotes == QUOTES_SINGLE)
						quotes = QUOTES_NONE;
					break;
				default:
					add_buffer(&buf, &blen, &slen, *ptr);
					break;
			}
			ptr++;
		}
	} else {
		while (!isspace(*ptr) && *ptr != '\0') {
			if (isinitiator(*ptr))
				break;
			
			if (*ptr == '\\')
				ptr++;
			
			add_buffer(&buf, &blen, &slen, *ptr);
			ptr++;
		}
	}
	
return_value:
	while (isspace(*ptr) && *ptr != '\0')
		ptr++;
	
	if (nptr != NULL)
		*nptr = ptr;
	
	return buf;
}

void rules_free(struct rules_t *rules) {
	int i, j, k;
	
	for (i = 0; i < rules->rules_c; i++) {
		for (j = 0; j < rules->rules[i].actions_c; j++) {
			if (rules->rules[i].actions[j].parameter != NULL) {
				for (k = 0; rules->rules[i].actions[j].parameter[k] != NULL; k++)
					free(rules->rules[i].actions[j].parameter[k]);
				free(rules->rules[i].actions[j].parameter);
			}
		}
		for (j = 0; j < rules->rules[i].conditions_c; j++) {
			free(rules->rules[i].conditions[j].key);
			free(rules->rules[i].conditions[j].value);
		}
		free(rules->rules[i].actions);
		free(rules->rules[i].conditions);
	}
	free(rules->rules);
}

struct rules_t *rules_from_config(char *input) {
	int status = STATUS_KEY, terminate;
	char *buf;
	struct rules_t *return_rules;
	
	int i, j;
	struct key_rec_t conditions[] = {	/*NOTE: We never have parameters for conditions. */
		{"is", 0, COND_MATCH_IS}, 
		{"==", 0, COND_MATCH_CMP}, 
		{"!=", 0, COND_NMATCH_CMP}, 
		{"~~", 0, COND_MATCH_RE}, 
		{"!~", 0, COND_NMATCH_RE},
		{NULL, 0, -1}
	};
	struct key_rec_t actions[] = {
		/*one line / one command*/
		{"run", 1, ACT_RUN_SHELL},
		{"exec", -1, ACT_RUN_NOSHELL},
		{"break", 0, ACT_STOP_PROCESSING},
		{"break_if_failed", 0, ACT_STOP_IF_FAILED},
		{"next", 0, ACT_NEXT_EVENT},
		{"next_if_failed", 0, ACT_NEXT_IF_FAILED},
		{"chown", 2, ACT_CHOWN},
		{"chmod", 2, ACT_CHMOD},
		{"chgrp", 2, ACT_CHGRP},
		{"setenv", 2, ACT_SETENV},
		/*symlink*/
		{"symlink", 2, ACT_SYMLINK},
		{"softlink", 2, ACT_SYMLINK},
		/*makedev*/
		{"mknod", 2, ACT_MAKE_DEVICE},
		{"makedev", 2, ACT_MAKE_DEVICE},
		{NULL, 0, -1}
	};

	return_rules = xmalloc(sizeof(struct rules_t));
	return_rules->rules_c = 1;
	return_rules->rules = xmalloc(sizeof(struct rule_t) * return_rules->rules_c);
	
	last_rule.actions = NULL;
	last_rule.actions_c = 0;
	last_rule.conditions = NULL;
	last_rule.conditions_c = 0;
	
	terminate = 0;
	do {
		buf = rules_get_value(input, &input);
		if (buf == NULL) {
			ERROR("rules_get_value", "Malformed rule - unable to read!");
			terminate = 1;
			break;
		}
		
		if (buf[0] == '#') {
			/* Skip to next line */
			while (*input != '\0' && *input != '\n')
				input++;
			continue;
		}
		
		switch (status) {
			case STATUS_KEY:
				last_rule.conditions_c++;
				last_rule.conditions = xrealloc(last_rule.conditions, sizeof(struct condition_t) * last_rule.conditions_c);
				last_rule.conditions[last_rule.conditions_c-1].key = strdup(buf);
				
				status = STATUS_CONDTYPE;
				break;
			case STATUS_CONDTYPE:
				last_rule.conditions[last_rule.conditions_c-1].type = -1;
				
				for (i = 0; conditions[i].key != NULL; i++) {
					if (!strcmp(conditions[i].key, buf)) {
						last_rule.conditions[last_rule.conditions_c-1].type = conditions[i].type;
						break;
					}
				}
				
				if (last_rule.conditions[last_rule.conditions_c-1].type == -1) {
					ERROR("rules_get_value / status / condtype", "Malformed rule - unknown condition type.");
					terminate = 1;
				}
				
				status = STATUS_VALUE;
				break;
			case STATUS_VALUE:
				last_rule.conditions[last_rule.conditions_c-1].value = strdup(buf);
			
				status = STATUS_INITIATOR;
				break;
			case STATUS_INITIATOR:
				if (!strcmp(buf, ",") || !strcmp(buf, ";")) {
					status = STATUS_KEY;
				} else if (!strcmp(buf, "{")) {
					status = STATUS_ACTION;
				} else {
					ERROR("rules_get_value / status / initiator", "Malformed rule - unknown initiator.");
					terminate = 1;
				}
				break;
			case STATUS_ACTION:
				if (!strcmp(buf, "}")) {
					status = STATUS_KEY;
					return_rules->rules_c++;
					return_rules->rules = xrealloc(return_rules->rules, sizeof(struct rule_t) * return_rules->rules_c);
					
					last_rule.actions = NULL;
					last_rule.actions_c = 0;
					last_rule.conditions = NULL;
					last_rule.conditions_c = 0;
					break;
				}
				
				last_rule.actions_c++;
				last_rule.actions = xrealloc(last_rule.actions, sizeof(struct action_t) * last_rule.actions_c);
				last_rule.actions[last_rule.actions_c-1].parameter = NULL;
				last_rule.actions[last_rule.actions_c-1].type = -1;
				
				for (i = 0; actions[i].key != NULL; i++) {
					if (!strcmp(actions[i].key, buf)) {
						last_rule.actions[last_rule.actions_c-1].type = actions[i].type;
						break;
					}
				}
				
				if (last_rule.actions[last_rule.actions_c-1].type == -1) {
					ERROR("rules_get_value / status / action", "Malformed rule - unknown action: %s.", buf);
					terminate = 1;
				}
				
				if (actions[i].param > 0) {
					last_rule.actions[last_rule.actions_c-1].parameter = xmalloc(sizeof(char*) * (actions[i].param + 1));
					last_rule.actions[last_rule.actions_c-1].parameter[actions[i].param] = NULL;
					
					for (j = 0; j < actions[i].param; j++) {
						last_rule.actions[last_rule.actions_c-1].parameter[j] = rules_get_value(input, &input);
						if (!strcmp(last_rule.actions[last_rule.actions_c-1].parameter[j], "}")) {
							ERROR("rules_get_value / status / action", "Malformed rule - not enough parameters passed.");
							status = STATUS_KEY;
							break;
						}
						last_rule.actions[last_rule.actions_c-1].parameter[j] = replace_str(last_rule.actions[last_rule.actions_c-1].parameter[j], "\\}", "}");
					}
				} else if (actions[i].param == -1) {
					j = 0;
					last_rule.actions[last_rule.actions_c-1].parameter = xmalloc(sizeof(char*) * (j + 1));
					last_rule.actions[last_rule.actions_c-1].parameter[j] = rules_get_value(input, &input);
					while (last_rule.actions[last_rule.actions_c-1].parameter[j] != NULL) {
						if (!strcmp(last_rule.actions[last_rule.actions_c-1].parameter[j], ";")) {
							break;
						}
						if (!strcmp(last_rule.actions[last_rule.actions_c-1].parameter[j], "}")) {
							ERROR("rules_get_value / status / action", "Malformed rule - missing parameter terminator ';'.");
							status = STATUS_KEY;
							break;
						}
						if (last_rule.actions[last_rule.actions_c-1].parameter[j][0] == '\0') {
							ERROR("rules_get_value / status / action", "Malformed rule - missing parameter terminator ';'.");
							status = STATUS_KEY;
							break;
						}
						last_rule.actions[last_rule.actions_c-1].parameter[j] = replace_str(last_rule.actions[last_rule.actions_c-1].parameter[j], "\\}", "}");
						last_rule.actions[last_rule.actions_c-1].parameter[j] = replace_str(last_rule.actions[last_rule.actions_c-1].parameter[j], "\\;", ";");
						
						j++;
						last_rule.actions[last_rule.actions_c-1].parameter = xrealloc(last_rule.actions[last_rule.actions_c-1].parameter, sizeof(char*) * (j + 1));
						last_rule.actions[last_rule.actions_c-1].parameter[j]  = rules_get_value(input, &input);
					}
					free(last_rule.actions[last_rule.actions_c-1].parameter[j]);
					last_rule.actions[last_rule.actions_c-1].parameter[j] = NULL;
				}
				
				if (status == STATUS_KEY) {
					return_rules->rules_c++;
					return_rules->rules = xrealloc(return_rules->rules, sizeof(struct rule_t) * return_rules->rules_c);
					
					last_rule.actions = NULL;
					last_rule.actions_c = 0;
					last_rule.conditions = NULL;
					last_rule.conditions_c = 0;
				}
				break;
		}
		
		free(buf);
	} while (*input != '\0' && !terminate);
	
	if (!terminate) {
		/* A little bit hacky cleanup */
		return_rules->rules_c--;
		return return_rules;
	} else {
		rules_free(return_rules);
		free(return_rules);
		return NULL;
	}
}
