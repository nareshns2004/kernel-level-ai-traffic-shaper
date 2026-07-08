/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>

#include "kernelmind_cli.h"

int cmd_flows(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	printf("FLOW_ID          SRC              DST              "
	       "CLASS     PKTS\n");
	printf("----------------------------------------------------------------\n");
	printf("(no active flows — module may not be loaded)\n");

	if (kernelmind_nl_request(KERNELMIND_NL_CMD_FLOWS, NULL, 0,
				  NULL, 0) < 0)
		fprintf(stderr, "warning: netlink request failed\n");

	return 0;
}
