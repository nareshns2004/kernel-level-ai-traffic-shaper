/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Minimal vmlinux.h stub for out-of-tree BPF builds.
 * Regenerate with: ./scripts/gen_vmlinux.sh
 */
#ifndef __VMLINUX_H__
#define __VMLINUX_H__

#include <linux/types.h>

typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;
typedef __s64 s64;
typedef __s32 s32;
typedef __s16 s16;
typedef __s8 s8;

enum {
	BPF_ANY		= 0,
	BPF_NOEXIST	= 1,
	BPF_EXIST	= 2,
	BPF_F_LOCK	= 4,
};

enum bpf_map_type {
	BPF_MAP_TYPE_UNSPEC		= 0,
	BPF_MAP_TYPE_HASH		= 1,
	BPF_MAP_TYPE_ARRAY		= 2,
	BPF_MAP_TYPE_PROG_ARRAY		= 3,
	BPF_MAP_TYPE_PERF_EVENT_ARRAY	= 4,
	BPF_MAP_TYPE_LRU_HASH		= 9,
};

enum {
	BPF_F_CURRENT_CPU	= 0xffffffffULL,
};

struct bpf_map_def {
	unsigned int type;
	unsigned int key_size;
	unsigned int value_size;
	unsigned int max_entries;
	unsigned int map_flags;
};

#endif /* __VMLINUX_H__ */
