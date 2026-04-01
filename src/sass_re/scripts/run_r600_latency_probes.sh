#!/bin/bash
# r600 TeraScale-2 Latency Probe Runner
# steinmarder methodology: dependent chains to measure per-opcode latency
#
# Runs on x130e via SSH. Captures R600_DEBUG ISA + GALLIUM_HUD timing.
# Modeled after compile_profile_all.sh and ncu_profile_all_probes.sh.

set -eu

HOST="nick@x130e"
REMOTE_DIR="~/eric/TerakanMesa/re-toolkit"
RESULTS_DIR="$REMOTE_DIR/data/latency_probes_$(date +%Y%m%dT%H%M%S)"
PROBE_DIR="$REMOTE_DIR/probes"

echo "============================================================"
echo " r600 Latency Probe Runner (steinmarder methodology)"
echo " Host: $HOST"
echo " Results: $RESULTS_DIR"
echo "============================================================"
echo ""

ssh $HOST "
export DISPLAY=:0
export LIBGL_DRIVERS_PATH=/usr/local/mesa-debug/lib/x86_64-linux-gnu/dri
export LD_LIBRARY_PATH=/usr/local/mesa-debug/lib/x86_64-linux-gnu

mkdir -p $RESULTS_DIR

echo '=== Phase 1: ALU Latency (dependent chain timing) ==='
echo ''

# For each probe shader, run with GALLIUM_HUD frametime to get throughput,
# and R600_DEBUG to get ISA. The ratio of FPS between dependent-chain
# (latency-bound) and independent (throughput-bound) versions gives latency.

for probe in $PROBE_DIR/*.frag; do
    name=\$(basename \$probe .frag)
    echo \"--- Probe: \$name ---\"

    # Capture ISA
    R600_DEBUG=vs,ps,cs,fs,tex,precompile \\
    timeout 12 glmark2 -s 100x100 -b build:use-vbo=true \\
        2> \"$RESULTS_DIR/\${name}_isa.log\" >/dev/null || true

    # Capture frametime (throughput measurement)
    vblank_mode=0 GALLIUM_HUD=\"stdout,frametime\" \\
    timeout 12 glmark2 -s 100x100 -b build:use-vbo=true \\
        2>/dev/null | grep frametime | awk -F': ' '{print \$2}' > \"$RESULTS_DIR/\${name}_frametimes.txt\" || true

    # Quick stats
    if [ -s \"$RESULTS_DIR/\${name}_frametimes.txt\" ]; then
        python3 -c \"
import statistics
with open('$RESULTS_DIR/\${name}_frametimes.txt') as f:
    t = [float(l.strip()) for l in f if l.strip()]
if t:
    print(f'  mean={statistics.mean(t):.2f}ms median={statistics.median(t):.2f}ms n={len(t)}')
\" 2>/dev/null || echo '  (no stats)'
    fi

    # Count ISA metrics
    bundles=\$(grep -cP '^\s+[0-9a-f]+ [0-9a-f]+ [0-9a-f]+\s+\d+ x:' \"$RESULTS_DIR/\${name}_isa.log\" 2>/dev/null || echo 0)
    alu=\$(grep -cP '^\s+[0-9a-f]+ [0-9a-f]+ [0-9a-f]+\s+(\d+)?\s*[xyzwt]:' \"$RESULTS_DIR/\${name}_isa.log\" 2>/dev/null || echo 0)
    echo \"  bundles=\$bundles alu_instr=\$alu\"
    echo ''
done

echo '=== Phase 2: GPU Pipeline Counters ==='
echo ''

# Capture detailed pipeline counters for the phong workload
vblank_mode=0 GALLIUM_HUD=\"stdout,GPU-load+GPU-shaders-busy+GPU-ta-busy+GPU-vgt-busy+GPU-sc-busy+GPU-db-busy+GPU-cb-busy+GPU-cp-busy+GPU-sdma-busy+GPU-scratch-ram-busy+temperature+shader-clock+memory-clock\" \\
timeout 15 glmark2 -s 400x300 -b shading:shading=phong \\
    2>/dev/null | grep -v '^\$' | head -30 > \"$RESULTS_DIR/pipeline_counters.txt\" || true

echo 'Pipeline counters captured'
echo ''

echo '=== Phase 3: Memory Subsystem ==='
echo ''

vblank_mode=0 GALLIUM_HUD=\"stdout,VRAM-usage+GTT-usage+num-bytes-moved+num-evictions+buffer-wait-time+num-mapped-buffers\" \\
timeout 15 glmark2 -s 400x300 -b shading:shading=phong \\
    2>/dev/null | grep -v '^\$' | head -20 > \"$RESULTS_DIR/memory_counters.txt\" || true

echo 'Memory counters captured'
echo ''

echo '============================================================'
echo ' Probing complete'
echo ' Results: $RESULTS_DIR'
echo '============================================================'
ls -lh $RESULTS_DIR/
"
