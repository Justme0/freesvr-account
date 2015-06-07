#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "config.h"

#define BUFSIZE		256
#define FALSE		0
#define TRUE		1

enum vartype { BOOL, STRING, INT };

struct Config g_config;

struct option_array
{
	const char *name;
	enum vartype type;
	void *var;
	char cmdline;
	int reloadable;
	int essential;
};

struct option_array opts[] = {
	{"LogFile", STRING, &g_config.log_file, '\0', FALSE, TRUE},
	{"WriteLocalLog", BOOL, &g_config.write_local_log, '\0', TRUE, TRUE},
	{"MysqlAddress", STRING, &g_config.mysql_address, '\0', TRUE, TRUE},
	{"MysqlUsername", STRING, &g_config.mysql_username, '\0', TRUE, TRUE},
	{"MysqlPassword", STRING, &g_config.mysql_password, '\0', TRUE, TRUE},
	{"MysqlDatabase", STRING, &g_config.mysql_database, '\0', TRUE, TRUE},
	{"Timeout", INT, &g_config.timeout, '\0', TRUE, TRUE},
	{"RetryTimes", INT, &g_config.retry_times, '\0', TRUE, TRUE},
	{0, 0, 0, 0, 0, 0}
};

/* Static Variable */
static FILE *fp = NULL;
static int line_no;
static int rereading = FALSE;
static char cmdline_set[100] = { 0 };

/* Static Function */
static int parse_line(char *);
static int set_opt(struct option_array *, char *);
#ifndef NDEBUG
static int print_config(void);
#endif

int read_config()
{
	struct option_array *opt;
	char *buf;

	if (!fp)
	{
		fp = fopen(CONFIG_FILE, "r");
		if (fp == NULL)
			return -1;
		line_no = 0;
	}

	buf = (char *) malloc(BUFSIZE);
	while (fgets(buf, BUFSIZE, fp) != NULL)
	{
		line_no++;
		switch (parse_line(buf))
		{
			case -1:
				free(buf);
				fclose(fp);
				return (-1);
				break;

			case 0:
				break;

			case 1:				/* end subsection */
				free(buf);
				return (0);
				break;
		}
	}
	free(buf);
	fclose(fp);
	fp = NULL;

	opt = opts;
	while (opt->name != NULL && !opt->essential)
		opt++;
	if (opt->name != NULL)
	{

		fprintf(stderr, "Essential option \"%s\" not specified.\n", opt->name);
		return (-1);
	}

#ifndef NDEBUG
	print_config();
#endif
	return (0);
}

int parse_line(char *buf)
{
	struct option_array *opt;
	char *name, *value, *tok;
	const char *delim = " =\t\r\n";

	if (buf[0] == '#')
		return (0);

	int i, j, len = strlen(buf);

	for (i = 0; i < len && strchr(delim, buf[i]); i++);
	tok = strtok(buf + i, delim);
	if (tok == NULL)
		return (0);
	name = strdup(tok);

	for (j = i + strlen(name); j < len && strchr(delim, buf[j]); j++);
	tok = strtok(buf + j, delim);
	if (tok == NULL)
	{

		fprintf(stderr, "Bad option \"%s\" at line %d of %s\n", buf, line_no, CONFIG_FILE);
		return (-1);
	}
	value = strdup(tok);

	opt = opts;
	while (opt->name != NULL && strcasecmp(name, opt->name))
		opt++;
	if (opt->name == NULL)
	{

		fprintf(stderr, "Unrecognised option \"%s\" at line %d of %s\n", name, line_no,
				CONFIG_FILE);
		return (-1);
	}
	if (opt->cmdline && strchr(cmdline_set, opt->cmdline))
	{

		fprintf(stderr, "\"%s\" specified on the command line: ignored\n", name);
		return (0);
	}
	if (set_opt(opt, value) == -1)
	{

		fprintf(stderr, "Invalid argument to %s at line %d of %s\n", name, line_no, CONFIG_FILE);
		return (-1);
	}
	free(name);
	free(value);
	return (0);
}

int set_opt(struct option_array *opt, char *str)
{
	if (rereading && !opt->reloadable)
		return (0);

	opt->essential = FALSE;

	switch (opt->type)
	{
		case BOOL:
			if (!strcasecmp(str, "yes"))
				*(int *) opt->var = TRUE;
			else if (!strcasecmp(str, "no"))
				*(int *) opt->var = FALSE;
			else
				return (-1);
			break;
		case STRING:
			*(char **) opt->var = strdup(str);
			break;
		case INT:
			*(int *) opt->var = atoi(str);
			break;
		default:
			return (-1);
			break;
	}
	return (0);
}

#ifndef NDEBUG
int print_config(void)
{
	struct option_array *opt;
	opt = opts;
	while (opt->name != NULL)
	{
		switch (opt->type)
		{
			case BOOL:
				fprintf(stderr, "%-30s = %s\n", opt->name, *(int *) opt->var == TRUE ? "YES" : "NO");
				break;
			case STRING:
				fprintf(stderr, "%-30s = %s\n", opt->name, *(char **) opt->var);
				break;
			case INT:
				fprintf(stderr, "%-30s = %d\n", opt->name, *(int *) opt->var);
				break;
			default:
				return -1;
		}
		opt++;
	}
	return 0;
}
#endif

int reload_config(void)
{
	int ret;
	rereading = TRUE;
	ret = read_config();
	return ret;
}
