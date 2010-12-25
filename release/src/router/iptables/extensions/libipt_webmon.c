/*  webmon --	An iptables extension to match URLs in HTTP requests 
 *  		This module can match using string match or regular expressions
 *  		Originally designed for use with Gargoyle router firmware (gargoyle-router.com)
 *
 *
 *  Copyright © 2008-2010 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <arpa/inet.h>

/*
 * in iptables 1.4.0 and higher, iptables.h includes xtables.h, which
 * we can use to check whether we need to deal with the new requirements
 * in pre-processor directives below
 */
#include <iptables.h>  
#include <linux/netfilter_ipv4/ipt_webmon.h>

#ifdef _XTABLES_H
	#define iptables_match		xtables_match
#endif

/* 
 * XTABLES_VERSION_CODE is only defined in versions 1.4.1 and later, which
 * also require the use of xtables_register_match
 * 
 * Version 1.4.0 uses register_match like previous versions
 */
#ifdef XTABLES_VERSION_CODE 
	#define register_match          xtables_register_match
#endif

unsigned char* read_entire_file(FILE* in, unsigned long read_block_size, unsigned long *length);

#define DEFAULT_MAX      300

#define SEARCH_LOAD_FILE 100
#define DOMAIN_LOAD_FILE 101
#define CLEAR_SEARCH     102
#define CLEAR_DOMAIN     103

static char* domain_load_file = NULL;
static char* search_load_file = NULL;
static uint32_t global_max_domains  = DEFAULT_MAX;
static uint32_t global_max_searches = DEFAULT_MAX;

/* Function which prints out usage message. */
static void help(void)
{
	printf(	"webmon options:\n");
}

static struct option opts[] = 
{
	{ .name = "max_domains",        .has_arg = 1, .flag = 0, .val = WEBMON_MAXDOMAIN },
	{ .name = "max_searches",       .has_arg = 1, .flag = 0, .val = WEBMON_MAXSEARCH },
	{ .name = "search_load_file",   .has_arg = 1, .flag = 0, .val = SEARCH_LOAD_FILE },
	{ .name = "domain_load_file",   .has_arg = 1, .flag = 0, .val = DOMAIN_LOAD_FILE },
	{ .name = "clear_search",       .has_arg = 0, .flag = 0, .val = CLEAR_SEARCH },
	{ .name = "clear_domain",       .has_arg = 0, .flag = 0, .val = CLEAR_DOMAIN },

	{ .name = 0 }
};

static void webmon_init(
#ifdef _XTABLES_H
	struct xt_entry_match *match
#else
	struct ipt_entry_match *match, unsigned int *nfcache
#endif
	)
{
	struct ipt_webmon_info *info = (struct ipt_webmon_info *)match->data;
	info->max_domains=DEFAULT_MAX;
	info->max_searches=DEFAULT_MAX;
	info->ref_count = NULL;
}


/* Function which parses command options; returns true if it ate an option */
static int parse(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
#ifdef _XTABLES_H
			const void *entry,
#else
			const struct ipt_entry *entry,
			unsigned int *nfcache,
#endif			
			struct ipt_entry_match **match
			)
{
	struct ipt_webmon_info *info = (struct ipt_webmon_info *)(*match)->data;
	int valid_arg = 1;
	long max;
	switch (c)
	{
		case WEBMON_MAXSEARCH:
			if( sscanf(argv[optind-1], "%ld", &max) == 0)
			{
				info->max_searches = DEFAULT_MAX ;
				valid_arg = 0;
			}
			else
			{
				info->max_searches = (uint32_t)max;
				global_max_searches = info->max_searches;
			}
			break;
		case WEBMON_MAXDOMAIN:
			if( sscanf(argv[optind-1], "%ld", &max) == 0)
			{
				info->max_domains = DEFAULT_MAX ;
				valid_arg = 0;
			}
			else
			{
				info->max_domains = (uint32_t)max;
				global_max_domains = info->max_domains;
			}
			break;
		case SEARCH_LOAD_FILE:
			search_load_file = strdup(optarg);
			break;
		case DOMAIN_LOAD_FILE:
			domain_load_file = strdup(optarg);
			break;
		case CLEAR_SEARCH:
			search_load_file = strdup("/dev/null");
			break;
		case CLEAR_DOMAIN:
			domain_load_file = strdup("/dev/null");
			break;
		default:
			valid_arg = 0;
	}
	return valid_arg;

}

	
static void print_webmon_args(	struct ipt_webmon_info* info )
{
	printf("--max_domains %ld ", (unsigned long int)info->max_domains);
	printf("--max_searches %ld ", (unsigned long int)info->max_searches);
}


static void do_load(char* file, uint32_t max, unsigned char type)
{
	if(file != NULL)
	{
		unsigned char* data = NULL;
		unsigned long data_length = 0;
		if(strcmp(file, "/dev/null") == 0)
		{
			data = (unsigned char*)malloc(10);
			if(data != NULL)
			{
				uint32_t* maxp = (uint32_t*)(data+1);
				data_length = 3+sizeof(uint32_t);
				data[0] = type;
				*maxp = max;
				data[ sizeof(uint32_t)+1 ] = ' ';
				data[ sizeof(uint32_t)+1 ] = '\0';
			}
		}
		else
		{
			FILE* in = fopen(file, "r");
			if(in != NULL)
			{
				char* file_data = read_entire_file(in, 4096, &data_length);
				fclose(in);
				if(file_data != NULL)
				{
					data_length = strlen(file_data) + sizeof(uint32_t)+2;
					data = (unsigned char*)malloc(data_length);
					if(data != NULL)
					{
						uint32_t* maxp = (uint32_t*)(data+1);
						data[0] = type;
						*maxp = max;
						sprintf( (data+1+sizeof(uint32_t)),  "%s", file_data);
					}
					free(file_data);
				}
			}
		}

		if(data != NULL && data_length > 0)
		{
			int sockfd = -1;
			sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
			if(sockfd >= 0)
			{
				setsockopt(sockfd, IPPROTO_IP, WEBMON_SET, data, data_length);
			}
		}
		if(data != NULL)
		{
			free(data);
		}
	}
}


static void final_check(unsigned int flags)
{
	do_load(domain_load_file, global_max_domains,  WEBMON_DOMAIN);
	do_load(search_load_file, global_max_searches, WEBMON_SEARCH);
}

/* Prints out the matchinfo. */
#ifdef _XTABLES_H
static void print(const void *ip, const struct xt_entry_match *match, int numeric)
#else	
static void print(const struct ipt_ip *ip, const struct ipt_entry_match *match, int numeric)
#endif
{
	printf("WEBMON ");
	struct ipt_webmon_info *info = (struct ipt_webmon_info *)match->data;

	print_webmon_args(info);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
#ifdef _XTABLES_H
static void save(const void *ip, const struct xt_entry_match *match)
#else
static void save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
#endif
{
	struct ipt_webmon_info *info = (struct ipt_webmon_info *)match->data;
	print_webmon_args(info);
}

static struct iptables_match webmon = 
{ 
	.next		= NULL,
 	.name		= "webmon",
	#ifdef XTABLES_VERSION_CODE
		.version = XTABLES_VERSION,
	#else
		.version = IPTABLES_VERSION,
	#endif
	.size		= IPT_ALIGN(sizeof(struct ipt_webmon_info)),
	.userspacesize	= IPT_ALIGN(sizeof(struct ipt_webmon_info)),
	.help		= &help,
	.init           = &webmon_init,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match(&webmon);
}


unsigned char* read_entire_file(FILE* in, unsigned long read_block_size, unsigned long *length)
{
	int max_read_size = read_block_size;
	unsigned char* read_string = (unsigned char*)malloc(max_read_size+1);
	unsigned long bytes_read = 0;
	int end_found = 0;
	while(end_found == 0)
	{
		int nextch = '?';
		while(nextch != EOF && bytes_read < max_read_size)
		{
			nextch = fgetc(in);
			if(nextch != EOF)
			{
				read_string[bytes_read] = (unsigned char)nextch;
				bytes_read++;
			}
		}
		read_string[bytes_read] = '\0';
		end_found = (nextch == EOF) ? 1 : 0;
		if(end_found == 0)
		{
			unsigned char *new_str;
			max_read_size = max_read_size + read_block_size;
		       	new_str = (unsigned char*)malloc(max_read_size+1);
			memcpy(new_str, read_string, bytes_read);
			free(read_string);
			read_string = new_str;
		}
	}
	*length = bytes_read;
	return read_string;
}

