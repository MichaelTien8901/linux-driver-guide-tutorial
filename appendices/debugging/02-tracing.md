---
layout: default
title: "A.2 Tracing"
parent: "Appendix A: Debugging"
nav_order: 2
---

# Tracing

ftrace and kprobes for understanding code flow.

## ftrace: Function Tracing

ftrace is built into the kernel - no special tools needed.

### Basic Function Tracing

```bash
# Check available tracers
cat /sys/kernel/debug/tracing/available_tracers

# Enable function tracer
echo function > /sys/kernel/debug/tracing/current_tracer

# Filter to your module
echo 'my_driver_*' > /sys/kernel/debug/tracing/set_ftrace_filter

# Start tracing
echo 1 > /sys/kernel/debug/tracing/tracing_on

# Trigger your code...

# Stop and view
echo 0 > /sys/kernel/debug/tracing/tracing_on
cat /sys/kernel/debug/tracing/trace
```

### Function Graph (Call Tree)

```bash
echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo 'my_probe' > /sys/kernel/debug/tracing/set_graph_function
echo 1 > /sys/kernel/debug/tracing/tracing_on

# Output shows call hierarchy:
#  my_probe() {
#    devm_kzalloc() { ... }
#    platform_get_resource() { ... }
#    devm_ioremap() { ... }
#  }
```

### trace-cmd (Easier Interface)

```bash
# Record trace
sudo trace-cmd record -p function -l 'my_driver_*' sleep 5

# View
trace-cmd report
```

## Kprobes: Dynamic Instrumentation

Insert probes without recompiling.

### Using /sys/kernel/debug/kprobes

```bash
# Add probe at function entry
echo 'p:my_probe my_driver_read' > /sys/kernel/debug/tracing/kprobe_events

# Enable
echo 1 > /sys/kernel/debug/tracing/events/kprobes/my_probe/enable

# Trigger function...

# View trace
cat /sys/kernel/debug/tracing/trace

# Remove probe
echo '-:my_probe' > /sys/kernel/debug/tracing/kprobe_events
```

### Probe with Arguments

```bash
# Capture function argument (arg1)
echo 'p:read_probe my_driver_read count=%cx' > /sys/kernel/debug/tracing/kprobe_events

# Capture return value
echo 'r:read_ret my_driver_read $retval' > /sys/kernel/debug/tracing/kprobe_events
```

## BPF Tracing (Modern)

For complex tracing, use BPF tools:

```bash
# Trace function latency
sudo funclatency my_driver_read

# Count function calls
sudo funccount 'my_driver_*'

# Trace with bpftrace
sudo bpftrace -e 'kprobe:my_driver_read { printf("read called\n"); }'
```

{: .note }
> BPF tools require `bcc-tools` or `bpftrace` packages.

## Tracepoints

Static instrumentation points in the kernel:

```bash
# List available
cat /sys/kernel/debug/tracing/available_events | grep block

# Enable
echo 1 > /sys/kernel/debug/tracing/events/block/block_rq_issue/enable
```

## Quick Reference

| Task | Command |
|------|---------|
| Trace function calls | `echo function > current_tracer` |
| Show call graph | `echo function_graph > current_tracer` |
| Filter functions | `echo 'pattern' > set_ftrace_filter` |
| Add kprobe | `echo 'p:name func' > kprobe_events` |
| View trace | `cat trace` |
| Clear trace | `echo > trace` |

## Further Reading

- [ftrace](https://docs.kernel.org/trace/ftrace.html) - Official docs
- [Kprobes](https://docs.kernel.org/trace/kprobes.html) - Probe documentation
- [BPF Documentation](https://docs.kernel.org/bpf/index.html)
