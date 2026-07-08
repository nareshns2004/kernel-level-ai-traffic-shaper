/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernelmind_cli.h"

int cmd_policy(int argc, char **argv)
{
	if (argc < 3 || strcmp(argv[2], "add") != 0) {
		fprintf(stderr, "Usage: kernelmind policy add "
			"--match MATCH --class CLASS [--reason REASON]\n");
		return 1;
	}

	printf("Policy add requested\n");
	return kernelmind_nl_request(KERNELMIND_NL_CMD_POLICY_ADD,
				     NULL, 0, NULL, 0);
}
