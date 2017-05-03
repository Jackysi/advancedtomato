/*
 * mysql.c
 *
 * Copyright (C) 2014 bwq518, Hyzoom
 *
 */
#include <stdlib.h>
#include <rc.h>
#include <shutils.h>
#include <utils.h>
#include <syslog.h>
#include <sys/stat.h>
#include <shared.h>

void start_mysql(void)
{
	FILE *fp;
	char pbi[128];
	char ppr[64];
	char pdatadir[256], ptmpdir[256];
	char full_datadir[256], full_tmpdir[256], basedir[256];
	char tmp1[256], tmp2[256];

// make sure its really stop
	stop_mysql();
//only if enable...
	if( !nvram_match( "mysql_enable", "1" ) ) return;

	if (nvram_match( "mysql_binary", "internal" ) )
	{
		strcpy(pbi,"/usr/bin");
	}
	else if (nvram_match( "mysql_binary", "optware" ) )
	{
		strcpy(pbi,"/opt/bin");
	}
	else
	{
		strcpy(pbi,nvram_safe_get( "mysql_binary_custom" ));
	}
	if(pbi[strlen(pbi)-1] =='/') pbi[strlen(pbi) - 1] ='\0';
	splitpath(pbi, basedir, tmp1);

	/* Generate download saved path based on USB partition (mysql_dlroot) and directory name (mysql_datadir) */
	if (nvram_match("mysql_usb_enable", "1"))
	{
		tmp1[0] = 0;
		tmp2[0] = 0;
		strcpy(tmp1,nvram_safe_get("mysql_dlroot"));
		trimstr(tmp1);
		if ( tmp1[0] != '/')
		{
			sprintf(tmp2, "/%s", tmp1);
			strcpy(tmp1,tmp2);
		}
		strcpy(ppr, tmp1);
		if(ppr[strlen(ppr)-1] == '/')
		{
			ppr[strlen(ppr)-1] = 0;
		}
		if(strlen(ppr) == 0)
		{
			syslog(LOG_ERR, "Found no mounted USB partition. You must mount a USB disk first." );
			return;
		}
	}
	else
	{
		ppr[0] = '\0';
	}

	strcpy(pdatadir, nvram_safe_get("mysql_datadir"));
	trimstr(pdatadir);
	if(pdatadir[strlen(pdatadir)-1] == '/')
	{
		pdatadir[strlen(pdatadir)-1] = 0;
	}
	if(strlen(pdatadir) == 0)
	{
		strcpy (pdatadir, "data");
		nvram_set("mysql_dir","data");
	}
	if(pdatadir[0] == '/')
	{
		sprintf(full_datadir,"%s%s",ppr,pdatadir);
	}
	else
	{
		sprintf(full_datadir,"%s/%s",ppr,pdatadir);
	}

	strcpy(ptmpdir, nvram_safe_get("mysql_tmpdir"));
	trimstr(ptmpdir);
	if(ptmpdir[strlen(ptmpdir)-1] == '/')
	{
		ptmpdir[strlen(ptmpdir)-1] = 0;
	}
	if(strlen(ptmpdir) == 0)
	{
		strcpy (ptmpdir, "tmp");
		nvram_set("mysql_tmpdir","tmp");
	}
	if(ptmpdir[0] == '/')
	{
		sprintf(full_tmpdir,"%s%s",ppr,ptmpdir);
	}
	else
	{
		sprintf(full_tmpdir,"%s/%s",ppr,ptmpdir);
	}

	// config file  /etc/my.cnf
	if( !( fp = fopen( "/etc/my.cnf", "w" ) ) )
	{
		syslog(LOG_ERR,  "Can not open /etc/my.cnf for writing." );
		return;
	}
	fprintf(fp, "[client]\n");
	fprintf(fp, "port            = %s\n", nvram_safe_get("mysql_port"));
	fprintf(fp, "socket          = /var/run/mysqld.sock\n\n");
	fprintf(fp, "[mysqld]\n");
	fprintf(fp, "user            = root\n");
	fprintf(fp, "socket          = /var/run/mysqld.sock\n");
	fprintf(fp, "port            = %s\n", nvram_safe_get("mysql_port"));
	fprintf(fp, "basedir         = %s\n\n", basedir);
	fprintf(fp, "datadir         = %s\n", full_datadir);
	fprintf(fp, "tmpdir          = %s\n\n", full_tmpdir);
	fprintf(fp, "skip-external-locking\n");
	if (nvram_match("mysql_allow_anyhost", "1")) fprintf(fp, "bind-address            = 0.0.0.0\n");
	else fprintf(fp, "bind-address            = 127.0.0.1\n");
	fprintf(fp, "key_buffer_size         = %sM\n", nvram_safe_get("mysql_key_buffer"));
	fprintf(fp, "max_allowed_packet      = %sM\n", nvram_safe_get("mysql_max_allowed_packet"));
	fprintf(fp, "thread_stack            = %sK\n", nvram_safe_get("mysql_thread_stack"));
	fprintf(fp, "thread_cache_size       = %s\n\n", nvram_safe_get("mysql_thread_cache_size"));
	fprintf(fp, "table_open_cache        = %s\n", nvram_safe_get("mysql_table_open_cache"));
	fprintf(fp, "sort_buffer_size        = %sK\n", nvram_safe_get("mysql_sort_buffer_size"));
	fprintf(fp, "read_buffer_size        = %sK\n", nvram_safe_get("mysql_read_buffer_size"));
	fprintf(fp, "read_rnd_buffer_size    = %sK\n", nvram_safe_get("mysql_read_rnd_buffer_size"));
	fprintf(fp, "query_cache_size        = %sM\n", nvram_safe_get("mysql_query_cache_size"));
	fprintf(fp, "max_connections         = %s\n", nvram_safe_get("mysql_max_connections"));
	fprintf(fp, "#The following items are from mysql_server_custom\n");
	fprintf(fp, "%s\n", nvram_safe_get("mysql_server_custom"));
	fprintf(fp, "#end of mysql_server_custom\n\n");
	fprintf(fp, "[mysqldump]\n");
	fprintf(fp, "quick\nquote-names\n");
	fprintf(fp, "max_allowed_packet      = 16M\n");
	fprintf(fp, "[mysql]\n\n");
	fprintf(fp, "[isamchk]\n");
	fprintf(fp, "key_buffer_size         = 8M\n");
	fprintf(fp, "sort_buffer_size        = 8M\n");
	fprintf(fp, "\n");
	fclose(fp);

	//start file
	if( !( fp = fopen( "/tmp/start_mysql.sh", "w" ) ) )
	{
		syslog(LOG_ERR,  "Can not open /tmp/start_mysql.sh for writing." );
		return;
	}
	fprintf( fp, "#!/bin/sh\n\n" );
	fprintf( fp, "BINPATH=%s\n", pbi);
	fprintf( fp, "PID=/var/run/mysqld.pid\n");
	fprintf( fp, "NEW_INSTALL=0\n");
	fprintf( fp, "MYLOG=/var/log/mysql.log\n");
	fprintf( fp, "ROOTNAME=`nvram get mysql_username`\n");
	fprintf( fp, "ROOTPASS=`nvram get mysql_passwd`\n");
	fprintf( fp, "NGINX_DOCROOT=`nvram get nginx_docroot`\n");
	fprintf( fp, "ANYHOST=`nvram get mysql_allow_anyhost`\n");
	fprintf( fp, "alias elog=\"logger -t mysql -s\"\n");
	fprintf( fp, "sleep %s\n", nvram_safe_get( "mysql_sleep" ) );
	fprintf( fp, "rm -f $MYLOG\n");
	fprintf( fp, "touch $MYLOG\n");
	fprintf( fp, "if [ ! -d \"%s\" ]; then\n", full_datadir);
	fprintf( fp, "  elog \"datadir in /etc/my.cnf doesn't exist. Creating ...\"\n");
	fprintf( fp, "  mkdir -p %s\n", full_datadir);
	fprintf( fp, "  if [ -d \"%s\" ]; then\n", full_datadir);
	fprintf( fp, "    elog \"Created successfully.\"\n");
	fprintf( fp, "  else\n");
	fprintf( fp, "    elog \"Created failed. exit.\"\n");
	fprintf( fp, "    exit 1\n");
	fprintf( fp, "  fi\n");
	fprintf( fp, "fi\n");
	fprintf( fp, "if [ ! -d \"%s\" ]; then\n", full_tmpdir);
	fprintf( fp, "  elog \"tmpdir in /etc/my.cnf doesn't exist. creating ...\"\n");
	fprintf( fp, "  mkdir -p %s\n", full_tmpdir);
	fprintf( fp, "  if [ -d \"%s\" ]; then\n", full_tmpdir);
	fprintf( fp, "    elog \"Created successfully.\"\n");
	fprintf( fp, "  else\n");
	fprintf( fp, "    elog \"Created failed. exit.\"\n");
	fprintf( fp, "    exit 1\n");
	fprintf( fp, "  fi\n");
	fprintf( fp, "fi\n");
	fprintf( fp, "if [ ! -f \"%s/mysql/tables_priv.MYD\" ]; then\n", full_datadir);
	fprintf( fp, "  NEW_INSTALL=1\n");
	fprintf( fp, "  echo \"=========Found NO tables_priv.MYD====================\" >> $MYLOG\n");
	fprintf( fp, "  echo \"This is new installed MySQL.\" >> $MYLOG\n");
	fprintf( fp, "fi\n");
	fprintf( fp, "REINIT_PRIV_TABLES=`nvram get mysql_init_priv`\n");
	fprintf( fp, "if [[ $REINIT_PRIV_TABLES -eq 1 || $NEW_INSTALL -eq 1 ]]; then\n");
	fprintf( fp, "  echo \"=========mysql_install_db====================\" >> $MYLOG\n");
	fprintf( fp, "  $BINPATH/mysql_install_db --user=root --force >> $MYLOG 2>&1\n");
	fprintf( fp, "  elog \"Privileges table was already initialized.\"\n");
	fprintf( fp, "  nvram set mysql_init_priv=0\n");
	fprintf( fp, "  nvram commit\n");
	fprintf( fp, "fi\n");
	fprintf( fp, "REINIT_ROOT_PASSWD=`nvram get mysql_init_rootpass`\n");
	fprintf( fp, "if [[ $REINIT_ROOT_PASSWD -eq 1 || $NEW_INSTALL -eq 1 ]]; then\n");
	fprintf( fp, "  echo \"=========mysqld skip-grant-tables==================\" >> $MYLOG\n");
	fprintf( fp, "  nohup $BINPATH/mysqld --skip-grant-tables --skip-networking --pid-file=$PID >> $MYLOG 2>&1 &\n");
	fprintf( fp, "  sleep 2\n");
	fprintf( fp, "  [ -f /tmp/setpasswd.sql ] && rm -f /tmp/setpasswd.sql\n");
	fprintf( fp, "  echo \"use mysql;\" > /tmp/setpasswd.sql\n");
	fprintf( fp, "  echo \"update user set password=password('$ROOTPASS') where user='root';\" >> /tmp/setpasswd.sql\n");
	fprintf( fp, "  echo \"flush privileges;\" >> /tmp/setpasswd.sql\n");
	fprintf( fp, "  echo \"=========mysql < /tmp/setpasswd.sql====================\" >> $MYLOG\n");
	fprintf( fp, "  $BINPATH/mysql < /tmp/setpasswd.sql >> $MYLOG 2>&1\n");
	fprintf( fp, "  echo \"=========mysqldadmin shutdown====================\" >> $MYLOG\n");
	fprintf( fp, "  $BINPATH/mysqladmin -uroot -p\"$ROOTPASS\" --shutdown_timeout=3 shutdown >> $MYLOG 2>&1\n");
	fprintf( fp, "  killall mysqld\n");
	fprintf( fp, "  rm -f $PID /tmp/setpasswd.sql\n");
	fprintf( fp, "  nvram set mysql_init_rootpass=0\n");
	fprintf( fp, "  nvram commit\n");
	fprintf( fp, "  elog \"root password was already re-initialized.\"\n");
	fprintf( fp, "fi\n");
	fprintf( fp, "\n");
	fprintf( fp, "echo \"=========mysqld startup====================\" >> $MYLOG\n");
	fprintf( fp, "nohup $BINPATH/mysqld --pid-file=$PID >> $MYLOG 2>&1 &\n");
	fprintf( fp, "if [ $ANYHOST -eq 1 ]; then\n");
	fprintf( fp, "  sleep 3\n");
	fprintf( fp, "  [ -f /tmp/setanyhost.sql ] && rm -f /tmp/setanyhost.sql\n");
	fprintf( fp, "  echo \"GRANT ALL PRIVILEGES ON *.* TO 'root'@'%%' WITH GRANT OPTION;\" >> /tmp/setanyhost.sql\n");
	fprintf( fp, "  echo \"flush privileges;\" >> /tmp/setanyhost.sql\n");
	fprintf( fp, "  echo \"=========mysql < /tmp/setanyhost.sql====================\" >> $MYLOG\n");
	fprintf( fp, "  $BINPATH/mysql -uroot -p\"$ROOTPASS\" < /tmp/setanyhost.sql >> $MYLOG 2>&1\n");
	fprintf( fp, "fi\n");
	fprintf( fp, "/usr/bin/mycheck addcru\n");
	fprintf( fp, "elog \"MySQL successfully started.\"\n");
	fprintf( fp, "mkdir -p $NGINX_DOCROOT\n");
	fprintf( fp, "cp -p /www/adminer.php $NGINX_DOCROOT/\n");
	fclose( fp );

	chmod( "/tmp/start_mysql.sh", 0755 );
	xstart( "/tmp/start_mysql.sh" );
	return;

}

void stop_mysql(void)
{
	FILE *fp;
	char pbi[128];

	if (nvram_match( "mysql_binary", "internal" ) )
	{
		strcpy(pbi,"/usr/bin");
	}
	else if (nvram_match( "mysql_binary", "optware" ) )
	{
		strcpy(pbi,"/opt/bin");
	}
	else
	{
		strcpy(pbi,nvram_safe_get( "mysql_binary_custom" ));
	}
	//stop file
	if( !( fp = fopen( "/tmp/stop_mysql.sh", "w" ) ) )
	{
		syslog(LOG_ERR, "Can not open /tmp/stop_mysql.sh for writing." );
		return;
	}

	fprintf( fp, "#!/bin/sh\n\n" );
	fprintf( fp, "%s/mysqladmin -uroot -p\"%s\" --shutdown_timeout=3 shutdown\n", pbi, nvram_safe_get("mysql_passwd"));
	fprintf( fp, "killall mysqld\n" );
	fprintf( fp, "logger \"MySQL successfully stopped\" \n");
	fprintf( fp, "sleep 1\n");
	fprintf( fp, "rm -f /var/run/mysql.pid\n");
	fprintf( fp, "/usr/bin/mycheck addcru\n");

	fclose( fp );
	chmod( "/tmp/stop_mysql.sh", 0755 );

	xstart( "/tmp/stop_mysql.sh" );
	return;
}

