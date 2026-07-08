// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include "../../ebpf/common.h"

static void test_inference_threshold(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, KERNELMIND_INFERENCE_THRESHOLD, 8);
}

static void test_max_features(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, KERNELMIND_MAX_FEATURES, 32);
	KUNIT_EXPECT_EQ(test, KERNELMIND_MAX_TREES, 64);
}

static struct kunit_case inference_test_cases[] = {
	KUNIT_CASE(test_inference_threshold),
	KUNIT_CASE(test_max_features),
	{}
};

static struct kunit_suite inference_test_suite = {
	.name = "kernelmind_inference",
	.test_cases = inference_test_cases,
};

kunit_test_suite(inference_test_suite);
