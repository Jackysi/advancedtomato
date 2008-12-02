/* files.h */
#ifndef _FILES_H
#define _FILES_H

struct config_keyword {
	char keyword[14];
	int (*handler)(char *line, void *var);
	void *var;
	char def[30];
};


int read_config(char *file);
void write_leases(void);
void read_leases(char *file);
void delete_leases(void);
int compare_leases(uint32_t requested_ip);

#endif
