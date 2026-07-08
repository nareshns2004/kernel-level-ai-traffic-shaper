// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>

static char tc_interface[IFNAMSIZ];
static bool tc_active;

int kernelmind_tc_init(const char *ifname)
{
	struct net_device *dev;

	if (!ifname)
		return -EINVAL;

	dev = dev_get_by_name(&init_net, ifname);
	if (!dev)
		return -ENODEV;

	strscpy(tc_interface, ifname, IFNAMSIZ);
	dev_put(dev);
	tc_active = true;

	pr_info("kernelmind: tc integration ready on %s\n", tc_interface);
	return 0;
}

void kernelmind_tc_exit(void)
{
	tc_active = false;
	tc_interface[0] = '\0';
}

bool kernelmind_tc_is_active(void)
{
	return tc_active;
}

const char *kernelmind_tc_interface(void)
{
	return tc_interface;
}

int kernelmind_tc_set_class_rate(u8 class_id, u32 rate_kbps)
{
	if (!tc_active)
		return -ENODEV;

	pr_debug("kernelmind: set class %u rate to %u kbps on %s\n",
		 class_id, rate_kbps, tc_interface);
	return 0;
}
