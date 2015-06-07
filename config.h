#ifndef CONFIG_H
#define CONFIG_H	/* + To stop multiple inclusions. + */

struct Config
{
	char *log_file;
	int write_local_log;
	char *mysql_address;
	char *mysql_username;
	char *mysql_password;
	char *mysql_database;
	int timeout;
	int retry_times;
};

extern struct Config g_config;

int read_config(void);
int reload_config(void);

#endif
