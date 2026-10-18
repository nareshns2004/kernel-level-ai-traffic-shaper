// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/ktime.h>

#include "../ebpf/common.h"

static bool online_learning_enabled = true;

int kernelmind_online_learn(struct kernelmind_feedback *fb)
{
	if (!online_learning_enabled || !fb)
		return -EINVAL;

	if (fb->packet_loss)
		fb->reward_q8 = -32;
	else if (fb->rtt_us > 100000)
		fb->reward_q8 = -16;
	else
		fb->reward_q8 = 16;

	return 0;
}

void kernelmind_online_learn_set_enabled(bool enabled)
{
	online_learning_enabled = enabled;
}

bool kernelmind_online_learn_is_enabled(void)
{
	return online_learning_enabled;
}
