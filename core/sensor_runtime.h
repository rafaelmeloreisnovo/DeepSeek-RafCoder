#ifndef CORE_SENSOR_RUNTIME_H
#define CORE_SENSOR_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RAF_SENSOR_TORUS_DIM 7u
#define RAF_SENSOR_DEFAULT_RING_CAP 256u
#define RAF_SENSOR_DEBUG_WORDS 8u
#define RAF_SENSOR_Q16_ONE 65535u

#define RAF_SENSOR_OK 0
#define RAF_SENSOR_ERR_NULL -1
#define RAF_SENSOR_ERR_CAPACITY -2
#define RAF_SENSOR_ERR_EMPTY -3
#define RAF_SENSOR_ERR_RANGE -4

#define RAF_SENSOR_FLAG_OVERFLOW       (1u << 0)
#define RAF_SENSOR_FLAG_BAD_TIMESTAMP  (1u << 1)
#define RAF_SENSOR_FLAG_RANGE_CLAMPED  (1u << 2)
#define RAF_SENSOR_FLAG_ROLLBACK       (1u << 3)
#define RAF_SENSOR_FLAG_NO_COMMIT      (1u << 4)

/* Android sensor types are passed as numeric ids by the JNI/NDK bridge.
 * The runtime intentionally avoids Android headers so the hot path remains
 * portable C and can be unit-tested on host, Termux and Android NDK builds. */
typedef struct raf_sensor_sample {
    uint64_t seq;
    uint64_t timestamp_ns;
    uint32_t sensor_type;
    uint32_t flags;
    int32_t x_q16;
    int32_t y_q16;
    int32_t z_q16;
    uint32_t accuracy_q16;
    uint32_t witness_crc32;
} raf_sensor_sample;

typedef struct raf_sensor_debug_frame {
    uint64_t seq;
    uint64_t timestamp_ns;
    uint32_t sensor_mask;
    uint32_t batch_crc32;
    uint32_t coherence_q16;
    uint32_t entropy_q16;
    uint32_t invariant_milli;
    uint32_t dropped_samples;
    uint32_t flags;
    uint32_t stage;
    uint32_t output[RAF_SENSOR_DEBUG_WORDS];
} raf_sensor_debug_frame;

typedef struct raf_sensor_ring {
    raf_sensor_sample* samples;
    uint32_t capacity;      /* Must be power-of-two for mask addressing. */
    uint32_t mask;
    uint32_t write_idx;
    uint32_t read_idx;
    uint32_t dropped;
    uint32_t flags;
} raf_sensor_ring;

typedef struct raf_sensor_runtime {
    raf_sensor_ring ring;
    uint64_t seq;
    uint64_t last_timestamp_ns;
    uint64_t hash64;
    uint32_t coherence_q16;
    uint32_t entropy_q16;
    uint32_t batch_crc32;
    uint32_t sensor_mask;
    uint32_t invariant_milli;
    uint32_t flags;
    uint32_t stage;
    uint32_t torus_q16[RAF_SENSOR_TORUS_DIM];
    raf_sensor_debug_frame last_debug;
} raf_sensor_runtime;

/* Convert signed milli-units to Q16.16-like signed fixed point.
 * Example bridge usage: accel_mps2 * 1000.0f -> milli_units -> q16.
 * This keeps float conversion outside the runtime hot path. */
int32_t raf_sensor_milli_to_q16(int32_t milli_units, int32_t milli_full_scale);

int raf_sensor_runtime_init(raf_sensor_runtime* rt,
                            raf_sensor_sample* backing_store,
                            uint32_t capacity_power_of_two);

int raf_sensor_push_q16(raf_sensor_runtime* rt,
                        uint32_t sensor_type,
                        uint64_t timestamp_ns,
                        int32_t x_q16,
                        int32_t y_q16,
                        int32_t z_q16,
                        uint32_t accuracy_q16);

/* Process up to max_samples from the ring and update T^7/debug state.
 * out_frame may be NULL when the caller only wants to advance state. */
int raf_sensor_process(raf_sensor_runtime* rt,
                       uint32_t max_samples,
                       raf_sensor_debug_frame* out_frame);

/* Snapshot-only helper: copies the last debug frame without consuming samples. */
int raf_sensor_debug_snapshot(const raf_sensor_runtime* rt,
                              raf_sensor_debug_frame* out_frame);

#ifdef __cplusplus
}
#endif

#endif
