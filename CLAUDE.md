# Traffic shaper (eBPF): guidance for Claude Code

## Status
Prototype being rewritten. The kernel module under kernel/ is deprecated and
must not be extended. New datapath work is eBPF at tc (clsact) only, loaded
with libbpf skeletons. Generic Linux traffic shaping only: no code, names,
designs or numbers from any employer project.

Current state is described honestly in README.md. The roadmap (P0–P6) is in
the private master booklet, section 15; P0 (cleanup) is done. Next is P2:
generated vmlinux.h and a CI job that compiles every BPF object.

## Commands
Planned targets (introduced in P2/P5; today's Makefile still has `all`,
`ebpf`, `userspace`, `test`, `bench`):
- make bpf      # clang -target bpf with bpftool-generated vmlinux.h
- make test     # unit tests + netns integration tests (needs root in a VM)
- make bench    # pktgen/iperf3 scripts; writes bench/RESULTS.md

Nothing is installed on the user's own machine; heavy or root-requiring
work runs in a cloud VM.

## Rules
- Never write benchmark numbers by hand; RESULTS.md is generated and records
  kernel version, CPU and NIC.
- Fail open: if the model or a map lookup is missing, pass the packet untouched.
- Never overwrite skb->mark outside the reserved masked range; never drop by
  default.
- Every BPF change must load under the verifier in CI (bpftool prog load or
  BPF_PROG_TEST_RUN) and have a netns test.
- Only run tc, ip, bpftool or module commands inside a VM or network namespace;
  ask before touching any real interface.
- Never commit study material (high_level_design/, low_level_design/), build
  output, or the private booklet PDF.
