// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include "../../ebpf/common.h"

static void test_flow_classes(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, KM_CLASS_REALTIME, 1);
	KUNIT_EXPECT_EQ(test, KM_CLASS_ANOMALY, 10);
	KUNIT_EXPECT_LT(test, KM_CLASS_MAX, KERNELMIND_NUM_CLASSES);
}

static void test_policy_actions(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, KM_ACTION_ACCEPT, 0);
	KUNIT_EXPECT_EQ(test, KM_ACTION_DROP, 3);
}

static struct kunit_case policy_test_cases[] = {
	KUNIT_CASE(test_flow_classes),
	KUNIT_CASE(test_policy_actions),
	{}
};

static struct kunit_suite policy_test_suite = {
	.name = "kernelmind_policy",
	.test_cases = policy_test_cases,
};

kunit_test_suite(policy_test_suite);
