/* SPDX-License-Identifier: MIT */
#include <stdio.h>

#include "kernelmind_cli.h"

int cmd_stats(int argc, char **argv)
{
	struct kernelmind_global_stats stats = {};

	(void)argc;
	(void)argv;

	if (kernelmind_nl_request(KERNELMIND_NL_CMD_STATS, NULL, 0,
				  &stats, sizeof(stats)) < 0) {
		fprintf(stderr, "Failed to query stats (is kernelmind loaded?)\n");
		return 1;
	}

	printf("KernelMind Statistics\n");
	printf("=====================\n");
	printf("Packets processed:   %llu\n", stats.packets_processed);
	printf("Flows classified:    %llu\n", stats.flows_classified);
	printf("Anomalies detected:  %llu\n", stats.anomalies_detected);
	printf("Packets dropped:     %llu\n", stats.packets_dropped);
	printf("Inference count:     %llu\n", stats.inference_count);
	printf("Inference total ns:  %llu\n", stats.inference_total_ns);

	if (stats.inference_count)
		printf("Avg inference ns:    %llu\n",
		       stats.inference_total_ns / stats.inference_count);

	return 0;
}
