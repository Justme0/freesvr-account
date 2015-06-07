#include <string.h>
#include "global.h"

void init_info(Info * const pinfo) {
	memset(pinfo, 0x00, sizeof(*pinfo));
	pinfo->user_type = USER_PRIV_NORMAL;
}
