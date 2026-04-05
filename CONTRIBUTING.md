# Contributing to KernelMind Traffic Shaper

Thank you for your interest in contributing. KernelMind spans kernel C, eBPF, Python, and systems tooling — contributions of all kinds are welcome, from bug fixes and documentation to new AI models and kernel features.

Please read this guide before submitting your first PR.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Project Structure at a Glance](#project-structure-at-a-glance)
- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Coding Standards](#coding-standards)
- [Commit Convention](#commit-convention)
- [Pull Request Process](#pull-request-process)
- [Testing Requirements](#testing-requirements)
- [Working on the Kernel Module](#working-on-the-kernel-module)
- [Working on eBPF Programs](#working-on-ebpf-programs)
- [Working on the AI / Model Layer](#working-on-the-ai--model-layer)
- [Working on Userspace Tools](#working-on-userspace-tools)
- [Reporting Bugs](#reporting-bugs)
- [Proposing New Features](#proposing-new-features)
- [Security Vulnerabilities](#security-vulnerabilities)
- [Release Process](#release-process)

---

## Code of Conduct

This project follows the [Contributor Covenant v2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/). By participating, you agree to uphold it. Violations can be reported to the maintainers at `conduct@kernelmind.dev`.

---

## Project Structure at a Glance

```
kernel/       Kernel module (C, GPL-2.0)
ebpf/         eBPF / BPF programs (C, GPL-2.0)
ai/           Model compiler & trainer (Python, MIT)
userspace/    CLI, daemon, SDK (C + Python, MIT)
tests/        KUnit, pytest, benchmarks
config/       TOML config templates
scripts/      Setup & packaging helpers
```

Each directory has its own concerns, style rules, and test requirements — see the relevant section below.

---

## Getting Started

### 1. Fork and clone

```bash
git clone https://github.com/your-fork/kernelmind-shaper.git
cd kernelmind-shaper
git remote add upstream https://github.com/your-org/kernelmind-shaper.git
```

### 2. Set up the development environment

See [Development Environment](#development-environment) below.

### 3. Find something to work on

- Check issues tagged [`good first issue`](https://github.com/your-org/kernelmind-shaper/issues?q=label%3A%22good+first+issue%22)
- Check the [Roadmap in README.md](README.md#roadmap) for planned features
- Open a discussion before starting large or architectural changes

### 4. Make your changes, test, commit, and open a PR

See [Pull Request Process](#pull-request-process).

---

## Development Environment

### Recommended setup

A dedicated Linux VM or bare-metal machine is **strongly recommended** for kernel development. Never develop or test the kernel module on a production machine.

```
OS:       Ubuntu 22.04 LTS or Fedora 38+ (in a VM)
Kernel:   Linux 5.15 LTS or 6.6 LTS
RAM:      4 GB minimum, 8 GB recommended
vCPUs:    2 minimum
```

### Install dependencies

```bash
# Ubuntu / Debian
sudo apt install \
    build-essential linux-headers-$(uname -r) \
    clang-15 llvm-15 libelf-dev libbpf-dev bpftool \
    python3.11 python3.11-dev python3-pip python3-venv \
    cmake pkg-config iproute2 iperf3

# Fedora
sudo dnf install \
    kernel-devel clang llvm elfutils-libelf-devel \
    libbpf-devel bpftool python3-devel cmake iproute iperf3
```

### Python environment

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -e "userspace/sdk[dev]"
pip install -r ai/trainer/requirements.txt
```

### Build everything

```bash
make all          # kernel module + eBPF + userspace CLI
make install      # install to /lib/modules and /usr/local/bin
make test         # run all tests (requires root for kunit)
```

### Generate `vmlinux.h` (if your kernel version changed)

```bash
./scripts/gen_vmlinux.sh
```

---

## Coding Standards

### Kernel module C (`kernel/`, `ebpf/`)

KernelMind follows the [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html) strictly.

Key rules:

- **Tabs, not spaces.** Tab width = 8.
- **Line length: 80 columns.** Hard limit for kernel C. eBPF C allows up to 100.
- **Function names: `snake_case`.** No camelCase anywhere in kernel or eBPF code.
- **No C99 variable-length arrays (VLAs).** Banned in kernel code.
- **No dynamic memory allocation in hot paths.** Use pre-allocated per-CPU or per-flow structures.
- **Every kernel function that can fail must check its return value.** Use `IS_ERR()`, `PTR_ERR()`, etc.
- **All kernel pointers must be validated before dereference.** Never trust a pointer from userspace.
- **Use `BUILD_BUG_ON` for compile-time assertions**, not runtime checks where the invariant is fixed.

Enforce locally with:

```bash
./scripts/checkpatch.pl --strict -f kernel/*.c kernel/*.h
clang-format --style=file -i kernel/*.c kernel/*.h
```

The `.clang-format` at the repo root is configured for the Linux kernel style. Run it before every commit on any C file you touched.

### eBPF C (`ebpf/`)

Everything in the kernel coding style section applies, plus:

- **BPF verifier constraints are non-negotiable.** Every loop must have a bounded iteration count. Every map access must check for NULL return. Every helper call return value must be checked.
- **No function calls across BPF programs.** Use `static __always_inline` for shared helpers.
- **Annotate every map access with `__u32 key = ...` before the lookup**, not inline in the macro.
- **Do not use global variables for state.** All state lives in BPF maps.
- **BPF program sections must be explicitly named** with `SEC("...")` matching the expected attach point.

### Python (`ai/`, `userspace/sdk/`)

- **Formatter: `black` (line length 88).**
- **Linter: `ruff` with the `E`, `F`, `I` rule sets enabled.**
- **Type hints required on all public function signatures.**
- **Docstrings required on all public classes and functions** (Google style).
- **No `print()` in library code.** Use `logging`.

Run before committing:

```bash
black ai/ userspace/sdk/
ruff check ai/ userspace/sdk/
mypy ai/ userspace/sdk/ --ignore-missing-imports
```

### Shell scripts (`scripts/`)

- **`bash` only.** No `sh`-isms.
- **`set -euo pipefail` at the top of every script.**
- **Lint with `shellcheck`.**

---

## Commit Convention

We use [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/).

```
<type>(<scope>): <short summary>

[optional body]

[optional footer]
```

**Types:**

| Type       | When to use |
|------------|-------------|
| `feat`     | New feature |
| `fix`      | Bug fix |
| `perf`     | Performance improvement |
| `refactor` | Code restructure with no behaviour change |
| `test`     | Adding or fixing tests |
| `docs`     | Documentation only |
| `build`    | Build system changes (Makefile, CMake, pyproject) |
| `ci`       | CI/CD changes |
| `chore`    | Maintenance (version bumps, file moves) |

**Scopes:** `kernel`, `ebpf`, `ai`, `cli`, `daemon`, `sdk`, `config`, `scripts`, `docs`

**Examples:**

```
feat(kernel): add HFSC qdisc support alongside HTB
fix(ebpf): bounds-check flow table index before map lookup
perf(ai): reduce GBDT inference path from 220ns to 180ns
docs(sdk): add retraining walkthrough to trainer docstring
test(kunit): add edge cases for flow_table eviction under pressure
```

**Rules:**

- Summary line ≤ 72 characters, imperative mood ("add", not "added" or "adds")
- Body wraps at 72 characters
- Reference issues with `Fixes #123` or `Closes #456` in the footer
- Breaking changes must include `BREAKING CHANGE:` in the footer

---

## Pull Request Process

1. **Rebase onto `main` before opening a PR.** We do not merge PRs with merge commits.

   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **One logical change per PR.** Split unrelated fixes into separate PRs.

3. **All tests must pass.** See [Testing Requirements](#testing-requirements).

4. **All linters must pass with zero warnings.** The CI will block merging otherwise.

5. **Fill in the PR template** — description, motivation, how to test, and affected components.

6. **At least one maintainer approval is required** before merging.

7. **Kernel module changes require two approvals** from maintainers who have reviewed the kernel code path.

8. PRs that touch `ebpf/inference_prog.bpf.c` or `ai/compiler/` require the author to include benchmark output showing inference latency has not regressed.

### PR title format

Same as commit convention — the PR title becomes the squash commit message.

---

## Testing Requirements

Every PR must pass the full test suite. The table below describes what is required per component:

| Component changed       | Tests required |
|-------------------------|---------------|
| `kernel/*.c`            | KUnit tests + integration tests |
| `ebpf/*.bpf.c`          | KUnit tests + BPF verifier load test |
| `ai/compiler/`          | Constraint validator + inference accuracy test |
| `ai/trainer/`           | Unit tests in `tests/integration/test_classification.py` |
| `userspace/cli/`        | Integration tests in `tests/integration/` |
| `userspace/daemon/`     | Integration tests + manual smoke test |
| `userspace/sdk/`        | pytest unit tests |
| `config/`               | Config parse test (`make test-config`) |

### Running the test suite

```bash
# KUnit (requires root, runs inside the kernel)
sudo make kunit

# Integration tests (requires a configured test VM)
pytest tests/integration/ -v

# Benchmark (run before and after your change if touching hot paths)
sudo tests/bench/bench_throughput.sh
sudo ./tests/bench/bench_inference  # outputs p50/p99 in nanoseconds
```

### BPF verifier load test

All eBPF programs must load cleanly through the verifier:

```bash
make ebpf
sudo bpftool prog load ebpf/inference_prog.bpf.o /sys/fs/bpf/test_load && \
  echo "Verifier passed" && \
  sudo rm /sys/fs/bpf/test_load
```

### Performance regression rule

If your change touches any file in `kernel/`, `ebpf/`, or `ai/compiler/`:

- Run `bench_inference` before and after your change.
- If p99 latency increases by more than **5%**, the PR will not be merged without a documented justification.
- Include the before/after numbers in your PR description.

---

## Working on the Kernel Module

### Key rules

- **Never call `schedule()` or `sleep()` from netfilter hooks.** Hooks run in softirq context — any blocking call will deadlock.
- **Use `rcu_read_lock()` / `rcu_read_unlock()` around flow table lookups.** The flow table uses RCU for lockless reads on the hot path.
- **Increment `module_refcount` before spawning kernel threads.** Ensure cleanup in `module_exit()` joins all threads.
- **Memory allocated with `kmalloc()` must be freed in every error path.** Use `goto err_cleanup` patterns consistently.
- **`copy_from_user()` / `copy_to_user()` must be the only way data crosses the kernel/user boundary.** Never dereference a raw userspace pointer.

### Testing kernel changes without rebooting

Use `rmmod kernelmind && insmod kernel/kernelmind.ko` during development. If the module panics, the VM will crash — use a snapshot. Never test on bare metal without a serial console and KDB configured.

### Adding a new hook

1. Register in `kernel/kernelmind.c` using `nf_register_net_hook()`.
2. Implement handler in `kernel/hook.c` — return `NF_ACCEPT` unless you intend to modify or drop.
3. Add a kunit test in `tests/kunit/` covering the normal path and at least two error paths.
4. Document the new hook in the architecture section of `README.md`.

---

## Working on eBPF Programs

### BPF constraints checklist (required before submitting)

Before opening a PR that touches any `.bpf.c` file, verify:

- [ ] All loops have a bounded max iteration count visible to the verifier
- [ ] Every `bpf_map_lookup_elem()` return value is checked for NULL before dereference
- [ ] Stack usage does not exceed 512 bytes per BPF program frame
- [ ] No `bpf_tail_call()` chains longer than 33 (verifier limit)
- [ ] The program loads cleanly through the verifier (see BPF verifier load test above)
- [ ] `common.h` structs are `__attribute__((packed))` where shared with kernel C

### Adding a new BPF map

1. Define the map in `ebpf/flow_maps.bpf.c` using the `SEC("maps")` macro.
2. Forward-declare it in `ebpf/common.h` so kernel-side C can reference it.
3. Size it conservatively — explain your sizing rationale in a comment.
4. Document its purpose, key type, and value type in a block comment above the definition.

### Updating model weights via BPF map

Model weight hot-swap (no module reload) works by writing new leaf values into the `model_weights` BPF LRU hash map from userspace. The inference program reads weights via `bpf_map_lookup_elem()` on every classification. If you change the weight format:

1. Bump `MODEL_VERSION` in `ebpf/common.h`.
2. Update `ai/compiler/gbdt_to_bpf.py` to emit the new format.
3. Update `kernel/online_learn.c` to write the new format.
4. Add a migration note in `CHANGELOG.md`.

---

## Working on the AI / Model Layer

### BPF compilation constraints

The model compiler (`ai/compiler/gbdt_to_bpf.py`) enforces these hard limits — do not relax them:

| Constraint | Limit | Reason |
|------------|-------|--------|
| Tree depth | ≤ 8 | BPF stack frame limit |
| Estimators | ≤ 64 | BPF instruction count budget |
| Features | ≤ 32 | Stack allocation in inference_prog |
| Weight format | Q8.8 fixed-point | No floating point in BPF |

If a new model architecture requires relaxing any of these, open a design discussion issue first — it will require changes to the eBPF inference program and re-verification.

### Adding a new flow class

1. Add the class label to `ai/models/model_schema.json`.
2. Add labeled training examples to your dataset.
3. Retrain and re-verify constraints.
4. Add the class to the policy config documentation in `config/kernelmind.conf.example`.
5. Add a test case to `tests/integration/test_classification.py` with at least 50 labeled flows.

### Submitting a new pre-trained model

To replace or improve `ai/models/default.kmmodel`:

1. Run `ai/compiler/verify_constraints.py` — must pass with zero violations.
2. Run `tests/integration/test_classification.py` — per-class F1 must be ≥ 0.85.
3. Run `tests/bench/bench_inference` — p99 must not regress beyond 5%.
4. Include a brief model card in your PR description: training dataset description, class distribution, and per-class accuracy table.

---

## Working on Userspace Tools

### CLI (`userspace/cli/`)

- Commands are implemented as independent `.c` files (`cmd_flows.c`, `cmd_model.c`, etc.).
- Each command file exposes a single entry-point function: `int cmd_<name>(int argc, char **argv)`.
- All netlink communication goes through the shared helpers in `kernel/netlink.h` — do not open raw sockets in command files.
- Help text must be accurate and complete. Run `kernelmind help <cmd>` as part of your manual test.

### Daemon (`userspace/daemon/`)

- The daemon must handle `SIGTERM` and `SIGHUP` gracefully (flush metrics, reload config).
- All REST endpoints must be documented in a comment block above the handler function.
- Prometheus metric names must follow the `kernelmind_<subsystem>_<name>_<unit>` convention.

### Python SDK (`userspace/sdk/`)

- The SDK is the public API for external tooling. **Do not break backward compatibility without a major version bump.**
- Every public class and function must have a docstring with a usage example.
- Type hints are required — `mypy` must pass with no errors.

---

## Reporting Bugs

Open a GitHub Issue using the **Bug Report** template. Include:

1. **Kernel version** (`uname -r`)
2. **KernelMind version** (`modinfo kernelmind | grep version`)
3. **Steps to reproduce** — minimal and specific
4. **Expected vs actual behavior**
5. **`dmesg` output** from around the time of the bug
6. **`/proc/kernelmind/stats`** output if the module loaded
7. **Network topology** (interface names, qdisc setup) if relevant

For kernel panics, include the full oops message and (if available) a stack trace from KDB or `crash`.

---

## Proposing New Features

For small features (a new CLI flag, a new config option), open a PR directly with the implementation and tests.

For larger features (new qdisc support, new ML architecture, new BPF program), open a **Design Discussion** issue first using the template. Include:

- Problem statement
- Proposed solution
- Alternatives considered
- Impact on BPF verifier constraints, memory, and latency
- Test plan

Wait for at least one maintainer to comment before starting implementation. This saves you from writing code that won't be accepted.

---

## Security Vulnerabilities

**Do not open a public GitHub issue for security vulnerabilities.**

Report them privately to `security@kernelmind.dev`. Include:

- Description of the vulnerability
- Steps to reproduce
- Potential impact (privilege escalation, DoS, data exfiltration, etc.)
- Suggested fix if you have one

We aim to acknowledge within 48 hours and provide a fix timeline within 7 days. Credit will be given in the security advisory unless you prefer to remain anonymous.

---

## Release Process

Releases are managed by maintainers. The process:

1. `CHANGELOG.md` is updated with all changes since the last release.
2. `MODULE_VERSION` in `kernel/kernelmind.c` is bumped.
3. `MODEL_VERSION` in `ebpf/common.h` is bumped if the model format changed.
4. A signed tag is pushed: `git tag -s v0.x.y -m "Release v0.x.y"`.
5. GitHub Actions builds the release artifacts (`.ko`, CLI binary, Python wheel).
6. The release is published with the signed artifacts and the CHANGELOG excerpt.

Contributors do not need to manage releases — just ensure your PR has a clear description that can be included in the CHANGELOG.

---

*Questions? Open a [Discussion](https://github.com/your-org/kernelmind-shaper/discussions) or ping `@maintainers` in an issue.*
