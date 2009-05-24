/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <sys/stat.h>



static const char webMsgFile[] = "/tmp/.webmsg";


void setWebMsg(const char *s)
{
	FILE *f;
	if (s) {
		if ((f = fopen(webMsgFile, "w")) != NULL) {
			fputs(s, f);
			fclose(f);
		}
	}
	else {
		unlink(webMsgFile);
	}
}

void getWebMsg(char *s, int max)
{
	FILE *f;
	int n;

	if ((f = fopen(webMsgFile, "r")) != NULL) {
		n = fread(s, 1, max - 1, f);
		s[n] = 0;
		fclose(f);
		unlink(webMsgFile);
	}
	else s[0] = 0;
}

int webMsgExist(void)
{
	struct stat st;
	return ((stat(webMsgFile, &st) == 0) && (st.st_size > 0));
}

void asp_webmsg(int argc, char **argv)
{
	char s[512];
	const char *msg = s;

	getWebMsg(s, sizeof(s));
	if (s[0] == 0) {
		if (argc == 0) return;
		msg = argv[0];
	}
	if ((argc >= 2) && (argv[1][0] == '1')) web_putj(msg);
        else web_puth(msg);
}


