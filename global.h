#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "config.h"
#include "log.h"

#define BUFSIZE		2048

#define SERVER_IP_MAX	64
#define USERNAME_MAX	64
#define GROUPNAME_MAX	64
#define PASSWORD_MAX	32

enum OperationType {
	USER_CREATE = 3,
	USER_DELETE,
	USER_ENABLE,
	USER_DISABLE,

	GROUP_CREATE,
	GROUP_DELETE,

	ADD_USER_TO_GROUP,
	REMOVE_USER_FROM_GROUP = 16,

// We provide extra four functions:
	GET_ALL_USERS,
	GET_ALL_GROUPS,
	GET_USERS_FROM_GROUP,
	GET_GROUPS_FROM_USER,

	GET_ALL_USERS_FROM_GROUPS,
	GET_ALL_GROUPS_FROM_USERS
};

enum UserType {
	USER_PRIV_GUEST,
	USER_PRIV_NORMAL,
	USER_PRIV_ADMIN
};

typedef struct Info {
	enum OperationType operation_type;
	char server_ip[SERVER_IP_MAX];
	char username[USERNAME_MAX];
	char password[PASSWORD_MAX];
	char groupname[GROUPNAME_MAX];
	enum UserType user_type;
} Info;

void init_info(Info * const pinfo);

#endif
