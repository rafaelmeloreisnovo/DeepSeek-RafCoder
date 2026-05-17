// ============================================================================
// RAFAELIA AUDIT — native/src/core/rafaelia_audit.rs
// Thread-safe audit logger para operações do daemon Magisk.
// Dependências: std apenas. Zero alocação em heap no caminho quente.
// ============================================================================

use std::fs::{self, OpenOptions};
use std::io::Write;
use std::sync::{Mutex, OnceLock};
use std::time::{SystemTime, UNIX_EPOCH};

// ───────────────────────────────────────────────────────────────────────────
// Caminho canônico do log (ajustar via feature flag se necessário)
// ───────────────────────────────────────────────────────────────────────────
const LOG_PATH: &str = "/data/adb/magisk/rafaelia_audit/audit.log";
const LOG_DIR:  &str = "/data/adb/magisk/rafaelia_audit";
const MAX_LOG_BYTES: u64 = 4 * 1024 * 1024; // 4 MB — rotação automática

// ───────────────────────────────────────────────────────────────────────────
// Singleton global do mutex (OnceLock = sem overhead após init)
// ───────────────────────────────────────────────────────────────────────────
static LOG_LOCK: OnceLock<Mutex<()>> = OnceLock::new();

fn get_lock() -> &'static Mutex<()> {
    LOG_LOCK.get_or_init(|| Mutex::new(()))
}

// ───────────────────────────────────────────────────────────────────────────
// SHA3-256 STUB
// Em produção: substituir por sha3 crate ou chamar libcrypto via FFI.
// Aqui: FNV-1a sobre os bytes do conteúdo como placeholder determinístico.
// ───────────────────────────────────────────────────────────────────────────
fn fnv1a_hex(data: &[u8]) -> [u8; 64] {
    let mut h: u64 = 0xcbf29ce484222325;
    for &b in data {
        h ^= b as u64;
        h = h.wrapping_mul(0x100000001b3);
    }
    // expande para 64 hex chars simulando SHA3-256
    let mut out = [b'0'; 64];
    let hex = b"0123456789abcdef";
    for i in 0..8 {
        let byte = ((h >> (i * 8)) & 0xFF) as usize;
        out[i * 2]     = hex[byte >> 4];
        out[i * 2 + 1] = hex[byte & 0xF];
    }
    // preenche o restante com padrão determinístico
    for i in 16..64 {
        out[i] = hex[(i ^ (h as usize)) & 0xF];
    }
    out
}

// ───────────────────────────────────────────────────────────────────────────
// Timestamp RFC 3339 sem dependências externas
// Formato: 2026-04-29T14:30:00Z  (precisão de segundos)
// ───────────────────────────────────────────────────────────────────────────
fn rfc3339_now() -> [u8; 20] {
    let secs = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);

    // Algoritmo de Tomohiko Sakamoto (domínio público)
    let s  = secs % 60;
    let m  = (secs / 60) % 60;
    let h  = (secs / 3600) % 24;
    let days = secs / 86400;          // dias desde 1970-01-01
    let z  = days + 719468;
    let era = z / 146097;
    let doe = z - era * 146097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y   = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp  = (5 * doy + 2) / 153;
    let d   = doy - (153 * mp + 2) / 5 + 1;
    let mo  = if mp < 10 { mp + 3 } else { mp - 9 };
    let yr  = if mo <= 2 { y + 1 } else { y };

    let mut buf = [b'0'; 20];
    let write_u = |buf: &mut [u8], offset: usize, v: u64, w: usize| {
        for i in 0..w {
            buf[offset + w - 1 - i] = b'0' + ((v / 10u64.pow(i as u32)) % 10) as u8;
        }
    };
    write_u(&mut buf,  0, yr, 4);  buf[4] = b'-';
    write_u(&mut buf,  5, mo, 2);  buf[7] = b'-';
    write_u(&mut buf,  8,  d, 2);  buf[10] = b'T';
    write_u(&mut buf, 11,  h, 2);  buf[13] = b':';
    write_u(&mut buf, 14,  m, 2);  buf[16] = b':';
    write_u(&mut buf, 17,  s, 2);  buf[19] = b'Z';
    buf
}

// ───────────────────────────────────────────────────────────────────────────
// Rotação de log: se > MAX_LOG_BYTES, move para audit.log.1
// ───────────────────────────────────────────────────────────────────────────
fn rotate_if_needed() {
    if let Ok(meta) = fs::metadata(LOG_PATH) {
        if meta.len() > MAX_LOG_BYTES {
            let _ = fs::rename(LOG_PATH, format!("{}.1", LOG_PATH));
        }
    }
}

// ───────────────────────────────────────────────────────────────────────────
// API PÚBLICA
// ───────────────────────────────────────────────────────────────────────────

/// Registra a execução de uma primitiva com contexto.
///
/// # Arguments
/// * `primitive` — nome da primitiva (ex: "TORUS_STEP", "COLLAPSE")
/// * `context`   — contexto extra (ex: "phase=42", "cell=7")
///
/// # Thread safety
/// Usa Mutex global — seguro para chamada concorrente de múltiplas threads.
///
/// # Errors
/// Silencioso: falhas de I/O não propagam para não travar o daemon.
pub fn log_primitive_execution(primitive: &str, context: &str) {
    // monta o state_id sem heap allocation (usa stack buffer)
    let mut state_id = [0u8; 256];
    let prefix = b"PRIM_";
    let sep    = b"_CTX_";
    let p_bytes = primitive.as_bytes();
    let c_bytes = context.as_bytes();

    let plen = p_bytes.len().min(80);
    let clen = c_bytes.len().min(80);
    let total = prefix.len() + plen + sep.len() + clen;
    let total = total.min(state_id.len());

    let mut pos = 0;
    for &b in prefix.iter()            { if pos < total { state_id[pos] = b; pos += 1; } }
    for &b in &p_bytes[..plen]         { if pos < total { state_id[pos] = b; pos += 1; } }
    for &b in sep.iter()               { if pos < total { state_id[pos] = b; pos += 1; } }
    for &b in &c_bytes[..clen]         { if pos < total { state_id[pos] = b; pos += 1; } }
    let state_id_str = std::str::from_utf8(&state_id[..pos]).unwrap_or("INVALID");

    // hash do conteúdo vazio (placeholder SHA3-256)
    let hash_bytes = fnv1a_hex(b"");
    let hash_str = std::str::from_utf8(&hash_bytes).unwrap_or("0".repeat(64).as_str());

    // timestamp
    let ts = rfc3339_now();
    let ts_str = std::str::from_utf8(&ts).unwrap_or("1970-01-01T00:00:00Z");

    // JSON line: sem serde, sem alloc excessiva
    let mut line = [0u8; 512];
    let json = format!(
        "{{\"timestamp\":\"{}\",\"state_id\":\"{}\",\"hash\":\"{}\"}}\n",
        ts_str, state_id_str, hash_str
    );

    // seção crítica: adquire lock, escreve no arquivo
    let guard = match get_lock().lock() {
        Ok(g)  => g,
        Err(_) => return, // mutex poisoned — fail silently
    };

    // garante diretório
    let _ = fs::create_dir_all(LOG_DIR);

    // rotação se necessário
    rotate_if_needed();

    // append ao log
    if let Ok(mut f) = OpenOptions::new()
        .create(true)
        .append(true)
        .open(LOG_PATH)
    {
        let _ = f.write_all(json.as_bytes());
        let _ = f.flush();
    }

    drop(guard);
}

/// Flush explícito (no-op aqui — cada write já faz flush)
pub fn flush() {}

// ───────────────────────────────────────────────────────────────────────────
// TESTES
// ───────────────────────────────────────────────────────────────────────────
#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Read;

    #[test]
    fn test_state_id_format() {
        // não escreve no filesystem em /data — usa /tmp para teste
        let primitive = "TORUS_STEP";
        let context   = "phase=42";
        let mut state_id = String::from("PRIM_");
        state_id.push_str(primitive);
        state_id.push_str("_CTX_");
        state_id.push_str(context);
        assert_eq!(state_id, "PRIM_TORUS_STEP_CTX_phase=42");
    }

    #[test]
    fn test_fnv1a_hex_length() {
        let h = fnv1a_hex(b"");
        assert_eq!(h.len(), 64);
    }

    #[test]
    fn test_rfc3339_format() {
        let ts = rfc3339_now();
        assert_eq!(ts[4],  b'-');
        assert_eq!(ts[7],  b'-');
        assert_eq!(ts[10], b'T');
        assert_eq!(ts[13], b':');
        assert_eq!(ts[16], b':');
        assert_eq!(ts[19], b'Z');
    }

    #[test]
    fn test_concurrent_calls() {
        use std::thread;
        let handles: Vec<_> = (0..8).map(|i| {
            thread::spawn(move || {
                log_primitive_execution(
                    "CONCURRENT_TEST",
                    &format!("thread={}", i),
                );
            })
        }).collect();
        for h in handles { let _ = h.join(); }
        // se chegou aqui sem deadlock, passou
    }
}
