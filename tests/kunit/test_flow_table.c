// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include "../../kernel/flow_table.h"
#include "../../ebpf/common.h"

static void test_flow_key_size(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, sizeof(struct kernelmind_flow_key), 16);
}

static void test_flow_table_constants(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, KERNELMIND_FLOW_TABLE_SIZE, 65536);
	KUNIT_EXPECT_EQ(test, KERNELMIND_NUM_CLASSES, 12);
}

static struct kunit_case flow_table_test_cases[] = {
	KUNIT_CASE(test_flow_key_size),
	KUNIT_CASE(test_flow_table_constants),
	{}
};

static struct kunit_suite flow_table_test_suite = {
	.name = "kernelmind_flow_table",
	.test_cases = flow_table_test_cases,
};

kunit_test_suite(flow_table_test_suite);
