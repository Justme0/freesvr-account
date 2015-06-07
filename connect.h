#ifndef _CONNECT_H_
#define _CONNECT_H_

#include "global.h"

#define SERVER_PORT	36137

int connect2svr(Info const * const pinfo, int *presult, char *recv_msg);

#endif
