#include "sensor_runtime.h"

#define RAF_SENSOR_FNV_OFFSET 0xCBF29CE484222325ULL
#define RAF_SENSOR_FNV_PRIME  0x100000001B3ULL
#define RAF_SENSOR_CRC_POLY   0xEDB88320u
#define RAF_SENSOR_EMA_OLD    3u
#define RAF_SENSOR_EMA_NEW    1u
#define RAF_SENSOR_EMA_DEN    4u

static uint32_t raf_sensor_is_power_of_two(uint32_t v) {
    return (v != 0u) && ((v & (v - 1u)) == 0u);
}

static uint32_t raf_sensor_abs_i32_q16(int32_t v) {
    int32_t mask = v >> 31;
    return (uint32_t)((v ^ mask) - mask);
}

static uint32_t raf_sensor_min_u32(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

static uint32_t raf_sensor_clamp_u32(uint32_t v, uint32_t hi) {
    return raf_sensor_min_u32(v, hi);
}

static uint64_t raf_sensor_hash_step_u32(uint64_t h, uint32_t v) {
    uint32_t i;
    for (i = 0u; i < 4u; ++i) {
        uint8_t b = (uint8_t)((v >> (i * 8u)) & 0xFFu);
        h ^= (uint64_t)b;
        h *= RAF_SENSOR_FNV_PRIME;
    }
    return h;
}

static uint64_t raf_sensor_hash_step_u64(uint64_t h, uint64_t v) {
    uint32_t i;
    for (i = 0u; i < 8u; ++i) {
        uint8_t b = (uint8_t)((v >> (i * 8u)) & 0xFFu);
        h ^= (uint64_t)b;
        h *= RAF_SENSOR_FNV_PRIME;
    }
    return h;
}

static uint32_t raf_sensor_crc32_step(uint32_t crc, uint32_t v) {
    uint32_t i;
    uint32_t byte_idx;
    for (byte_idx = 0u; byte_idx < 4u; ++byte_idx) {
        crc ^= (v >> (byte_idx * 8u)) & 0xFFu;
        for (i = 0u; i < 8u; ++i) {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1u) ^ (RAF_SENSOR_CRC_POLY & mask);
        }
    }
    return crc;
}

static uint32_t raf_sensor_sample_crc32(const raf_sensor_sample* s) {
    uint32_t crc = 0xFFFFFFFFu;
    crc = raf_sensor_crc32_step(crc, (uint32_t)s->seq);
    crc = raf_sensor_crc32_step(crc, (uint32_t)(s->seq >> 32u));
    crc = raf_sensor_crc32_step(crc, (uint32_t)s->timestamp_ns);
    crc = raf_sensor_crc32_step(crc, (uint32_t)(s->timestamp_ns >> 32u));
    crc = raf_sensor_crc32_step(crc, s->sensor_type);
    crc = raf_sensor_crc32_step(crc, (uint32_t)s->x_q16);
    crc = raf_sensor_crc32_step(crc, (uint32_t)s->y_q16);
    crc = raf_sensor_crc32_step(crc, (uint32_t)s->z_q16);
    crc = raf_sensor_crc32_step(crc, s->accuracy_q16);
    return ~crc;
}

static uint32_t raf_sensor_metric_from_sample(const raf_sensor_sample* s) {
    uint32_t ax = raf_sensor_abs_i32_q16(s->x_q16);
    uint32_t ay = raf_sensor_abs_i32_q16(s->y_q16);
    uint32_t az = raf_sensor_abs_i32_q16(s->z_q16);
    uint32_t avg = (ax / 3u) + (ay / 3u) + (az / 3u);
    return raf_sensor_clamp_u32(avg, RAF_SENSOR_Q16_ONE);
}

static uint32_t raf_sensor_entropy_proxy(uint32_t crc, uint32_t metric_q16, uint32_t jitter_q16) {
    uint32_t folded = (crc ^ (crc >> 16u)) & 0xFFFFu;
    uint32_t mix = (folded + (metric_q16 & 0xFFFFu) + (jitter_q16 & 0xFFFFu)) / 3u;
    return raf_sensor_clamp_u32(mix, RAF_SENSOR_Q16_ONE);
}

static uint32_t raf_sensor_invariant_milli(uint32_t coherence_q16, uint32_t entropy_q16) {
    uint32_t c_milli = (coherence_q16 * 1000u) / RAF_SENSOR_Q16_ONE;
    uint32_t h_milli = (entropy_q16 * 1000u) / RAF_SENSOR_Q16_ONE;
    uint32_t one_minus_h = (h_milli > 1000u) ? 0u : (1000u - h_milli);
    return (c_milli * one_minus_h) / 1000u;
}

int32_t raf_sensor_milli_to_q16(int32_t milli_units, int32_t milli_full_scale) {
    int64_t scaled;
    if (milli_full_scale == 0) {
        return 0;
    }
    scaled = ((int64_t)milli_units * (int64_t)RAF_SENSOR_Q16_ONE) / (int64_t)milli_full_scale;
    if (scaled > 2147483647LL) {
        return 2147483647;
    }
    if (scaled < -2147483647LL) {
        return -2147483647;
    }
    return (int32_t)scaled;
}

int raf_sensor_runtime_init(raf_sensor_runtime* rt,
                            raf_sensor_sample* backing_store,
                            uint32_t capacity_power_of_two) {
    uint32_t i;
    if (rt == (void*)0 || backing_store == (void*)0) {
        return RAF_SENSOR_ERR_NULL;
    }
    if (!raf_sensor_is_power_of_two(capacity_power_of_two)) {
        return RAF_SENSOR_ERR_CAPACITY;
    }

    rt->ring.samples = backing_store;
    rt->ring.capacity = capacity_power_of_two;
    rt->ring.mask = capacity_power_of_two - 1u;
    rt->ring.write_idx = 0u;
    rt->ring.read_idx = 0u;
    rt->ring.dropped = 0u;
    rt->ring.flags = 0u;

    rt->seq = 0ULL;
    rt->last_timestamp_ns = 0ULL;
    rt->hash64 = RAF_SENSOR_FNV_OFFSET;
    rt->coherence_q16 = RAF_SENSOR_Q16_ONE / 2u;
    rt->entropy_q16 = RAF_SENSOR_Q16_ONE / 2u;
    rt->batch_crc32 = 0u;
    rt->sensor_mask = 0u;
    rt->invariant_milli = 0u;
    rt->flags = 0u;
    rt->stage = 0u;

    for (i = 0u; i < RAF_SENSOR_TORUS_DIM; ++i) {
        rt->torus_q16[i] = 0u;
    }

    for (i = 0u; i < capacity_power_of_two; ++i) {
        backing_store[i].seq = 0ULL;
        backing_store[i].timestamp_ns = 0ULL;
        backing_store[i].sensor_type = 0u;
        backing_store[i].flags = 0u;
        backing_store[i].x_q16 = 0;
        backing_store[i].y_q16 = 0;
        backing_store[i].z_q16 = 0;
        backing_store[i].accuracy_q16 = 0u;
        backing_store[i].witness_crc32 = 0u;
    }

    return RAF_SENSOR_OK;
}

int raf_sensor_push_q16(raf_sensor_runtime* rt,
                        uint32_t sensor_type,
                        uint64_t timestamp_ns,
                        int32_t x_q16,
                        int32_t y_q16,
                        int32_t z_q16,
                        uint32_t accuracy_q16) {
    uint32_t next;
    raf_sensor_sample* s;

    if (rt == (void*)0 || rt->ring.samples == (void*)0) {
        return RAF_SENSOR_ERR_NULL;
    }

    next = (rt->ring.write_idx + 1u) & rt->ring.mask;
    if (next == rt->ring.read_idx) {
        rt->ring.read_idx = (rt->ring.read_idx + 1u) & rt->ring.mask;
        rt->ring.dropped += 1u;
        rt->ring.flags |= RAF_SENSOR_FLAG_OVERFLOW;
        rt->flags |= RAF_SENSOR_FLAG_OVERFLOW;
    }

    s = &rt->ring.samples[rt->ring.write_idx];
    rt->seq += 1ULL;
    s->seq = rt->seq;
    s->timestamp_ns = timestamp_ns;
    s->sensor_type = sensor_type;
    s->flags = 0u;
    s->x_q16 = x_q16;
    s->y_q16 = y_q16;
    s->z_q16 = z_q16;
    s->accuracy_q16 = raf_sensor_clamp_u32(accuracy_q16, RAF_SENSOR_Q16_ONE);

    if (rt->last_timestamp_ns != 0ULL && timestamp_ns <= rt->last_timestamp_ns) {
        s->flags |= RAF_SENSOR_FLAG_BAD_TIMESTAMP;
        rt->flags |= RAF_SENSOR_FLAG_BAD_TIMESTAMP;
    }
    rt->last_timestamp_ns = timestamp_ns;

    s->witness_crc32 = raf_sensor_sample_crc32(s);
    rt->ring.write_idx = next;
    return RAF_SENSOR_OK;
}

int raf_sensor_process(raf_sensor_runtime* rt,
                       uint32_t max_samples,
                       raf_sensor_debug_frame* out_frame) {
    uint32_t processed = 0u;
    uint32_t batch_crc = 0xFFFFFFFFu;
    uint32_t metric_acc = 0u;
    uint32_t jitter_q16 = 0u;
    uint64_t last_ts = 0ULL;

    if (rt == (void*)0 || rt->ring.samples == (void*)0) {
        return RAF_SENSOR_ERR_NULL;
    }
    if (max_samples == 0u) {
        return RAF_SENSOR_ERR_RANGE;
    }

    rt->stage = 1u;

    while (rt->ring.read_idx != rt->ring.write_idx && processed < max_samples) {
        raf_sensor_sample* s = &rt->ring.samples[rt->ring.read_idx];
        uint32_t witness = raf_sensor_sample_crc32(s);
        uint32_t metric = raf_sensor_metric_from_sample(s);
        uint32_t sensor_bit = s->sensor_type & 31u;

        rt->stage = 2u;
        if (witness != s->witness_crc32) {
            rt->flags |= RAF_SENSOR_FLAG_NO_COMMIT;
            rt->ring.read_idx = (rt->ring.read_idx + 1u) & rt->ring.mask;
            processed += 1u;
            continue;
        }

        if (last_ts != 0ULL && s->timestamp_ns > last_ts) {
            uint64_t dt = s->timestamp_ns - last_ts;
            jitter_q16 = (dt > 65535000ULL) ? RAF_SENSOR_Q16_ONE : (uint32_t)(dt / 1000ULL);
            jitter_q16 = raf_sensor_clamp_u32(jitter_q16, RAF_SENSOR_Q16_ONE);
        }
        last_ts = s->timestamp_ns;

        rt->stage = 3u;
        batch_crc = raf_sensor_crc32_step(batch_crc, s->witness_crc32);
        metric_acc = raf_sensor_clamp_u32(metric_acc + (metric / 4u), RAF_SENSOR_Q16_ONE);
        rt->hash64 = raf_sensor_hash_step_u64(rt->hash64, s->seq);
        rt->hash64 = raf_sensor_hash_step_u64(rt->hash64, s->timestamp_ns);
        rt->hash64 = raf_sensor_hash_step_u32(rt->hash64, s->sensor_type);
        rt->hash64 = raf_sensor_hash_step_u32(rt->hash64, s->witness_crc32);
        rt->sensor_mask |= (1u << sensor_bit);

        rt->stage = 4u;
        rt->torus_q16[0] = metric;
        rt->torus_q16[1] = raf_sensor_abs_i32_q16(s->x_q16);
        rt->torus_q16[2] = raf_sensor_abs_i32_q16(s->y_q16);
        rt->torus_q16[3] = raf_sensor_abs_i32_q16(s->z_q16);
        rt->torus_q16[4] = jitter_q16;
        rt->torus_q16[5] = s->accuracy_q16;
        rt->torus_q16[6] = (s->witness_crc32 ^ (s->witness_crc32 >> 16u)) & 0xFFFFu;

        rt->ring.read_idx = (rt->ring.read_idx + 1u) & rt->ring.mask;
        processed += 1u;
    }

    if (processed == 0u) {
        return RAF_SENSOR_ERR_EMPTY;
    }

    rt->stage = 5u;
    rt->batch_crc32 = ~batch_crc;
    {
        uint32_t c_in = metric_acc;
        uint32_t h_in = raf_sensor_entropy_proxy(rt->batch_crc32, metric_acc, jitter_q16);
        rt->coherence_q16 = ((RAF_SENSOR_EMA_OLD * rt->coherence_q16) + (RAF_SENSOR_EMA_NEW * c_in)) / RAF_SENSOR_EMA_DEN;
        rt->entropy_q16 = ((RAF_SENSOR_EMA_OLD * rt->entropy_q16) + (RAF_SENSOR_EMA_NEW * h_in)) / RAF_SENSOR_EMA_DEN;
        rt->invariant_milli = raf_sensor_invariant_milli(rt->coherence_q16, rt->entropy_q16);
    }

    rt->stage = 6u;
    rt->last_debug.seq = rt->seq;
    rt->last_debug.timestamp_ns = rt->last_timestamp_ns;
    rt->last_debug.sensor_mask = rt->sensor_mask;
    rt->last_debug.batch_crc32 = rt->batch_crc32;
    rt->last_debug.coherence_q16 = rt->coherence_q16;
    rt->last_debug.entropy_q16 = rt->entropy_q16;
    rt->last_debug.invariant_milli = rt->invariant_milli;
    rt->last_debug.dropped_samples = rt->ring.dropped;
    rt->last_debug.flags = rt->flags | rt->ring.flags;
    rt->last_debug.stage = rt->stage;
    rt->last_debug.output[0] = (uint32_t)(rt->hash64 & 0xFFFFFFFFu);
    rt->last_debug.output[1] = (uint32_t)((rt->hash64 >> 32u) & 0xFFFFFFFFu);
    rt->last_debug.output[2] = rt->torus_q16[0];
    rt->last_debug.output[3] = rt->torus_q16[1];
    rt->last_debug.output[4] = rt->torus_q16[2];
    rt->last_debug.output[5] = rt->torus_q16[3];
    rt->last_debug.output[6] = rt->torus_q16[4];
    rt->last_debug.output[7] = rt->torus_q16[6];

    if (out_frame != (void*)0) {
        *out_frame = rt->last_debug;
    }

    return (int)processed;
}

int raf_sensor_debug_snapshot(const raf_sensor_runtime* rt,
                              raf_sensor_debug_frame* out_frame) {
    if (rt == (void*)0 || out_frame == (void*)0) {
        return RAF_SENSOR_ERR_NULL;
    }
    *out_frame = rt->last_debug;
    return RAF_SENSOR_OK;
}
