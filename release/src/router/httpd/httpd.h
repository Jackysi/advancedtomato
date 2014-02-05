#ifndef __HTTPD_H__
#define __HTTPD_H__

extern struct sockaddr_storage clientsai;
extern int post;
extern char *user_agent;

struct mime_handler {
	const char *pattern;
	const char *mime_type;
	int cache;
	void (*input)(char *path, int len, char *boundary);
	void (*output)(char *path);
	int auth;
};
extern const struct mime_handler mime_handlers[];


//

extern void do_file(char *path);


// basic http i/o

typedef enum {
	WOF_NONE,
	WOF_JAVASCRIPT,
	WOF_HTML
} wofilter_t;

extern int web_getline(char *buffer, int max);
extern int web_putc(char c);
extern void web_puts(const char *buffer);
extern void web_putj(const char *buffer);
extern void web_puth(const char *buffer);
extern int web_write(const char *buffer, int len);
extern int web_read(void *buffer, int len);
extern int web_read_x(void *b, int len);
extern int web_eat(int max);
extern int web_flush(void);
extern int web_open(void);
extern int web_close(void);

extern int web_pipecmd(const char *cmd, wofilter_t wof);
extern int web_putfile(const char *fname, wofilter_t wof);

extern int _web_printf(wofilter_t wof, const char *format, ...);
#define web_printf(args...)		_web_printf(WOF_NONE, ##args)
#define web_printfj(args...)	_web_printf(WOF_JAVASCRIPT, ##args)
#define web_printfh(args...)	_web_printf(WOF_HTML, ##args)


// http header handling

extern const char mime_html[];
extern const char mime_plain[];
extern const char mime_javascript[];
extern const char mime_octetstream[];
extern const char mime_binary[];
extern int header_sent;

extern void send_header(int status, const char *header, const char *mime, int cache);
extern void send_error(int status, const char *header, const char *text);
extern void redirect(const char *path);
extern int skip_header(int *len);


// cgi handling

extern void webcgi_init(char *query);
extern char *webcgi_get(const char *name);
extern void webcgi_set(char *name, char *value);

#define webcgi_safeget(key, default) (webcgi_get(key) ? : (default))


// asp file handing

typedef struct {
	const char *name;
	void (*exec)(int argc, char **argv);
} aspapi_t;

extern const aspapi_t aspapi[];

extern int parse_asp(const char *path);


//
extern void check_id(const char *url);

#endif
