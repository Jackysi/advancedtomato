/*
	 * nginx.c
	 *
	 * Copyright (C) 2013 NGinX for Tomato RAF
	 *  ***** Ofer Chen, roadkill AT tomatoraf DOT com
	 *  ***** Vicente Soriano, victek AT tomatoraf DOT com
	 *
	 * No part of this program can be used out of Tomato Firmware without owners permission.
	 * This code generates the configurations files for NGINX. You can see these files in /etc/nginx/
*/

#include <stdlib.h>
#include <rc.h>
#include <shutils.h>
#include <utils.h>
#include <syslog.h>
#include <sys/stat.h>
#include <stdarg.h>

#define nginxbin		"nginx"					// process name
#define nginxname		"tomato.local"				// server name
#define nginxdocrootdir		"/www"					// document root
#define nginxconf		"/tmp/etc/nginx/nginx.conf"		// config file
#define nginxcustom		"#"					// additional window for custom parameter.
#define fastcgiconf		"/tmp/etc/nginx/fastcgi.conf"		// fastcgi config file
#define mimetypes		"/tmp/etc/nginx/mime.types"		// mime.types config
#define nginxdir		"/tmp/etc/nginx/"			// directory to write config files
#define client_body_temp_path	"/tmp/var/lib/nginx/client"		// temp path needed to execute nginx
#define fastcgi_temp_path	"/tmp/var/lib/nginx/fastcgi"		// temp path needed to execute nginx fastcgi
#define uwsgi_temp_path		"/tmp/var/lib/nginx/uwsgi"		// temp path needed to execute nginx
#define scgi_temp_path		"/tmp/var/lib/nginx/scgi"		// temp path needed to execute nginx
#define proxy_temp_path		"/tmp/var/lib/nginx/proxy"		// temp path needed to execute reverse proxy
//#define nginxuser		"root"					// user. Beta test, root can't be exposed.
#define nginx_worker_proc	"1"					// worker processes. CPU, cores.
#define nginx_cpu_affinity	"0101"					// Can bind the worker process to a CPU, it calls sched_setaffinity().
//define nginx_worker_priority	"5"					// priority ((-20)=High Priority (19)=Lowest Priority) Info: kernel have -5.
#define nginx_multi_accept	"on"					// specifies multiple connections for one core CPU
#define nginx_limitrate		"50k"					// specifies a limit rate of 50Kbps (multiply it by limit_conn_zone) per IP.
#define nginx_limitzone		"one"					// specifies the server zone to apply the restriction (limit_rate+n).
#define nginx_globrate		"5m"					// defines max global speed.
#define nginx_master_process	"off"					// set to "on" in developpment mode only.
#define nginx_limitsimconn	"10"					// defines max number of simulta. Connections in the server zone (limitzone).
#define nginxerrorlog		"/tmp/var/log/nginx/error.log"		// error log
#define nginxpid		"/tmp/var/run/nginx.pid"		// pid
#define nginx_worker_rlimit_profile	"8192"				// worker rlimit profile
#define nginx_keepalive_timeout		"60"				// the server will close connections after this time
#define nginx_worker_connections	"512"				// worker_proc*512/keepalive_timeout*60 = 512 users per minute.
#define nginxaccesslog			"/tmp/var/log/nginx/access.log"	// access log
#define nginssendfile			"on"				// sendfile
#define nginxtcp_nopush			"on"				// tcp_nopush
#define nginxserver_names_hash_bucket_size	"128"			// server names hash bucket size

FILE * nginx_conf_file;
FILE * fastcgi_conf_file;
FILE * mimetypes_file;
FILE * phpini_file;
unsigned int fastpath=0;

void nginx_write(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(nginx_conf_file, format, args);
	va_end(args);
}

void fastcgi_write(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(fastcgi_conf_file, format, args);
	va_end(args);
}


void mimetypes_write(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(mimetypes_file, format, args);
	va_end(args);
}

int build_fastcgi_conf(void) {
// Starting a fastcgi configuration file 
//		syslog(LOG_INFO,"FastCGI - config file generation started\n");

	if(mkdir_if_none(nginxdir));
//		syslog(LOG_INFO,"FastCGI - directory created %s\n", nginxdir);
	if((fastcgi_conf_file = fopen(fastcgiconf, "w")) == NULL) {
//		notice_set("FastCGI","config file %s has been created\n", fastcgiconf);
		simple_unlock(fastcgiconf);
	return 0;
	}

	fastcgi_write("# FastCGI generated config file\n");
	fastcgi_write("fastcgi_param SCRIPT_FILENAME\t\t$document_root$fastcgi_script_name;\n");
	fastcgi_write("fastcgi_param QUERY_STRING\t\t$query_string;\n");
	fastcgi_write("fastcgi_param REQUEST_METHOD\t\t$request_method;\n");
	fastcgi_write("fastcgi_param CONTENT_TYPE\t\t$content_type;\n");
	fastcgi_write("fastcgi_param CONTENT_LENGTH\t\t$content_length;\n");
	fastcgi_write("fastcgi_param SCRIPT_NAME\t\t$fastcgi_script_name;\n");
	fastcgi_write("fastcgi_param REQUEST_URI\t\t$request_uri;\n");
	fastcgi_write("fastcgi_param DOCUMENT_URI\t\t$document_uri;\n");
	fastcgi_write("fastcgi_param DOCUMENT_ROOT\t\t$document_root;\n");
	fastcgi_write("fastcgi_param SERVER_PROTOCOL\t\t$server_protocol;\n");
	fastcgi_write("fastcgi_param GATEWAY_INTERFACE\t\tCGI/1.1;\n");
	fastcgi_write("fastcgi_param SERVER_SOFTWARE\t\tnginx/$nginx_version;\n");
	fastcgi_write("fastcgi_param REMOTE_ADDR\t\t$remote_addr;\n");
	fastcgi_write("fastcgi_param REMOTE_PORT\t\t$remote_port;\n");
	fastcgi_write("fastcgi_param SERVER_ADDR\t\t$server_addr;\n");
	fastcgi_write("fastcgi_param SERVER_PORT\t\t$server_port;\n");
	fastcgi_write("fastcgi_param SERVER_NAME\t\t$server_name;\n");

	fastcgi_write("fastcgi_index index.php;\n");
	fastcgi_write("fastcgi_param REDIRECT_STATUS		200;\n");

	fclose(fastcgi_conf_file);
//	syslog(LOG_INFO,"FastCGI - config file finished succesfully!!\n");
	fprintf(stderr, "Wrote: %s\n", fastcgiconf);

	return 0;
}

int build_mime_types(void) {
	unsigned int i; //integer cast

static const char *nginxdmimetypes[] = { 
					"text/html\t\t\t\thtml htm shtml",
					"text/css\t\t\t\tcss",
					"text/xml\t\t\t\txml rss",
					"image/gif\t\t\t\tgif",
					"image/jpeg\t\t\t\tjpeg jpg",
					"application/x-javascript\t\t\t\tjs",
					"text/plain\t\t\t\ttxt",
					"text/x-component\t\t\t\thtc",
					"text/mathml\t\t\t\tmml",
					"image/png\t\t\t\tpng",
					"image/x-icon\t\t\t\tico",
					"image/x-jng\t\t\t\tjng",
					"image/vnd.wap.wbmp\t\t\t\twbmp",
					"application/java-archive\t\t\t\tjar war ear",
					"application/mac-binhex40\t\t\t\thqx",
					"application/pdf\t\t\t\tpdf",
					"application/x-cocoa\t\t\t\tcco",
					"application/x-java-archive-diff\t\t\t\tjardiff",
					"application/x-java-jnlp-file\t\t\t\tjnlp",
					"application/x-makeself\t\t\t\trun",
					"application/x-perl\t\t\t\tpl pm",
					"application/x-pilot\t\t\t\tprc pdb",
					"application/x-rar-compressed\t\t\t\trar",
					"application/x-redhat-package-manager\t\t\t\trpm",
					"application/x-sea\t\t\t\tsea",
					"application/x-shockwave-flash\t\t\t\tswf",
					"application/x-stuffit\t\t\t\tsit",
					"application/x-tcl\t\t\t\ttcl tk",
					"application/x-x509-ca-cert\t\t\t\tder pem crt",
					"application/x-xpinstall\t\t\t\txpi",
					"application/zip\t\t\t\tzip",
					"application/octet-stream\t\t\t\tdeb",
					"application/octet-stream\t\t\t\tbin exe dll",
					"application/octet-stream\t\t\t\tdmg",
					"application/octet-stream\t\t\t\teot",
					"application/octet-stream\t\t\t\tiso img",
					"application/octet-stream\t\t\t\tmsi msp msm",
					"audio/mpeg\t\t\t\tmp3",
					"audio/x-realaudio\t\t\t\tra",
					"video/mpeg\t\t\t\tmpeg mpg",
					"video/quicktime\t\t\t\tmov",
					"video/x-flv\t\t\t\tflv",
					"video/x-msvideo\t\t\t\tavi",
					"video/x-ms-wmv\t\t\t\twmv",
					"video/x-ms-asf\t\t\t\tasx asf",
					"video/x-mng\t\t\t\tmng",
					 NULL };


// Starting the mime.types configuration file
//		syslog(LOG_INFO,"MimeTypes - File generation started\n");

	if(mkdir_if_none(nginxdir));
//		syslog(LOG_INFO,"MimeTypes - directory created %s\n", nginxdir);
	if((mimetypes_file = fopen(mimetypes, "w")) == NULL) {
//		notice_set("MimeTypes","file %s has been created\n", mimetypes);
		simple_unlock(mimetypes);
	return 0;
	}
		mimetypes_write("# Mime.Types generated file\n");
		mimetypes_write("types {\n");
	for(i=0; nginxdmimetypes[i]; i++) { mimetypes_write("\t%s;\n", nginxdmimetypes[i]); }
		mimetypes_write("}\n");
		fclose(mimetypes_file);
//		syslog(LOG_INFO,"MimeTypes - file built succesfully!!\n");
		fprintf(stderr, "Wrote: %s\n", mimetypes);
	return 0;
}

int build_nginx_conf(void) {
		char *buf; //default param buffer
		unsigned int i; //integer cast

// Starting the nginx configuration file
		syslog(LOG_INFO,"NGinX - started generating nginx config file\n");

	if(mkdir_if_none(nginxdir));
//		syslog(LOG_INFO,"NGinX - directory created %s\n", nginxdir);
	if((nginx_conf_file = fopen(nginxconf, "w")) == NULL) {
//		notice_set("NGinX","config file %s has been created\n", nginxconf);
		simple_unlock(nginxconf);
	return 0;
	}
		nginx_write("# NGinX generated config file\n");
//		syslog(LOG_INFO,"NGinX","started writing config file %s\n", nginxconf);

//Global process
		nginx_write("user\t%s;\n", nvram_safe_get("nginx_user"));
		nginx_write("worker_processes\t%s;\n", nginx_worker_proc);
		nginx_write("worker_cpu_affinity\t%s;\n", nginx_cpu_affinity);
		nginx_write("master_process\t%s;\n", nginx_master_process);
		i = nvram_get_int("nginx_priority");
	if ((i <= -20) || (i >= 19)) i = 10; // min = Max Performance and max= Min Performance value for worker_priority	
		nginx_write("worker_priority\t%d;\n", i);
		nginx_write("error_log\t%s;\n", nginxerrorlog);
		nginx_write("pid\t%s;\n", nginxpid);
		nginx_write("worker_rlimit_nofile\t%s;\n", nginx_worker_rlimit_profile);
		
//Events		
		nginx_write("events {\n");
		nginx_write("\tworker_connections\t%s;\n", nginx_worker_connections);
//		nginx_write("\tmulti_accept\t%s;\n", nginx_multi_accept);
		nginx_write("\t}\n");
		
//http
		nginx_write("http {\n");
		nginx_write("include\t%s;\n", mimetypes);
		nginx_write("include\t%s;\n", fastcgiconf);
		nginx_write("default_type\tapplication/octet-stream;\n");
		nginx_write("log_format   main '$remote_addr - $remote_user [$time_local]  $status '\n");
		nginx_write("'\"$request\" $body_bytes_sent \"$http_referer\" '\n");
		nginx_write("'\"$http_user_agent\" \"$http_x_forwarded_for\"';\n");
		nginx_write("sendfile\t%s;\n", nginssendfile);

//shibby - add custom config to http section
		nginx_write("client_max_body_size\t%sM;\n", nvram_safe_get("nginx_upload"));

	if ((buf = nvram_safe_get("nginx_httpcustom")) == NULL) buf = nginxcustom;
		nginx_write(buf);
		nginx_write("\n");

//		nginx_write("keepalive_timeout\t%s;\n", nginx_keepalive_timeout);
//		nginx_write("tcp_nopush\t%s;\n", nginxtcp_nopush);
//		nginx_write("server_names_hash_bucket_size\t%s;\n", nginxserver_names_hash_bucket_size);
//		nginx_write("limit_req_zone  $binary_remote_addr  zone=one:10m   rate=1r/s;\n");
		
//Basic Server Parameters.
		nginx_write("server {\n"); 
		i = nvram_get_int("nginx_port");
	if ((i <= 0) || (i >= 0xFFFF)) i = 85; // 0xFFFF 65535
		nginx_write("listen\t%d;\n", i);
	if ((buf = nvram_safe_get("nginx_fqdn")) == NULL) buf = nginxname;
		nginx_write("server_name\t%s;\n", buf);
		nginx_write("access_log\t%s\tmain;\n", nginxaccesslog);
		nginx_write("location\t/\t{\n");
	if ((buf = nvram_safe_get("nginx_docroot")) == NULL) buf = nginxdocrootdir;
		nginx_write("root\t%s;\n", buf);
		nginx_write("index\tindex.html\tindex.htm\tindex.php;\n");
// Error pages section
		nginx_write("error_page 404\t/404.html;\n");
		nginx_write("error_page 500\t502\t503\t504\t/50x.html;\n");
		nginx_write("location\t/50x.html\t{\n");
	if ((buf = nvram_safe_get("nginx_docroot")) == NULL) buf = nginxdocrootdir;
		nginx_write("root\t%s;\n", buf);
		nginx_write("}\n");
// PHP to FastCGI Server
	if( nvram_match( "nginx_php", "1" ) ) {
		nginx_write("location ~ ^(?<script_name>.+?\\.php)(?<path_info>/.*)?$ {\n");
		nginx_write("try_files \t$script_name = 404;\n");
		nginx_write("include\t%s;\n", fastcgiconf);
		nginx_write("fastcgi_param PATH_INFO $path_info;\n");
		nginx_write("fastcgi_pass\t127.0.0.1:9000;\n");
		nginx_write("\t}\n");
	}

// Server for static files
    		nginx_write("location ~ ^/(images|javascript|js|css|flash|media|static)/  {\n");
    		nginx_write("root\t%s;\n", buf);
    		nginx_write("expires 10d;\n");
    		nginx_write("\t\t\t}\n");
		nginx_write("\t\t}\n");

//shibby - add custom config to server section
	if ((buf = nvram_safe_get("nginx_servercustom")) == NULL) buf = nginxcustom;
		nginx_write(buf);
		nginx_write("\n");

		nginx_write("\t}\n");
		nginx_write("}\n");
// Process to close and write config file
		nginx_write("\n");
	if ((buf = nvram_safe_get("nginx_custom")) == NULL) buf = nginxcustom;
		nginx_write(buf);
		fclose(nginx_conf_file);
		syslog(LOG_INFO,"NGinX - config file built succesfully\n");
		fprintf(stderr, "Wrote: %s\n", nginxconf);

//shibby - create php.ini
	if( nvram_match( "nginx_php", "1" ) ) {
		if( !(phpini_file = fopen("/tmp/etc/php.ini", "w")) ) {
			perror( "/tmp/etc/php.ini" );
			return;
		}
		fprintf( phpini_file, "post_max_size = %sM\n", nvram_safe_get("nginx_upload"));
		fprintf( phpini_file, "upload_max_filesize = %sM\n", nvram_safe_get("nginx_upload"));
		fprintf( phpini_file, "mysql.default_port = 3309\n");
		fprintf( phpini_file, "mysql.default_socket = /var/run/mysqld.sock\n");
		fprintf( phpini_file, "%s\n", nvram_safe_get("nginx_phpconf"));
		fclose(phpini_file);
		syslog(LOG_INFO,"NGinX - php.ini file built succesfully\n");
	}
	return 0;
}

// Start the NGINX module according environment directives.
void start_nginx(void)
{
	if (fastpath != 1) {
		if(!nvram_match("nginx_enable", "1")){
		syslog(LOG_INFO,"NGinX - daemon not enabled cancelled generation of config file\n");
		return;
		}
	}else{
		syslog(LOG_INFO,"NGinX - fastpath forced generation of config file\n");
	}

	if (fastpath != 1) {
		stop_nginx();
	}else{
		stop_nginxfp();
	}

	if (!f_exists(fastcgiconf)) build_fastcgi_conf();
	if (!f_exists(mimetypes)) build_mime_types();
	if ((fastpath != 1) && (!nvram_match("nginx_keepconf", "1"))) {
		build_nginx_conf();
	}else{
		if (!f_exists(nginxconf)) build_nginx_conf();
	}

//shibby - create /tmp/var/log/nginx directory before start daemon (if does not exist)
	xstart("mkdir", "-p", "/tmp/var/log/nginx");

	if( nvram_match( "nginx_php", "1" ) ) {
//shibby - run spawn-fcgi
		xstart("spawn-fcgi", "-a", "127.0.0.1", "-p", "9000", "-P", "/var/run/php-fastcgi.pid", "-C", "2", "-u", nvram_safe_get("nginx_user"), "-g", nvram_safe_get("nginx_user"), "-f", "php-cgi");
	} else {
		killall_tk("php-cgi");
	}

	if(mkdir_if_none(client_body_temp_path));
//		syslog(LOG_INFO,"NGinX - directory created %s\n", client_body_temp_path);
	if(mkdir_if_none(fastcgi_temp_path));
//		syslog(LOG_INFO,"NGinX - directory created %s\n", fastcgi_temp_path);
	if(mkdir_if_none(uwsgi_temp_path));
//		syslog(LOG_INFO,"NGinX - directory created %s\n", uwsgi_temp_path);
	if(mkdir_if_none(scgi_temp_path));
//		syslog(LOG_INFO,"NGinX - directory created %s\n", scgi_temp_path);
		syslog(LOG_INFO,"NGinX - running daemon\n");
		if( nvram_match( "nginx_override", "1" ) ) {
			xstart(nginxbin, "-c", nvram_safe_get("nginx_overridefile"));
		} else {
			xstart(nginxbin, "-c", nginxconf);
		}
}
// Start NGinx using fastpath method no checks
void start_nginxfp(void)
{
fastpath = 1;
start_nginx();
fastpath = 0;
}
// Stopping NGINX and remove traces of the process.
void stop_nginx(void)
{
	unsigned int i;

	i = 0;
		syslog(LOG_INFO,"NGinX - killing daemon\n");
	if ((i = pidof (nginxbin)) > 0) {
		killall_tk(nginxbin);
		killall_tk("php-cgi"); //shibby
		if (f_exists(nginxpid)) {
			unlink(nginxpid);
//			syslog(LOG_INFO,"NGinX - removed pid file %s\n", nginxpid);
		}
		if (f_exists(fastcgiconf)) {
			if ((fastpath != 1) && (!nvram_match("nginx_keepconf", "1"))) {
				unlink(fastcgiconf);
//				syslog(LOG_INFO,"NGinX - removed fastcgi config file %s\n", fastcgiconf);
			}
//			syslog(LOG_INFO,"NGinX - skip removal of fastcgi config file %s due to fastpath method\n", fastcgiconf);
		}
		if (f_exists(mimetypes)) {
			if ((fastpath != 1) && (!nvram_match("nginx_keepconf", "1"))) {
				unlink(mimetypes);
//				syslog(LOG_INFO,"NGinX - remove mime types file %s\n", mimetypes);
			}
//			syslog(LOG_INFO,"NGinX - skip removal of mime types config file %s due to fastpath method\n", mimetypes);
		}
		if (f_exists(nginxconf)) {
			if ((fastpath != 1) && (!nvram_match("nginx_keepconf", "1"))) {
			unlink(nginxconf);
//			syslog(LOG_INFO,"NGinX - removed nginx config file %s\n", nginxconf);
			}
//			syslog(LOG_INFO,"NGinX - skip removal of nginx config file %s due to fastpath method\n", nginxconf);
		}
	}
}
// Stop NGinx using fastpath method no checks
void stop_nginxfp(void)
{
fastpath = 1;
stop_nginx();
fastpath = 0;
}

