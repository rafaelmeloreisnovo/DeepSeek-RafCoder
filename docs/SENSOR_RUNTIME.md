# RAFAELIA Sensor Runtime

This document defines the first deterministic sensor runtime layer for RafCoder / RAFAELOS.

The runtime is intentionally placed in `core/` because it belongs to the native execution layer, not to Android UI code. Android is responsible for collecting sensor events; this runtime is responsible for deterministic post-ingestion processing.

## Boundary contract

```text
Android SensorManager / ASensorManager
    -> JNI or NDK bridge
    -> raf_sensor_push_q16(...)
    -> raf_sensor_process(...)
    -> raf_sensor_debug_frame
```

The runtime does **not** attempt to read physical sensors through raw syscalls. On Android, normal applications receive sensor data through the framework/HAL boundary. The bare-metal-like part begins after each event has been copied into the native ring buffer.

## Design goals

- No malloc in the hot path.
- No garbage collection pressure in the hot path.
- No Android framework headers in `core/`.
- Deterministic ring buffer with power-of-two capacity.
- Q16-style integer coordinates for sensor vectors.
- Per-sample CRC witness.
- Batch CRC and debug frame for runtime inspection.
- Toroidal T^7 projection fields for downstream RAFAELIA experiments.
- Portable C path for host, Termux and Android NDK builds.

## Files

```text
core/sensor_runtime.h  public interface and data contracts
core/sensor_runtime.c  deterministic implementation
```

## Core structures

`raf_sensor_sample` stores one already-normalized event:

```text
seq, timestamp_ns, sensor_type, flags, x_q16, y_q16, z_q16, accuracy_q16, witness_crc32
```

`raf_sensor_runtime` stores the ring, counters, coherence/entropy state, batch CRC, sensor mask, T^7 projection and last debug frame.

`raf_sensor_debug_frame` is the runtime debugger surface:

```text
seq
timestamp_ns
sensor_mask
batch_crc32
coherence_q16
entropy_q16
invariant_milli
dropped_samples
flags
stage
output[8]
```

## Suggested JNI / NDK bridge

The bridge should convert Android floats to integer milli-units outside the runtime, then call:

```c
raf_sensor_milli_to_q16(milli_units, milli_full_scale)
raf_sensor_push_q16(&runtime, sensor_type, timestamp_ns, x_q16, y_q16, z_q16, accuracy_q16)
```

Example scaling policies:

```text
accelerometer: milli_full_scale = 20000   // +/- 20 m/s^2
 gyroscope:    milli_full_scale = 35000   // domain-specific angular scale
 magnetometer: milli_full_scale = 200000  // domain-specific magnetic scale
```

The runtime does not enforce physical interpretation; it only preserves deterministic fixed-point processing after ingestion.

## Processing contract

```c
raf_sensor_process(&runtime, max_samples, &debug_frame)
```

The function consumes up to `max_samples` samples from the ring and updates:

- batch CRC;
- FNV-style hash state;
- coherence EMA;
- entropy proxy;
- invariant score;
- T^7 projection;
- runtime debug frame.

Return value:

```text
>0  number of processed samples
 0  RAF_SENSOR_OK only for init/snapshot helpers
<0  RAF_SENSOR_ERR_*
```

## Runtime stages

```text
0 idle / initialized
1 process entered
2 witness validation
3 batch/hash accumulation
4 T^7 projection
5 coherence/entropy/invariant update
6 debug frame commit
```

## Flags

```text
RAF_SENSOR_FLAG_OVERFLOW       ring overflow; oldest sample was dropped
RAF_SENSOR_FLAG_BAD_TIMESTAMP  timestamp was not monotonic
RAF_SENSOR_FLAG_RANGE_CLAMPED  reserved for bridge/range validation
RAF_SENSOR_FLAG_ROLLBACK       reserved for future watchdog rollback
RAF_SENSOR_FLAG_NO_COMMIT      witness mismatch; sample skipped
```

## Why this belongs in DeepSeek-RafCoder

RafCoder already contains the native RAFAELOS runtime, architecture primitives, Android JNI/NDK bridge and deterministic sector processing. Sensor runtime is another deterministic input stream feeding the same family of native invariants.

This layer should later be connected to:

- `android/` JNI bridge;
- host unit tests;
- ARM32/ARM64 benchmark harness;
- optional NEON batch normalization;
- optional CRC32C dispatch when available.

## Next implementation gates

1. Add host C unit test for ring overflow, timestamp flags and CRC witness.
2. Add Android JNI bridge that receives `SensorEvent` batches and calls `raf_sensor_push_q16`.
3. Add benchmark for ARM32 and ARM64.
4. Add optional architecture dispatch for CRC32C and NEON batch paths.
5. Keep Android collection separate from deterministic native processing.
