#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include "global.h"
#include "process.h"
#include "db.h"
#include "log.h"

#include "mysql.h"

/*
 * parameter	: arr ends with NULL
 * return	: find wether arr has value
 */
bool has(char **arr, char *value) {
	// TODO: test when arr is empty (no user or group)
	assert(NULL != arr && NULL != value);

	for (char **p = arr; *p != NULL; ++p) {
		if (strcmp(value, *p) == 0) {
			return true;
		}
	}

	return false;
}

/*
 * write mysql error to log
 */
void print_sql_err(MYSQL *psql) {
	write_log("query database failed and the error information is:\n%s\n", mysql_error(psql));
}

/*
 * get last 'update' line id (similar to mysql_insert_id)
 */
int get_update_id(MYSQL * const psql, int *pupdate_id) {
	mysql_next_result(psql);
	mysql_next_result(psql);

	MYSQL_RES *result = mysql_store_result(psql);
	if (NULL == result) {
		print_sql_err(psql);
		return -1;
	}
	MYSQL_ROW row = mysql_fetch_row(result);
	int update_id = atoi(row[0]);

#ifndef NDEBUG
	printf("update id is %d\n", update_id);
#endif
	mysql_free_result(result);

	*pupdate_id = update_id;

	return 0;
}

int get_mark_or_action(Info const * const pinfo) {
	switch (pinfo->operation_type) {
		case USER_DELETE:
			return 1;
			break;

		case USER_ENABLE:
			return 4;
			break;

		case USER_DISABLE:
			return 3;
			break;

		default:
			assert(!"No need to get operation type.");
			return -1;
			break;
	}
}

int user_create(MYSQL *psql, Info const * const pinfo) {
	char sql_stat[BUFSIZE] = { '\0' };

	// data table
	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_user(username, createtime, changetime, mark)	\
			VALUES('%s', NOW(), NOW(), 0);", pinfo->username);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// log table
	// TODO: mysql_insert_id() return my_ulonglong
	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_userlog(account_userid, datetime, action)	\
			VALUES(%llu, NOW(), 0);", mysql_insert_id(psql));
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	return 0;
}

int user_delete_enable_or_disable(MYSQL *psql, Info const * const pinfo) {
	char sql_stat[BUFSIZE] = { '\0' };

	// update user mark and change time
	snprintf(sql_stat, sizeof(sql_stat),
			"SET @update_id := 0;	\
			UPDATE account_user SET sid = (SELECT @update_id := sid), changetime = NOW(), mark = %d	\
			WHERE username = '%s' AND mark != 1;	\
			SELECT @update_id;", get_mark_or_action(pinfo), pinfo->username);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	int update_id = 0;
	get_update_id(psql, &update_id);
	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_userlog(account_userid, datetime, action)	\
			VALUES(%d, NOW(), %d);", update_id, get_mark_or_action(pinfo));
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// cancel relationship between the user and some group
	if (pinfo->operation_type == USER_DELETE) {
		sprintf(sql_stat,
				"UPDATE account_usergroup SET action = 1 WHERE account_userid = %d AND action != 1;", update_id);
		if (0 != mysql_query(psql, sql_stat)) {
			print_sql_err(psql);
			return -1;
		}
	}

	return 0;
}

int group_create(MYSQL *psql, Info const * const pinfo) {
	char sql_stat[BUFSIZE] = { '\0' };

	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_group(groupname, createtime, changetime, mark)	\
			VALUES('%s', NOW(), NOW(), 0);", pinfo->groupname);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_grouplog(account_groupid, datetime, action)	\
			VALUES(%llu, NOW(), 0);", mysql_insert_id(psql));
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	return 0;
}

int group_delete(MYSQL *psql, Info const * const pinfo) {
	char sql_stat[BUFSIZE] = { '\0' };

	// update group mark and change time
	snprintf(sql_stat, sizeof(sql_stat),
			"SET @update_id := 0;	\
			UPDATE account_group SET sid = (SELECT @update_id := sid), changetime = NOW(), mark = 1	\
			WHERE groupname = '%s' AND mark != 1;	\
			SELECT @update_id;", pinfo->groupname);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// write group log
	int update_id = 0;
	get_update_id(psql, &update_id);
	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_grouplog(account_groupid, datetime, action)	\
			VALUES(%d, NOW(), 1);", update_id);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// change user mark
	sprintf(sql_stat,
			"UPDATE account_user SET changetime = NOW(), mark = 2 WHERE mark != 1 AND sid IN	\
			(SELECT account_userid FROM account_usergroup WHERE ACTION != 1 AND account_groupid = %d);", update_id);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// write user log
	sprintf(sql_stat, "INSERT INTO account_userlog(account_userid, DATETIME, ACTION)	\
			(SELECT account_userid, NOW(), 2 FROM account_usergroup WHERE ACTION != 1 AND account_groupid = %d);", update_id);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// cancel relationship between the group and some user
	sprintf(sql_stat,
			"UPDATE account_usergroup SET createtime = NOW(), action = 1 WHERE account_groupid = %d AND action != 1;", update_id);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	return 0;
}

int add_or_remove_user_group(MYSQL *psql, Info const * const pinfo) {
	char sql_stat[BUFSIZE] = { '\0' };
	// update user mark, change time and get userid
	snprintf(sql_stat, sizeof(sql_stat),
			"SET @update_id :=0;	\
			UPDATE account_user SET sid = (SELECT @update_id := sid), changetime = NOW(), mark = 2	\
			WHERE username = '%s' AND mark != 1 LIMIT 1;	\
			SELECT @update_id;", pinfo->username);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}
	int last_userid = 0;
	get_update_id(psql, &last_userid);

	// write user log
	snprintf(sql_stat, sizeof(sql_stat),
			"INSERT INTO account_userlog(account_userid, datetime, action)	\
			VALUES(%d, NOW(), 2);", last_userid);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	// get groupid (NOTE: don't update account_group and account_grouplog)
	snprintf(sql_stat, sizeof(sql_stat),
			"SELECT sid FROM account_group WHERE groupname = '%s' AND mark != 1 LIMIT 1;", pinfo->groupname);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}
	int last_groupid = atoi(mysql_fetch_row(mysql_store_result(psql))[0]);	// TODO: check return status

	// write account_usergroup
	if (pinfo->operation_type == ADD_USER_TO_GROUP) {
		snprintf(sql_stat, sizeof(sql_stat),
				"INSERT INTO account_usergroup(account_userid, account_groupid, createtime, action)	\
				VALUES(%d, %d, NOW(), 0);", last_userid, last_groupid);
	} else {
		assert(REMOVE_USER_FROM_GROUP == pinfo->operation_type);
		sprintf(sql_stat,
		"UPDATE account_usergroup	\
		SET createtime = NOW(), action = 1	\
		WHERE action != 1 AND account_userid = %d AND account_groupid = %d;",
		last_userid, last_groupid);
	}

	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}

	return 0;
}

/*
 * mutate msg to get names from server
 */
void get_names_from_server(char **server, char *msg) {
	// TODO: use strtok()
	int i = 0;
	for (char *p = msg; '\0' != *p; ++p) {
		if (0 == i) {
			server[i] = p;
			++i;
		}
		if (*p == ',') {
			*p = '\0';
			server[i] = p + 1;
			++i;
			assert(*(p + 1) != ',' && *(p + 1) != '\0');
			assert(*(p - 1) != ',' && *(p - 1) != '\0');
		}
	}
	server[i] = NULL;
}

int username2uid(MYSQL *psql, const char *username, int *puid) {
	char sql_stat[BUFSIZE] = { '\0' };
	sprintf(sql_stat, "SELECT sid FROM account_user WHERE username='%s' AND mark != 1 LIMIT 1;", username);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}
	*puid = atoi(mysql_fetch_row(mysql_store_result(psql))[0]);	// TODO: check return status

	return 0;
}

/*
 * query user_group table, get related groupid from userid
 */
int uid2gid(MYSQL *psql, int uid, int *pgid, int *psize) {
	char sql_stat[BUFSIZE] = { '\0' };
	sprintf(sql_stat,
			"SELECT account_groupid	\
			FROM account_usergroup	\
			WHERE account_userid=%d AND ACTION != 1;", uid);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}
	MYSQL_RES *result = mysql_store_result(psql);
	if (result == NULL) {
		print_sql_err(psql);
		return -1;
	}
	MYSQL_ROW row;
	int i = 0;
	while ((row = mysql_fetch_row(result))) {
		assert(NULL != row[0]);
		pgid[i] = atoi(row[0]);
		++i;
	}
	*psize = i;
	mysql_free_result(result);

	return 0;
}

int gid2groupname(MYSQL *psql, int gid, char *groupname) {
	memset(groupname, 0x00, sizeof groupname);

	char sql_stat[BUFSIZE];
	sprintf(sql_stat,
			"SELECT groupname	\
			FROM account_group	\
			WHERE sid = %d;",
			gid);
	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}
	strcpy(groupname, mysql_fetch_row(mysql_store_result(psql))[0]);	// TODO: check return status

	return 0;
}

/*
 * get groups' name of a user
 */
int get_names_from_client2(char **client, MYSQL *psql, const Info * const pinfo) {
	int userid = 0;
	if (-1 == username2uid(psql, pinfo->username, &userid)) {
		return -1;
	}
	int gid_arr[BUFSIZE] = { 0 };
	int size = 0;	// size means number of groups of a user
	if (-1 == uid2gid(psql, userid, gid_arr, &size)) {
		return -1;
	}

	static char s_name_arr[BUFSIZE];	// avoid malloc
	memset(s_name_arr, 0x00, sizeof(s_name_arr));
	char *p = s_name_arr;
	int i;
	for (i = 0; i < size; ++i) {
		char groupname[BUFSIZE] = { '\0' };
		gid2groupname(psql, gid_arr[i], groupname);
		strcpy(p, groupname);
		client[i] = p;
		assert(strlen(p) == strlen(groupname));
		p += (1 + strlen(groupname));
	}
	client[i] = NULL;

	return 0;
}

/*
 * get all group name or user name
 */
int get_names_from_client(char **client, MYSQL *psql, const Info * const pinfo) {
	char sql_stat[BUFSIZE] = { '\0' };
	// mark == 1 means the group was deleted
	switch (pinfo->operation_type) {
		case GET_ALL_GROUPS:
			snprintf(sql_stat, sizeof(sql_stat), "SELECT groupname FROM account_group WHERE mark != 1;");
			break;

		case GET_ALL_USERS:
			snprintf(sql_stat, sizeof(sql_stat), "SELECT username FROM account_user WHERE mark != 1;");
			break;

		case GET_GROUPS_FROM_USER:
			break;

		default:
			assert(!"Invalid operation type.");
			break;
	}

	if (0 != mysql_query(psql, sql_stat)) {
		print_sql_err(psql);
		return -1;
	}
	MYSQL_RES *result = mysql_store_result(psql);
	if (result == NULL) {
		print_sql_err(psql);
		return -1;
	}
	assert(1 == mysql_num_fields(result));
	MYSQL_ROW row;

	static char s_name_arr[BUFSIZE];	// avoid malloc
	memset(s_name_arr, 0x00, sizeof(s_name_arr));
	char *p = s_name_arr;
	int i = 0;
	while ((row = mysql_fetch_row(result))) {
		assert(NULL != row[0]);
		// TODO: can reuse get_names_from_server(char **server, char *msg)
		strcpy(p, row[0]);
		client[i] = p;
		++i;
		p += (1 + strlen(row[0]));
	}
	client[i] = NULL;
	mysql_free_result(result);

	return 0;
}

void print_name(char * const *client) {
#ifndef NDEBUG
	printf("======== names =============\n");
	for (char * const *p = client; *p != NULL; ++p) {
		printf("%s\n", *p);
	}
	printf("============================\n");
#endif
	(void)client;
}

int client_create(char **client, char **server, MYSQL *psql, const Info * const pinfo) {
	for (char **p = server; NULL != *p; ++p) {
		if (!has(client, *p)) {
			Info info;
			init_info(&info);
			switch (pinfo->operation_type) {
				case GET_ALL_GROUPS:
					strcpy(info.groupname, *p);
					if (0 != group_create(psql, &info)) {
						return -1;
					}
					break;

				case GET_ALL_USERS:
					strcpy(info.username, *p);
					if (0 != user_create(psql, &info)) {
						return -1;
					}
					break;

				case GET_GROUPS_FROM_USER:
					info.operation_type = ADD_USER_TO_GROUP; 
					strcpy(info.username, pinfo->username);
					strcpy(info.groupname, *p);
					if (0 != add_or_remove_user_group(psql, &info)) {
						return -1;
					}
					break;

				default:
					assert(!"Invalid operation type.");
					break;
			}
		}
	}

	return 0;
}

int client_delete(char **client, char **server, MYSQL *psql, const Info * const pinfo) {
	// TODO: redundance with client_create()
	// O(m*n), TODO: use hash, O(m+n)
	for (char **p = client; NULL != *p; ++p) {
		if (!has(server, *p)) {
			// *p doesn't exist in server, so delete it
			Info info;
			init_info(&info);
			switch (pinfo->operation_type) {
				case GET_ALL_GROUPS:
					// delete group
					strcpy(info.groupname, *p);
					if (0 != group_delete(psql, &info)) {
						return -1;
					}
					break;

				case GET_ALL_USERS:
					// delete user
					info.operation_type = USER_DELETE;	// key, used for user_delete_enable_or_disable() determine
					strcpy(info.username, *p);
					if (0 != user_delete_enable_or_disable(psql, &info)) {
						return -1;
					}
					break;

				case GET_GROUPS_FROM_USER:
					// remove user from group
					info.operation_type = REMOVE_USER_FROM_GROUP; 
					strcpy(info.username, pinfo->username);
					strcpy(info.groupname, *p);
					if (0 != add_or_remove_user_group(psql, &info)) {
						return -1;
					}
					break;

				default:
					assert(!"Invalid operation type.");
					break;
			}
		}
	}

	return 0;
}

char * g_server[BUFSIZE];

int get_all_users_or_groups(MYSQL *psql, Info const * const pinfo, char *msg) {
	assert(NULL != psql && NULL != pinfo && NULL != msg);

	char *client[BUFSIZE] = { NULL };
	memset(g_server, 0x00, sizeof g_server);

	// 1
	get_names_from_server(g_server, msg);
#ifndef NDEBUG
	printf("server names:\n");
	print_name(g_server);
#endif

	// 2
	if (0 != get_names_from_client(client, psql, pinfo)) {
		write_log("get_names_from_client() failed.\n");
		return -1;
	}
#ifndef NDEBUG
	printf("client names:\n");
	print_name(client);
#endif

	// 3
	if (0 != client_create(client, g_server, psql, pinfo)) {
		write_log("client_create() failed.\n");
		return -1;
	}

	// 4
	if (0 != client_delete(client, g_server, psql, pinfo)) {
		write_log("client_delete() failed.\n");
		return -1;
	}

	return 0;
}

int get_groups_from_user(MYSQL *psql, Info const * const pinfo, char * const msg) {
	char *server[BUFSIZE] = { NULL };
	char *client[BUFSIZE] = { NULL };

	get_names_from_server(server, msg);
#ifndef NDEBUG
	printf("the groups to which %s belongs in server:\n", pinfo->username);
	print_name(server);
#endif
	if (0 != get_names_from_client2(client, psql, pinfo)) {
		write_log("get_names_from_client2() failed.\n");
		return -1;
	}
#ifndef NDEBUG
	printf("the groups to which %s belongs in client:\n", pinfo->username);
	print_name(client);
#endif
	if (0 != client_create(client, server, psql, pinfo)) {
		write_log("client_create() failed.\n");
		return -1;
	}

	if (0 != client_delete(client, server, psql, pinfo)) {
		write_log("client_delete() failed.\n");
		return -1;
	}

	return 0;
}

void save_server_name(char **server, char *msg) {
	server[0] = msg;
	// *g_server points to server name msg (this msg is defined static in process())
	int i = 0;
	for (; NULL != g_server[i]; ++i) {
		strcpy(server[i], g_server[i]);
		server[1 + i] = server[i] + strlen(server[i]) + 1;
	}
	server[i] = NULL;
}

int get_all_groups_from_users(Info const * const pinfo) {
	Info info;
	init_info(&info);
	strcpy(info.server_ip, pinfo->server_ip);

	// get all groups
	info.operation_type = GET_ALL_GROUPS;
	if (0 != process(&info)) {
		return -1;
	}

	// get all users
	info.operation_type = GET_ALL_USERS;
	if (0 != process(&info)) {
		return -1;
	}

	char msg[BUFSIZE] = { '\0' };
	char * server[BUFSIZE] = { NULL };
	save_server_name(server, msg);

	// get groups of users
	info.operation_type = GET_GROUPS_FROM_USER;
	for (char **p = server; *p != NULL; ++p) {
#ifndef NDEBUG
		printf("----------print rest of server------\n");
		print_name(p);
		printf("------------------------------------\n");
#endif
		strcpy(info.username, *p);
		if (0 != process(&info)) {
			return -1;
		}
	}

	return 0;
}

int update_mysql(Info const * const pinfo, char *msg) {
	assert(NULL != pinfo && NULL != msg);

	MYSQL *psql = mysql_init(NULL);
	if (NULL == psql) {
		write_log("mysql init failed.\n");
		exit(1);
	}
	if (NULL == mysql_real_connect(psql,
				g_config.mysql_address,
				g_config.mysql_username,
				g_config.mysql_password,
				g_config.mysql_database,
				0, NULL, CLIENT_MULTI_STATEMENTS)) {
		print_sql_err(psql);
		return -1;
	}

	write_log("operate database...\n");
	int ret = 0;
	switch (pinfo->operation_type) {
		case USER_CREATE:
			ret = user_create(psql, pinfo);
			break;

		case USER_DELETE:
		case USER_ENABLE:
		case USER_DISABLE:
			ret = user_delete_enable_or_disable(psql, pinfo);
			break;

		case GROUP_CREATE:
			ret = group_create(psql, pinfo);
			break;

		case GROUP_DELETE:
			ret = group_delete(psql, pinfo);
			break;

		case ADD_USER_TO_GROUP:
		case REMOVE_USER_FROM_GROUP:
			ret = add_or_remove_user_group(psql, pinfo);
			break;

		case GET_ALL_USERS:
		case GET_ALL_GROUPS:
			ret = get_all_users_or_groups(psql, pinfo, msg);
			break;

		case GET_GROUPS_FROM_USER:
			ret = get_groups_from_user(psql, pinfo, msg);
			break;

		case GET_USERS_FROM_GROUP:
		case GET_ALL_USERS_FROM_GROUPS:
			break;

		case GET_ALL_GROUPS_FROM_USERS:
			ret = get_all_groups_from_users(pinfo);
			break;

		default:
			assert(!"Invalid operation type.");
			break;
	}
	mysql_close(psql);

	return ret;
}
