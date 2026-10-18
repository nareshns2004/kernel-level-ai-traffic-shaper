/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kernelmind_cli.h"

int cmd_model(int argc, char **argv)
{
	const char *output = NULL;
	const char *input = NULL;
	int i;

	if (argc < 3) {
		fprintf(stderr, "Usage: kernelmind model <export|import> "
			"[--output PATH | --path PATH]\n");
		return 1;
	}

	for (i = 3; i < argc; i++) {
		if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
			output = argv[++i];
		else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc)
			input = argv[++i];
	}

	if (strcmp(argv[2], "export") == 0) {
		printf("Exporting model to %s\n",
		       output ? output : "stdout");
		return kernelmind_nl_request(KERNELMIND_NL_CMD_MODEL_EXPORT,
					     NULL, 0, NULL, 0);
	}

	if (strcmp(argv[2], "import") == 0) {
		if (!input) {
			fprintf(stderr, "import requires --path\n");
			return 1;
		}
		printf("Importing model from %s\n", input);
		return kernelmind_nl_request(KERNELMIND_NL_CMD_MODEL_IMPORT,
					     NULL, 0, NULL, 0);
	}

	fprintf(stderr, "Unknown model subcommand: %s\n", argv[2]);
	return 1;
}
