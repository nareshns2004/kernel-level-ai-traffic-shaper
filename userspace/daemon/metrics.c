/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kmshaperd.h"

static struct kernelmind_global_stats cached_stats;

void metrics_init(void)
{
	memset(&cached_stats, 0, sizeof(cached_stats));
}

void metrics_update(const struct kernelmind_global_stats *stats)
{
	if (stats)
		memcpy(&cached_stats, stats, sizeof(cached_stats));
}

char *metrics_render(void)
{
	char *buf;
	u64 avg_ns = 0;

	if (cached_stats.inference_count)
		avg_ns = cached_stats.inference_total_ns /
			 cached_stats.inference_count;

	if (asprintf(&buf,
		     "# HELP kernelmind_flows_classified_total Total flows\n"
		     "# TYPE kernelmind_flows_classified_total counter\n"
		     "kernelmind_flows_classified_total %llu\n"
		     "# HELP kernelmind_inference_latency_ns Avg latency\n"
		     "# TYPE kernelmind_inference_latency_ns gauge\n"
		     "kernelmind_inference_latency_ns %llu\n"
		     "# HELP kernelmind_anomalies_total Anomalies\n"
		     "# TYPE kernelmind_anomalies_total counter\n"
		     "kernelmind_anomalies_total %llu\n",
		     cached_stats.flows_classified, avg_ns,
		     cached_stats.anomalies_detected) < 0)
		return NULL;

	return buf;
}
