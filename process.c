#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "connect.h"
#include "db.h"

/*
 * process socket and database.
 * return 0 if successful, otherwise -1.
 */
int process(Info const * const pinfo) {
	int recv_res = -1;
	static char recv_msg[BUFSIZE];
	memset(recv_msg, 0x00, sizeof recv_msg);
	// connect to server
	int ret = connect2svr(pinfo, &recv_res, recv_msg);	// recv_res is rcv_buf[0]
	if (0 != ret) {
		fprintf(stderr, "connect2svr() error\n");
		return -1;
	}

	switch (recv_res) {
		case 0:
			fprintf(stderr, "Server don't execute command.\n");
			return -1;
			break;

		case 1:
			fprintf(stderr, "Server has executed command successfully.\n");
			if (pinfo->operation_type == GET_USERS_FROM_GROUP
				|| pinfo->operation_type == GET_ALL_USERS_FROM_GROUPS) {
				// redundant, no need to write database, but received message was printed to log file
				return 0;
			}

			// update mysql
			if (0 != update_mysql(pinfo, recv_msg)) {
				fprintf(stderr, "update_mysql() error\n");
				return -1;
			}
			return 0;
			break;

		case 2:
			fprintf(stderr, "Server has executed command, but failed.\n");
			return -1;
			break;

		default:
			fprintf(stderr, "connect2svr() receive an invalid result.\n");
			return -1;
			break;
	}
}
