#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iconv.h>

#include "connect.h"
#include "openssl.h"

bool has_data(const char * const msg) {
	return 0 != strcmp(msg, "There is no data.\n");
}

const char * user_type2str(enum UserType user_type) {
	switch (user_type) {
		case USER_PRIV_GUEST:
			return "guest";
			break;

		case USER_PRIV_NORMAL:
			return "normal";
			break;

		case USER_PRIV_ADMIN:
			return "administrator";
			break;

		default:
			assert(!"Invalid user type");
			return "";
			break;
	}
}

const char * operation_type2str(enum OperationType ot) {
	switch (ot) {
		case USER_CREATE:
			return "user create";
			break;

		case USER_DELETE:
			return "user delete";
			break;

		case USER_ENABLE:
			return "user enable";
			break;

		case USER_DISABLE:
			return "user disable";
			break;

		case GROUP_CREATE:
			return "group create";
			break;

		case GROUP_DELETE:
			return "group delete";
			break;

		case ADD_USER_TO_GROUP:
			return "add user to group";
			break;

		case REMOVE_USER_FROM_GROUP:
			return "remove user from group";
			break;

		case GET_ALL_USERS:
			return "get all users";
			break;

		case GET_ALL_GROUPS:
			return "get all groups";
			break;

		case GET_USERS_FROM_GROUP:
			return "get users from group";
			break;

		case GET_GROUPS_FROM_USER:
			return "get groups from user";
			break;

		case GET_ALL_USERS_FROM_GROUPS:
			return "get all users from groups";
			break;

		case GET_ALL_GROUPS_FROM_USERS:
			return "get all groups from users";
			break;

		default:
			assert(!"Invalid operation type.");
			return "";
			break;
	}
}

/*
 * socket
 */
int connect2svr(Info const * const pinfo, int *presult, char *recv_msg)
{
	int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (connect_fd < 0) {
		perror("Failed to create socket.");
		return -1;
	}
	static struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = inet_addr(pinfo->server_ip);
	srv_addr.sin_port = htons(SERVER_PORT);

	struct timeval socket_timeout = { g_config.timeout, 0 };
	if (setsockopt(connect_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &socket_timeout, sizeof(socket_timeout)) < 0) {
		fprintf(stderr, "setsockopt error\n");
		close(connect_fd);
		return -1;
	}
	if (setsockopt(connect_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &socket_timeout, sizeof(socket_timeout)) < 0) {
		fprintf(stderr, "setsockopt error\n");
		close(connect_fd);
		return -1;
	}
	int ret = -1;
	int times = g_config.retry_times;
	while (ret != 0 && times-- > 0) {
		ret = connect(connect_fd, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
	}

	if (ret == -1) {
		fprintf(stderr, "Can't connect to the windows server.\n");
		close(connect_fd);
		return -1;
	}

	// fill string to be sent in snd_buf
	char snd_buf[BUFSIZE] = {'\0'};
	char cip_buf[BUFSIZE] = {'\0'};

	// operation type
	snd_buf[0] = pinfo->operation_type;

	int cu_len = 0;		// cipher name length. [1..2] store name len
	int encrypt_state = 0;
	uint16_t ulen = 0;
	int snd_len = 0;	// length of message to be sent
	int cp_len = 0;
	switch (pinfo->operation_type) {
		case USER_CREATE:
			// encrypt username
			encrypt_state = encrypt(pinfo->username, cip_buf, strlen(pinfo->username), &cu_len);
			assert(0 == encrypt_state);
			// Get the length of name's cipher
			ulen = htons(cu_len);	// NOTE: cu_len maybe 32 bits, while ulen is 16 bits
			// Pad length of name's cipher
			memcpy(&snd_buf[1], &ulen, sizeof(ulen));
			// Pad name's cipher
			memcpy(&snd_buf[3], cip_buf, cu_len);

			// password
			encrypt_state = encrypt(pinfo->password, cip_buf, strlen(pinfo->password), &cp_len);
			assert(0 == encrypt_state);
			ulen = htons(cp_len);
			memcpy(&snd_buf[1 + sizeof(ulen) + cu_len], &ulen, sizeof(ulen));
			memcpy(&snd_buf[1 + sizeof(ulen) * 2 + cu_len], cip_buf, cp_len);

			//length of message to be sent
			snd_len = 1 + cu_len + cp_len + sizeof(ulen) * 2;
			assert('\0' == snd_buf[snd_len]);

			// user_type
			snd_buf[snd_len] = pinfo->user_type;
			assert(1 == sizeof(char));

			snd_len += sizeof(char);
			break;

		case ADD_USER_TO_GROUP:
		case REMOVE_USER_FROM_GROUP:
			encrypt_state = encrypt(pinfo->username, cip_buf, strlen(pinfo->username), &cu_len);
			assert(0 == encrypt_state);
			// Get the length of name's cipher
			ulen = htons(cu_len);	// NOTE: cu_len maybe 32 bits, while ulen is 16 bits
			// Pad length of name's cipher
			memcpy(&snd_buf[1], &ulen, sizeof(ulen));
			// Pad name's cipher
			memcpy(&snd_buf[3], cip_buf, cu_len);

			// groupname
			encrypt_state = encrypt(pinfo->groupname, cip_buf, strlen(pinfo->groupname), &cp_len);
			assert(0 == encrypt_state);
			ulen = htons(cp_len);
			memcpy(&snd_buf[1 + sizeof(ulen) + cu_len], &ulen, sizeof(ulen));
			memcpy(&snd_buf[1 + sizeof(ulen) * 2 + cu_len], cip_buf, cp_len);

			snd_len = 1 + cu_len + cp_len + sizeof(ulen) * 2;
			break;

		case USER_DELETE:
		case USER_ENABLE:
		case USER_DISABLE:
		case GET_GROUPS_FROM_USER:
			// encrypt username
			encrypt_state = encrypt(pinfo->username, cip_buf, strlen(pinfo->username), &cu_len);
			assert(0 == encrypt_state);
			// Get the length of name's cipher
			ulen = htons(cu_len);	// NOTE: cu_len maybe 32 bits, while ulen is 16 bits
			// Pad length of name's cipher
			memcpy(&snd_buf[1], &ulen, sizeof(ulen));
			// Pad name's cipher
			memcpy(&snd_buf[3], cip_buf, cu_len);

			snd_len = 1 + cu_len + sizeof(ulen);
			break;

		case GROUP_CREATE:
		case GROUP_DELETE:
		case GET_USERS_FROM_GROUP:
			// encrypt groupname
			encrypt_state = encrypt(pinfo->groupname, cip_buf, strlen(pinfo->groupname), &cu_len);
			assert(0 == encrypt_state);
			// Get the length of name's cipher
			ulen = htons(cu_len);	// NOTE: cu_len maybe 32 bits, while ulen is 16 bits
			// Pad length of name's cipher
			memcpy(&snd_buf[1], &ulen, sizeof(ulen));
			// Pad name's cipher
			memcpy(&snd_buf[3], cip_buf, cu_len);

			snd_len = 1 + cu_len + sizeof(ulen);
			break;

		case GET_ALL_USERS:
		case GET_ALL_GROUPS:
		case GET_ALL_GROUPS_FROM_USERS:
		case GET_ALL_USERS_FROM_GROUPS:
			snd_len = 1;
			break;

		default:
			assert(!"Invalid operation type.");
			break;
	}
	assert('\0' == snd_buf[snd_len]);

#ifndef NDEBUG
	printf("========== DEBUG MODE ===========\n");
	printf("        all information          \n");
	printf("opertion type   : %s\n", operation_type2str(pinfo->operation_type));
	printf("server ip       : %s\n", pinfo->server_ip);
	printf("username        : %s\n", pinfo->username);
	printf("password        : %s\n", pinfo->password);
	printf("groupname       : %s\n", pinfo->groupname);
	printf("user type       : %s\n\n", user_type2str(pinfo->user_type));
	printf("ciphertext      : ");
	assert('\0' == snd_buf[snd_len]);
	for (int i = 0; i < snd_len; ++i) {
		if (isprint(snd_buf[i])) {
			printf("%c", snd_buf[i]);
		} else {
			printf(".");
		}
	}
	printf("\n=================================\n");
	printf("send...\n");
#endif
	if (send(connect_fd, snd_buf, snd_len, 0) != snd_len) {
		perror("Mismatch in number of sent bytes.");
		return -1;
	}

	sleep(1);

#ifndef NDEBUG
	printf("recv...\n");
#endif
	char rcv_buf[BUFSIZE] = {'\0'};
	/* Recv type */
	ret = recv(connect_fd, rcv_buf, 1, 0);
	if (ret == 0) {
		fprintf(stderr, "Peer shutdown.\n");
		return -1;
	} else if (ret == -1) {
		fprintf(stderr, "An error occurred when recv the reply type from agent.\n");
		return -1;
	} else if (rcv_buf[0] != (char)(pinfo->operation_type)) {
		fprintf(stderr, "Recv an unknown type from agent.\n");
		return -1;
	}

	/* Recv state */
	ret = recv(connect_fd, rcv_buf, 1, 0);
	if (ret == 0) {
		fprintf(stderr, "Peer shutdown when recv the state from agent.\n");
		return -1;
	} else if (ret == -1) {
		fprintf(stderr, "An error occurred when recv the state from agent.\n");
		return -1;
	}

	if (presult != NULL)
		*presult = (int)rcv_buf[0];

	/* Recv message length */
	ret = recv(connect_fd, &ulen, sizeof(ulen), 0);
	if (ret == 0) {
		fprintf(stderr, "Peer shutdown when recv the msg length from agent.\n");
		return -1;
	}
	else if (ret == -1) {
		fprintf(stderr, "An error occurred when recv the msg length from agent.\n");
		return -1;
	}

	/* Get msg length */
	uint16_t mlen = ntohs(ulen);

	/* Recv message */
	memset(rcv_buf, 0x00, sizeof(rcv_buf));
	ret = recv(connect_fd, rcv_buf, mlen, 0);
	if (ret == 0) {
		fprintf(stderr, "Peer shutdown when recv the message from agent.\n");
		return -1;
	} else if (ret == -1) {
		fprintf(stderr, "An error occurred when recv the message from agent.\n");
		return -1;
	}
	memset(cip_buf, 0x00, sizeof(cip_buf));
	int len = 0;
	decrypt(rcv_buf, cip_buf, mlen, &len);

	assert(NULL != recv_msg);
	strcpy(recv_msg, g2u(cip_buf));	// maybe overflow
	fprintf(stderr, "Length of the message received is %d, and the message is:\n%s\n", len, recv_msg);
	if (!has_data(recv_msg)) {
		recv_msg[0] = '\0';	// consistent to the following database operation
	}

	close(connect_fd);

	return 0;
}
