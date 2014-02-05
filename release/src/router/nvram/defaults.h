#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

typedef struct {
	const char *key;
	const char *value;
} defaults_t;

extern const defaults_t defaults[];
extern const defaults_t if_generic[];
extern const defaults_t if_vlan[];

#endif
