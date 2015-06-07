#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <getopt.h>

#include "global.h"
#include "process.h"
#include "log.h"

void print_usage() {
	fprintf(stderr, "Usage: %s [-h] [-o operation_type] [-u username] [-g groupname] [-p password] [-s server_ip]\n", PROGRAM_NAME);
	exit(-1);
}

enum OperationType str2operation_type(const char * const op_type, bool has_username, bool has_groupname) {
	assert(NULL != op_type);
	if ('\0' == *op_type) {
		fprintf(stderr, "invalid operation type\n");
		print_usage();
	}

	if (strcmp("cr", op_type) == 0) {
		// TODO: check all options
		if (has_username) {
			return USER_CREATE;
		} else {
			return GROUP_CREATE;
		}
	}

	if (strcmp("del", op_type) == 0) {
		if (has_username) {
			return USER_DELETE;
		} else {
			return GROUP_DELETE;
		}
	}

	if (strcmp("en", op_type) == 0) {
		return USER_ENABLE;
	}

	if (strcmp("dis", op_type) == 0) {
		return USER_DISABLE;
	}

	if (strcmp("add", op_type) == 0) {
		return ADD_USER_TO_GROUP; 
	}

	if (strcmp("rm", op_type) == 0) {
		return REMOVE_USER_FROM_GROUP;
	}

	if (strcmp("lsu", op_type) == 0) {
		if (has_groupname) {
			// e.g. (Guest: Bob, Jack)
			return GET_USERS_FROM_GROUP;
		} else {
			// e.g. (Bob, Tim)
			return GET_ALL_USERS;
		}
	}

	if (strcmp("lsg", op_type) == 0) {
		if (has_username) {
			// e.g. (Tim: Guest, IIS)
			return GET_GROUPS_FROM_USER;
		} else {
			// e.g. (Guest, Administrator, IIS, vmware)
			return GET_ALL_GROUPS;
		}
	}

	if (strcmp("lsug", op_type) == 0) {
		// e.g. (Tim: Guest, Administrator; Bob: Guest)
		return GET_ALL_GROUPS_FROM_USERS;
	}

	if (strcmp("lsgu", op_type) == 0) {
		return GET_ALL_USERS_FROM_GROUPS;
	}

	fprintf(stderr, "invalid operation type\n");
	print_usage();
	return USER_CREATE;	// avoid warning
}

int main(int argc, char **argv, char *envp[]) {
	init_set_proc_title(argc, argv, envp);
	opterr = 0;
	Info info;
	init_info(&info);
	char op_type[BUFSIZE] = { '\0' };
	bool has_username = false;
	bool has_groupname = false;
	for (int option = 0; -1 != (option = getopt(argc, argv, "o:u:p:g:s:h"));) {
		switch (option) {
			case 'o':
				memset(op_type, 0x00, sizeof(op_type));
				strcpy(op_type, optarg);
				break;

			case 'u':
				has_username = true;
				memset(info.username, 0, sizeof(info.username));
				strcpy(info.username, optarg);
				break;

			case 'p':
				memset(info.password, 0, sizeof(info.password));
				strcpy(info.password, optarg);
				break;

			case 'g':
				has_groupname = true;
				memset(info.groupname, 0, sizeof(info.groupname));
				strcpy(info.groupname, optarg);
				break;

			case 's':
				memset(info.server_ip, 0, sizeof(info.server_ip));
				strcpy(info.server_ip, optarg);
				break;

			case 'h':
				print_usage();
				return 0;
				break;

			default:
				fprintf(stderr, "invalid option\n");
				print_usage();
				return -1;
				break;
		}
	}
	info.operation_type = str2operation_type(op_type, has_username, has_groupname);

	if (read_config() == -1) {
		fprintf(stderr, "Can't read config file.\n");
		return -1;
	}

	if (init_log() == -1) {
		fprintf(stderr, "Can't open log file.\n");
		return -1;
	}

	write_log("/**************************************/");
	if (0 == process(&info)) {
		write_log("Success.\n");
#ifndef NDEBUG
		printf("Success.\n");
#endif
	} else {
		write_log("Execute process() failed.\n");
#ifndef NDEBUG
		printf("Execute process() failed.\n");
#endif
	}

	return 0;
}
